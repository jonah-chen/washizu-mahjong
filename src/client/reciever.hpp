#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <thread>
#include <condition_variable>
#include "server/message.hpp"

/**
 * @brief An interface to safely send and receive messages from the server,
 * as well as responding to occasional server pings.
 */
class R
{
public:
    using protocall = asio::ip::tcp;
    using message_type = msg::buffer;
public:
    template <typename IPType>
    R(IPType ip, unsigned short port)
        : socket(context), server_endpoint(ip, port)
    {
        socket.connect(server_endpoint);

        t_recv = std::thread(&R::recv_impl, this);
        t_recv.detach();
    }
    
    template<typename ObjType>
    void send(msg::header header, ObjType obj)
    {
        socket.send(asio::buffer(msg::buffer_data(header, obj), msg::BUFFER_SIZE));
    }

    message_type recv();

private:
    asio::io_context context;
    protocall::endpoint server_endpoint;
    protocall::socket socket;

    std::thread t_recv;
    void recv_impl();

    msg::queue<message_type> q;

    std::condition_variable cv;
    std::mutex m;
};
