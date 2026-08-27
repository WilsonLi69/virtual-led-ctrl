#include <systemd/sd-event.h>

#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/event.hpp>
#include <sdbusplus/server.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <xyz/openbmc_project/Led/Physical/server.hpp>

#include <chrono>
#include <iostream>
#include <string>

using LedPhysicalInterface =
    sdbusplus::xyz::openbmc_project::Led::server::Physical;
using LedObject = sdbusplus::server::object_t<LedPhysicalInterface>;

class LedController : public LedObject
{
  public:
    LedController(sdbusplus::bus_t& bus, const char* path,
                  const std::string& chipName, unsigned int lineOffset,
                  sdeventplus::Event& event) :
        LedObject(bus, path, action::defer_emit), chip(chipName),
        timer(event, [this](auto&) { this->toggleLed(); })
    {
        // Initialize the GPIO
        line = chip.get_line(lineOffset);
        gpiod::line_request request;
        request.request_type = gpiod::line_request::DIRECTION_OUTPUT;
        request.consumer = "Virtual-LED-Ctrl";

        // Request the line, defaulting to output low (0).
        line.request(request, 0);

        // Set the initial D-Bus state property.
        LedObject::state(LedPhysicalInterface::Action::Off);

        // Ready: publish this object on D-Bus.
        this->emit_object_added();

        std::cout << "LED Controller initialized on " << chipName << ", line "
                  << lineOffset << "\n";
    }

    // When a client changes the State property via D-Bus, sdbusplus
    // automatically calls this override.
    LedPhysicalInterface::Action state(
        LedPhysicalInterface::Action value) override
    {
        if (value == LedPhysicalInterface::Action::On)
        {
            line.set_value(1);
            std::cout << "D-Bus request: ON -> Set GPIO High\n";
        }
        else if (value == LedPhysicalInterface::Action::Off)
        {
            line.set_value(0);
            std::cout << "D-Bus request: OFF -> Set GPIO Low\n";
        }
        else if (value == LedPhysicalInterface::Action::Blink)
        {
            std::cout << "D-Bus request: BLINK -> Start Timer\n";
            timer.restart(std::chrono::milliseconds(500));
        }

        // Call the parent to store the new state in the property so that other
        // readers get the latest value.
        return LedObject::state(value);
    }

  private:
    gpiod::chip chip;
    gpiod::line line;

    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;

    bool currentLedState = false;

    void setLedStatus(bool status)
    {
        currentLedState = status;
        line.set_value(status ? 1 : 0);
    }

    void toggleLed()
    {
        setLedStatus(!currentLedState);
        std::cout << "Timer tick: LED is now "
                  << (currentLedState ? "High" : "Low") << "\n";
    }
};

int main()
{
    std::string objPath = "/xyz/openbmc_project/led/physical/virtual_test";

    // Create the event
    auto event = sdeventplus::Event::get_default();

    // Create a new bus
    auto bus = sdbusplus::bus::new_default();

    bus.request_name("xyz.openbmc_project.LED.Controller.VirtualTest");

    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    // Register an Object Manager
    sdbusplus::server::manager_t manager(bus, objPath.c_str());

    // Instantiate LED controller
    LedController led(bus, objPath.c_str(), "gpiochip0", 0, event); // test pin

    std::cout << "Virtual LED D-Bus Service is running...\n";

    return event.loop();
}
