#include "client.hpp"


void msgq::push_back(msgq::msg_type &&message)
{
    std::scoped_lock lock(mutex);
    container.push_back(std::move(message));
}

void msgq::flush()
{
    std::scoped_lock lock(mutex);
    container.clear();
}

msgq::msg_type msgq::pop_front()
{
    std::scoped_lock lock(mutex);
    msg_type elem = container.front();
    container.pop_front();
    return elem;
}

inline bool msgq::empty() const { return container.empty(); }

game_client::game_client() : uid(next_uid()), socket(context)
{
    acceptor.accept(socket);
    std::string ip = socket.remote_endpoint().address().to_string();

#ifdef RELEASE
    if (connected_ips.find(ip) != connected_ips.end())
    {
        reject();
        ~game_client();
        return;
    }
#endif

    listener = std::thread(&game_client::listening, this);
    pinger = std::thread(&game_client::pinging, this);
    listener.detach();
    pinger.detach();
}

game_client::game_client(game_client &&other)
    :   listener(std::move(other.listener)),
        pinger(std::move(other.pinger)),
        socket(std::move(other.socket)),
        uid(other.uid) {}

game_client::~game_client()
{
    if (socket.is_open())
        socket.close();
}

game_client::id_type game_client::next_uid()
{
    static game_client::id_type counter = 8000;
    return counter++;
}

std::string game_client::ip() const
{
    return socket.remote_endpoint().address().to_string();
}

void game_client::listening()
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

void game_client::pinging()
{
    while (socket.is_open())
    {
        std::this_thread::sleep_for(PING_FREQ);
        if (q.empty())
        {
            send(msg::header::ping, msg::PING);
            ping_lock = true;
            auto reply = recv(std::chrono::steady_clock::now()+PING_TIMEOUT, false);
            ping_lock = false;
            if (msg::type(reply)!=msg::header::ping)
            {
                if (msg::type(reply)==msg::header::timeout)
                    std::cout << "TIMEOUT: ";
                std::cout << "PING NOT REPLIED TO " << uid << "\n";
            }
        }
        else
            std::cout << "There is already something in Q\n";
    }
}

void game_client::reject()
{
    send(msg::header::reject, msg::REJECT);
    if (socket.is_open())
        socket.close();
}

asio::io_context game_client::context;

game_client::protocall::endpoint game_client::server_endpoint(game_client::protocall::v4(), 10000);

game_client::protocall::acceptor game_client::acceptor(game_client::context, game_client::server_endpoint);