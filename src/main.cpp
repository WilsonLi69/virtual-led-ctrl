#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/detail/error_code.hpp>
#include <gpiod.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

class VirtualLed
{
  public:
    VirtualLed(boost::asio::io_context& io,
               sdbusplus::asio::object_server& objectServer,
               const std::string& chipName, unsigned int lineOffset) :
        chip(chipName), timer(io)
    {
        // Initialize the GPIO
        line = chip.get_line(lineOffset);
        gpiod::line_request request;
        request.request_type = gpiod::line_request::DIRECTION_OUTPUT;
        request.consumer = "Virtual-LED-Ctrl";

        // Request the line, defaulting to output low (0).
        line.request(request, 0);

        // Register a D-Bus interface
        std::string objPath = "/xyz/openbmc_project/led/physical/virtual_test";
        iface = objectServer.add_interface(objPath,
                                           "xyz.openbmc_project.Led.Physical");

        iface->register_property(
            "State", stateStr,
            [this](const std::string& req, std::string& old) {
                stateStr = req;
                old = req;
                applyState();
                return 1;
            });

        iface->register_property("Period", periodMs,
                                 [this](const uint16_t& req, uint16_t& old) {
                                     periodMs = req;
                                     old = req;
                                     if (stateStr == actionBlink)
                                         applyState();
                                     return 1;
                                 });
        iface->register_property("DutyOn", duty,
                                 [this](const uint8_t& req, uint8_t& old) {
                                     duty = req;
                                     old = req;
                                     if (stateStr == actionBlink)
                                         applyState();
                                     return 1;
                                 });

        iface->initialize();

        std::cout << "LED Controller initialized on " << chipName << ", line "
                  << lineOffset << std::endl;
    }

  private:
    gpiod::chip chip;
    gpiod::line line;

    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
    const std::string actionOn = "xyz.openbmc_project.Led.Physical.Action.On";
    const std::string actionOff = "xyz.openbmc_project.Led.Physical.Action.Off";
    const std::string actionBlink =
        "xyz.openbmc_project.Led.Physical.Action.Blink";

    boost::asio::steady_timer timer;

    bool currentLedState = false;
    std::string stateStr = "xyz.openbmc_project.Led.Physical.Action.Off";
    uint16_t periodMs = 1000;
    uint8_t duty = 50;

    void setLedStatus(bool status)
    {
        currentLedState = status;
        line.set_value(status ? 1 : 0);
    }

    void applyState()
    {
        timer.cancel();
        if (stateStr == actionOn)
        {
            setLedStatus(true);
            std::cout << "D-Bus request: ON -> Set GPIO High" << std::endl;
        }
        else if (stateStr == actionOff)
        {
            setLedStatus(false);
            std::cout << "D-Bus request: OFF -> Set GPIO Low" << std::endl;
        }
        else if (stateStr == actionBlink)
        {
            std::cout << "D-Bus request: BLINK -> Period: " << periodMs
                      << "ms, DutyOn: " << (int)duty << std::endl;
            if (duty == 100)
            {
                setLedStatus(true);
            }
            else if (duty == 0)
            {
                setLedStatus(false);
            }
            else
            {
                setLedStatus(true);
                scheduleNextBlink();
            }
        }
    }

    void scheduleNextBlink()
    {
        uint32_t waitTimeMs = currentLedState ? (periodMs * duty / 100)
                                              : (periodMs * (100 - duty) / 100);
        timer.expires_after(std::chrono::milliseconds(waitTimeMs));

        timer.async_wait([this](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }
            if (ec)
            {
                std::cerr << "Timer error: " << ec.message() << std::endl;
                return;
            }
            setLedStatus(!currentLedState);
            scheduleNextBlink();
        });
    }
};

int main()
{
    // Create an I/O context, which provides the core event loop
    // for all asynchronous operations (e.g., D-Bus message handling).
    boost::asio::io_context io;

    // Establish a new D-Bus connection by wrapping the io_context
    // in an sdbusplus ASIO connection object.
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);

    // Request ownership of the well-known service name on the system bus,
    // so other clients can reach this virtual LED controller service.
    systemBus->request_name("xyz.openbmc_project.LED.Controller.VirtualTest");

    // Register an Object Manager on the bus to handle introspection
    // and to expose the objects managed by this service.
    sdbusplus::asio::object_server objectServer(systemBus);

    // Instantiate LED controller
    VirtualLed led(io, objectServer, "gpiochip0", 0);

    std::cout << "Virtual LED D-Bus Service is running..." << std::endl;

    // Start the event loop; this blocks and processes D-Bus messages
    // and other asynchronous events until the service is stopped.
    io.run();

    return 0;
}
