#ifndef MJ_CLIENT_RECEIVER_HPP
#define MJ_CLIENT_RECEIVER_HPP

#define ASIO_STANDALONE
#include <asio.hpp>
#include <thread>
#include <condition_variable>
#include "utils/message.hpp"

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
        : server_endpoint(ip, port), socket(context)
    {
        socket.connect(server_endpoint);

        t_recv = std::thread(&R::recv_impl, this);
        t_recv.detach();
    }
    
    template<typename ObjType>
    void send(msg::header header, ObjType obj)
    {
        socket.send(asio::buffer(msg::buffer_data(header, obj), msg::BUFFER_SIZE));
        std::cout << "Sent something\n";
    }

    void send(msg::header header)
    {
        socket.send(asio::buffer(msg::buffer_data(
            header, msg::NO_INFO), msg::BUFFER_SIZE));
    }

    message_type recv();

    template<typename ObjType>
    void recv(msg::header &header, ObjType &obj)
    {
        auto cur_msg = recv();
        header = msg::type(cur_msg);
        obj = msg::data<ObjType>(cur_msg);
    }

    bool available() const noexcept { return !q.empty(); }

private:
    asio::io_context context;
    protocall::endpoint server_endpoint;
    protocall::socket socket;

    std::thread t_recv;
    void recv_impl();

    std::condition_variable cv;
    std::mutex m;

    msg::queue<message_type> q { cv };

};

#endif
