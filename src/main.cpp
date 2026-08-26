#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <iostream>
#include <string>

int main()
{
    // Create a new bus and affix an object manager for the subtree path we
    // intend to place objects at..
    auto bus = sdbusplus::bus::new_default();

    bus.request_name("xyz.openbmc_project.LED.Controller.VirtualTest");

    std::string objPath = "/xyz/openbmc_project/led/physical/virtual_test";

    sdbusplus::server::manager_t manager(bus, objPath.c_str());

    std::cout << "Virtual LED D-Bus Service is running...\n";

    bus.process_loop();

    std::cout << "End of Virtual LED D-Bus Service ...\n";

    return 0;
}
