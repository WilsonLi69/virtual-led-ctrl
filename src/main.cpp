#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Led/Physical/server.hpp>

#include <iostream>
#include <string>

using LedPhysicalInterface =
    sdbusplus::xyz::openbmc_project::Led::server::Physical;
using LedObject = sdbusplus::server::object_t<LedPhysicalInterface>;

class LedController : public LedObject
{
  public:
    LedController(sdbusplus::bus_t& bus, const char* path,
                  const std::string& chipName, unsigned int lineOffset) :
        LedObject(bus, path, action::defer_emit), chip(chipName)
    {
        // 1. Initialize the GPIO (via the libgpiod C++ API).
        line = chip.get_line(lineOffset);
        gpiod::line_request request;
        request.request_type = gpiod::line_request::DIRECTION_OUTPUT;
        request.consumer = "Virtual-LED-Ctrl";

        // Request the line, defaulting to output low (0).
        line.request(request, 0);

        // 2. Set the initial D-Bus state property.
        LedObject::state(LedPhysicalInterface::Action::Off);

        // 3. Ready: publish this object on D-Bus.
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
            std::cerr
                << "Blink is not implemented in Phase 1! Defaulting to ON.\n";
            line.set_value(1); // Phase 1 treats Blink as On for now.
        }

        // Call the parent to store the new state in the property so that other
        // readers get the latest value.
        return LedObject::state(value);
    }

  private:
    gpiod::chip chip;
    gpiod::line line;
};

int main()
{
    // Create a new bus and affix an object manager for the subtree path we
    // intend to place objects at..
    auto bus = sdbusplus::bus::new_default();

    bus.request_name("xyz.openbmc_project.LED.Controller.VirtualTest");

    std::string objPath = "/xyz/openbmc_project/led/physical/virtual_test";

    sdbusplus::server::manager_t manager(bus, objPath.c_str());

    std::cout << "Virtual LED D-Bus Service is running...\n";

    LedController led(bus, objPath.c_str(), "gpiochip0", 0); // test pin

    bus.process_loop();

    return 0;
}
