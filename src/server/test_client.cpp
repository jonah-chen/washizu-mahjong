#define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
    // 10 byte buffer string
    std::string x = "123456789012";

    // Connect to the server on localhost
    asio::io_context io_context;
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 8080);
    asio::ip::tcp::socket socket(io_context);
    socket.connect(endpoint);

    // Send the data
    asio::write(socket, asio::buffer(x));

    // read the response
    asio::read(socket, asio::buffer(x));

    // Print the data

    std::cout << x << std::endl;

    return 0;
}