#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <thread>
#include <condition_variable>
#include "server/message.hpp"

class R
{
public:
    using protocall = asio::ip::tcp;
    using message_type = msg::buffer;
public:
    R();

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

    std::condition_variable local_cv;
    std::mutex local_m;
    bool send_lock {false};
};
