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
    using protocol = asio::ip::tcp;
    using message_type = msg::buffer;
public:
    /**
     * @brief Construct a new receiver object
     *
     * @details This constructor will block until the connection is established.
     * Then, it will start a thread to constantly receive messages from the
     * server and respond to pings until the connection is closed.
     *
     * @tparam IPType The type of IP address to use.
     *
     * @param ip The IP address of the server.
     * @param port The port to connect to.
     */
    template <typename IPType>
    R(IPType ip, unsigned short port)
        : server_endpoint(ip, port), socket(context)
    {
        try
        {
            socket.connect(server_endpoint);
        }
        catch (std::exception &e)
        {
            std::cerr << "Failed to connect to server" << std::endl;
            exit(EXIT_FAILURE);
        }

        t_recv = std::thread(&R::recv_impl, this);
        t_recv.detach();
    }

    /**
     * @brief Send a message to the server with no data.
     *
     * @param header The header of the message.
     */
    void send(msg::header header)
    {
        socket.send(asio::buffer(msg::buffer_data(
            header, msg::NO_INFO), msg::BUFFER_SIZE));
    }

    /**
     * @brief Send a message to the server with data.
     *
     * @tparam ObjType The type of data to send.
     *
     * @param header The header of the message.
     * @param data The data to send.
     */
    template<typename ObjType>
    void send(msg::header header, ObjType obj)
    {
        socket.send(asio::buffer(msg::buffer_data(header, obj), msg::BUFFER_SIZE));
    }

    /**
     * @brief Fetch a message that was received from the server.
     *
     * @return The message that was received. Will wait until a message is
     * received.
     */
    message_type recv()
    {
        std::unique_lock ul(m);
        cv.wait(ul, [this]{ return !q.empty(); });
        return q.pop_front();
    }

    /**
     * @brief Fetch a message that was received from the server and store it
     * in the parameters header and obj.
     *
     * @tparam ObjType The type of data to store in obj.
     *
     * @param header The header of the message.
     * @param obj The data of the message.
     */
    template<typename ObjType>
    void recv(msg::header &header, ObjType &obj)
    {
        auto cur_msg = recv();
        header = msg::type(cur_msg);
        obj = msg::data<ObjType>(cur_msg);
    }

    /**
     * @return true if there is data available to be read, false otherwise.
     */
    bool inline available() const noexcept { return !q.empty(); }

    bool inline is_open() const noexcept { return socket.is_open(); }

private:
    asio::io_context context;
    protocol::endpoint server_endpoint;
    protocol::socket socket;

    std::condition_variable cv;

    /**
     * @brief Fetch a message that was received from the server.
     *
     * @param header The header of the message.
     * @param pos The position of the player who sent the message.
     *
     * @return The message that was received. Will wait until a message is
     * received.
     */
    std::mutex m;

    msg::queue<message_type> q { cv };

    std::thread t_recv;

    /**
     * Continuously receive messages from the server and put them in the queue
     * until the connection is closed.
     */
    void recv_impl()
    {
        while(socket.is_open())
        {
            msg::buffer cur_msg;
            socket.receive(asio::buffer(cur_msg, msg::BUFFER_SIZE));
            if (msg::type(cur_msg) == msg::header::ping)
                socket.send(asio::buffer(cur_msg, msg::BUFFER_SIZE));
            else
                q.push_back(std::move(cur_msg));
        }
    }
};

#endif
