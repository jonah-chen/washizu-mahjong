#define ASIO_STANDALONE
#include <asio.hpp>
#include <deque>
#include <mutex>
#include "message.hpp"
#include <iostream>

class msgq
{
public:
    using msg_type = msg::buffer;
    using container_type = std::deque<msg_type>;
public:
    msgq() = default;
    ~msgq() = default;

    msgq(msgq &&other) : container(std::move(other.container)) {}

    void push_back(msg_type &&message)
    {
        std::scoped_lock lock(mutex);
        container.push_back(std::move(message));
    }

    void flush()
    {
        std::scoped_lock lock(mutex);
        container.clear();
    }

    msg_type pop_front()
    {
        std::scoped_lock lock(mutex);
        msg_type elem = container.front();
        container.pop_front();
        return elem;
    }

    bool empty() const { return container.empty(); }

private:
    container_type container {};
    std::mutex mutex;
};

template<typename SocketType>
class game_client 
{
public:
    using socket_type = SocketType;
    using id_type = unsigned short;
    static constexpr auto PING_FREQ = std::chrono::seconds(5);
    static constexpr auto PING_TIMEOUT = std::chrono::milliseconds(900);
public:
    id_type uid;
    socket_type socket;
    std::mutex mutex;

    game_client(socket_type &&socket)
        : socket(std::move(socket)), uid(next_uid()), 
            listener(&game_client<SocketType>::listening, this),
            pinger(&game_client<SocketType>::pinging, this) {
                std::cout << 
            }

    game_client(game_client const &) = delete;
    
    game_client(game_client &&other)
        : socket(std::move(other.socket)), uid(other.uid), q(std::move(other.q)),
        listener(std::move(other.listener)), pinger(std::move(other.pinger)) {}

    ~game_client() 
    {
        if (socket.is_open())
            socket.close();
    }

    static id_type next_uid() 
    {
        static id_type counter = 8000;
        return counter++;
    } 
    
    inline bool operator==(const game_client &other) const { return uid == other.uid; }
    inline bool operator!=(const game_client &other) const { return uid != other.uid; }

    template <typename ObjType>
    std::size_t send(msg::header header, ObjType obj)
    {
        try
        {
            return socket.send(asio::buffer(msg::buffer_data(header, obj), msg::BUFFER_SIZE));
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        return 0;
        
    }

    template <typename TimepointType>
    msg::buffer recv(TimepointType until)
    {
        if (!q.empty())
            return q.pop_front();

        std::mutex local_m;
        std::condition_variable local_cv;
        std::unique_lock local_l(local_m);

        if (local_cv.wait_until(local_l, until, [this](){ return !q.empty(); }))
            return q.pop_front();

        return msg::buffer_data(msg::header::timeout, msg::TIMEOUT);
    }


private:
    msgq q;
    std::thread listener;
    std::thread pinger;
    bool ping_lock {false};

    void listening()
    {
        while (socket.is_open())
        {
            msg::buffer buf;
            try
            {
                socket.receive(asio::buffer(buf, msg::BUFFER_SIZE));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }

            q.push_back(std::move(buf));
        }
    }

    void pinging()
    {
        while (true)
        {
            std::this_thread::sleep_for(PING_FREQ);
            if (q.empty())
            {
                send(msg::header::ping, msg::PING);
                std::cout << "Pinged\n";
                if (msg::type(recv(std::chrono::steady_clock::now()+PING_TIMEOUT))!=msg::header::ping)
                {
                    socket.close();
                }
            }
            else
                std::cout << "There is already something in Q\n";
        }
    }
};
