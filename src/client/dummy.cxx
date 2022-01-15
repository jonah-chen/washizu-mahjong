#include "server/message.hpp"
#define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include <iostream>

int main()
{
    asio::io_context io_context;

    // connect to port 10000 on localhost
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 10000);
    asio::ip::tcp::socket socket(io_context);
    socket.connect(endpoint);

    // send a connection request
    auto conn_req = msg::buffer_data(msg::header::join_as_player, msg::NEW_PLAYER);
    socket.send(asio::buffer(conn_req, msg::BUFFER_SIZE));
    while (1)
    {
        // receive a message
        msg::buffer msg;
        socket.receive(asio::buffer(msg, msg::BUFFER_SIZE));
        
        if (msg::type(msg) == msg::header::ping)
        {
            socket.send(asio::buffer(msg, msg::BUFFER_SIZE));
        }
        // print the message
        std::cout << static_cast<char>(msg::type(msg)) << " " << msg::data<unsigned short>(msg) << std::endl;
    }
    return 0;
}