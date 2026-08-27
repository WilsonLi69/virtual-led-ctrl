#include <boost/asio.hpp>
#include <gpiod.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <memory>

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

    std::cout << "Virtual LED D-Bus Service is running..." << std::endl;

    // Start the event loop; this blocks and processes D-Bus messages
    // and other asynchronous events until the service is stopped.
    io.run();

    return 0;
}
