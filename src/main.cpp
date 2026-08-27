#include <boost/asio.hpp>
#include <gpiod.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <memory>
#include <string>

class VirtualLed
{
  public:
    VirtualLed(sdbusplus::asio::object_server& objectServer,
               const std::string& chipName, unsigned int lineOffset) :
        chip(chipName)
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

    bool currentLedState = false;
    std::string stateStr = "xyz.openbmc_project.Led.Physical.Action.Off";

    void setLedStatus(bool status)
    {
        currentLedState = status;
        line.set_value(status ? 1 : 0);
    }

    void applyState()
    {
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
        else if (false)
        {
            uint16_t periodMs;
            uint16_t duty;
            std::cout << "D-Bus request: BLINK -> Period: " << periodMs
                      << "ms, DutyOn: " << (int)duty << "%\n";
        }
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
    VirtualLed led(objectServer, "gpiochip0", 0);

    std::cout << "Virtual LED D-Bus Service is running..." << std::endl;

    // Start the event loop; this blocks and processes D-Bus messages
    // and other asynchronous events until the service is stopped.
    io.run();

    return 0;
}
