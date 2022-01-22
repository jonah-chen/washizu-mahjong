#ifndef MJ_SERVER_CLIENT_HPP
#define MJ_SERVER_CLIENT_HPP

#define ASIO_STANDALONE
#include <asio.hpp>
#include "utils/message.hpp"
#include <unordered_set>
#include <optional>

/**
 * A message that also include the sender's id.
 */
struct identified_msg
{
    unsigned short id;
    msg::buffer data;
};

/**
 * @brief The game_client class handles the sending and receiving of messages
 * for the server.
 *
 * @details This class is implemented with the asio framework and the protocol
 * in the utils/message.hpp header.
 */
class game_client
{
public:
    using protocol      = asio::ip::tcp;
    using socket_type   = protocol::socket;
    using id_type       = unsigned short;
    using queue_type    = msg::queue<identified_msg>;
    using clock_type    = std::chrono::steady_clock;

public:
    static constexpr std::chrono::duration
        PING_FREQ           = std::chrono::milliseconds(15000),
        PING_TIMEOUT        = std::chrono::milliseconds(300),
        CONNECTION_TIMEOUT  = std::chrono::milliseconds(400);

    static asio::io_context                 context;
    static protocol::endpoint               server_endpoint;
    static protocol::acceptor               acceptor;
    static std::unordered_set<std::string>  connected_ips;

public:
    id_type uid;

    game_client(queue_type &shared_q, unsigned short &game_id, bool &as_player);

    game_client(game_client const &) = delete;

    game_client(game_client &&other);

    ~game_client() noexcept;

    /**
     * @return The next uid for a new client.
     */
    static id_type next_uid() noexcept;

    /**
     * @return The ip string of the client, or empty optional if the ip cannot
     * be retrieved.
     */
    std::optional<std::string> ip() const noexcept;

    /**
     * Equality is based on the uid.
     */
    bool inline operator==(const game_client &other) const noexcept { return uid == other.uid; }
    bool inline operator!=(const game_client &other) const noexcept { return uid != other.uid; }

    /**
     * @brief Attempts to send a message to the client. If it fails, the error
     * code is logged and the client is disconnected.
     *
     * @tparam ObjType The type of the message.
     * @param header The header of the message.
     * @param obj The data of the message.
     * @return The number of bytes sent.
     */
    template <typename ObjType>
    std::size_t send(msg::header header, ObjType obj)
    {
        try
        {
            if (socket.is_open())
                return socket.send(asio::buffer(
                    msg::buffer_data(header, obj), msg::BUFFER_SIZE));
        }
        catch(const std::exception& e)
        {
            std::cerr << "Send raised exception: " << e.what() << std::endl;
            socket.close();
        }
        return 0;
    }

    /**
     * @brief Attempts to receive a message from the client for the given
     * duration.
     *
     * @tparam DurationType The type of the duration.
     * @param wait_dur The duration to wait for.
     * @return The message if it was received, or the timeout message if it
     * timed out.
     */
    template <typename DurationType>
    msg::buffer recv(DurationType wait_dur)
    {
        msg::buffer buf;
        std::unique_lock ul(local_m);
        if (local_cv.wait_for(ul, wait_dur,
            [this](){ return socket.available() >= msg::BUFFER_SIZE; }))
        {
            socket.receive(asio::buffer(buf, msg::BUFFER_SIZE));
            return buf;
        }
        return msg::buffer_data(msg::header::timeout, msg::TIMEOUT);
    }

    /**
     * @brief Reject the connection by sending the reject message and then
     * disconnecting the client.
     */
    void reject() noexcept;

    /**
     * @brief Close the connection by removing the ip from the connected_ips
     * set and closing the socket.
     */
    void close() noexcept;

    /**
     * @return If the connection to this client is still open.
     */
    bool inline is_open() const noexcept { return socket.is_open(); }

private:
    /**
     * reference to the shared queue
     */
    queue_type &q;

    std::thread listener;
    std::thread pinger;

    std::mutex local_m;
    std::condition_variable local_cv;

    std::mutex ping_m;
    std::condition_variable ping_recv;

    socket_type socket { context };

    /**
     * The method for the listening thread to constantly listen for messages
     * until the client disconnects. It also fetches special ping messages to
     * notify the pinging thread. Ping messages are not added to the queue.
     * The method returns when the client disconnects.
     */
    void listening();

    /**
     * The method for the pinging thread to ping the client periodically until
     * the client disconnects. The method returns when the client disconnects.
     */
    void pinging();
};

#endif
