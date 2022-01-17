#define ASIO_STANDALONE
#include <asio.hpp>
#include "message.hpp"
#include <iostream>
#include <unordered_set>

struct identified_msg
{
    unsigned short id;
    msg::buffer data;
};

class game_client 
{
public:
    using protocall = asio::ip::tcp;
    using socket_type = protocall::socket;
    using id_type = unsigned short;
    using queue_type = msg::queue<identified_msg>;
    using clock_type = std::chrono::steady_clock;

public:
    static constexpr std::chrono::duration 
        PING_FREQ           = std::chrono::seconds(15);
    static constexpr std::chrono::duration
        PING_TIMEOUT        = std::chrono::milliseconds(300),
        CONNECTION_TIMEOUT  = std::chrono::milliseconds(400);
    
    static asio::io_context context;
    static protocall::endpoint server_endpoint;
    static protocall::acceptor acceptor;
    static std::unordered_set<std::string> connected_ips;
    
public:
    id_type uid;
    std::mutex mutex;

    game_client(queue_type &shared_q, unsigned short &game_id, bool &as_player);

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

    template <typename DurationType> 
    msg::buffer recv(DurationType wait_dur)
    {
        msg::buffer buf;
        std::unique_lock ul(local_m);
        if (local_cv.wait_for(ul, wait_dur, [this](){ return socket.available() >= msg::BUFFER_SIZE; }))
        {
            socket.receive(asio::buffer(buf, msg::BUFFER_SIZE));
            return buf;
        }
        return msg::buffer_data(msg::header::timeout, msg::TIMEOUT);     
    }

    void reject();

    bool is_open() const { return socket.is_open(); } 

private:
    queue_type &q;
    std::thread listener;
    std::thread pinger;

    std::mutex local_m;
    std::condition_variable local_cv;

    std::mutex ping_m;
    std::condition_variable ping_recv;

    socket_type socket;

    void listening();

    void pinging();
};
