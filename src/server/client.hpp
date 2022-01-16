#define ASIO_STANDALONE
#include <asio.hpp>
#include <deque>
#include <mutex>
#include "message.hpp"
#include <iostream>
#include <unordered_set>

class msgq
{
public:
    using msg_type = msg::buffer;
    using container_type = std::deque<msg_type>;

public:
    msgq() = default;
    ~msgq() = default;

    msgq(msgq &&other) : container(std::move(other.container)) {}

    void push_back(msg_type &&message);

    void flush();

    msg_type pop_front();

    bool empty() const;

private:
    container_type container {};
    std::mutex mutex;
};


class game_client 
{
public:
    using protocall = asio::ip::tcp;
    using socket_type = protocall::socket;
    using id_type = unsigned short;

    static constexpr std::chrono::duration 
        PING_FREQ       = std::chrono::seconds(15);
    static constexpr std::chrono::duration
        PING_TIMEOUT    = std::chrono::milliseconds(300);
    
    static asio::io_context context;
    static protocall::endpoint server_endpoint;
    static protocall::acceptor acceptor;
    static std::unordered_set<std::string> connected_ips;
public:
    id_type uid;
    socket_type socket;
    std::mutex mutex;

    game_client();

    game_client(game_client const &) = delete;

    game_client(game_client &&other);

    ~game_client();

    static id_type next_uid();

    std::string ip() const;
    
    inline bool operator==(const game_client &other) const { return uid == other.uid; }
    inline bool operator!=(const game_client &other) const { return uid != other.uid; }

    template <typename ObjType>
    std::size_t send(msg::header header, ObjType obj)
    {
        try
        {
            if (socket.is_open())
                return socket.send(asio::buffer(msg::buffer_data(header, obj), msg::BUFFER_SIZE));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            socket.close();
        }
        return 0;
        
    }

    template <typename TimepointType>
    msg::buffer recv(TimepointType until, bool controlled=true)
    {
        std::unique_lock local_l(local_m);

        if (local_cv.wait_until(local_l, until, [this,controlled](){ 
            return !(q.empty() || (ping_lock && controlled)); 
        }))
            return q.pop_front();
            

        return msg::buffer_data(msg::header::timeout, msg::TIMEOUT);
    }

private:
    msgq q;
    std::thread listener;
    std::thread pinger;
    bool ping_lock {false};

    std::mutex local_m;
    std::condition_variable local_cv;

    void listening();

    void pinging();

    void reject();
};
