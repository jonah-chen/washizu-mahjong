#include "client.hpp"
#include <iostream>

game_client::game_client(queue_type &shared_q, unsigned short &game_id, bool &as_player)
    : uid(next_uid()), q(shared_q)
{
    acceptor.accept(socket);
    std::string ip = socket.remote_endpoint().address().to_string();

    if (online_mode)
    {
        if (connected_ips.find(ip) != connected_ips.end())
        {
            reject();
            return;
        }
        else
        {
            connected_ips.insert(ip);
        }
    }

    msg::buffer conn_req, conn_id;
    try
    {
        socket.send(asio::buffer(msg::buffer_data(msg::header::your_id, uid), msg::BUFFER_SIZE));
        conn_req = recv(CONNECTION_TIMEOUT);
        conn_id  = recv(CONNECTION_TIMEOUT);
    }
    catch (const std::exception& e)
    {
        std::cerr << "game_client::game_client raised " << e.what() << std::endl;
        close();
        return;
    }

    if (msg::type(conn_id) != msg::header::my_id)
    {
        close();
        return;
    }

    msg::header header = msg::type(conn_req);
    unsigned short id = msg::data<unsigned short>(conn_id);

    if (header == msg::header::join_as_player)
    {
        game_id = msg::data<unsigned short>(conn_req);
        as_player = true;
        uid = id;
    }
    else if (header == msg::header::join_as_spectator && id == uid)
    {
        game_id = msg::data<unsigned short>(conn_req);
        as_player = false;
    }
    else
    {
        close();
        return;
    }

    listener = std::thread(&game_client::listening, this);
    pinger = std::thread(&game_client::pinging, this);
    listener.detach();
    pinger.detach();
}

game_client::game_client(game_client &&other)
    :   uid(other.uid), q(other.q),
        listener(std::move(other.listener)),
        pinger  (std::move(other.pinger)),
        socket  (std::move(other.socket))
{}

game_client::~game_client() noexcept
{
    close();
}

game_client::id_type game_client::next_uid() noexcept
{
    static game_client::id_type counter = 8000;
    return counter++;
}

std::optional<std::string> game_client::ip() const noexcept
{
    try
    {
        if (socket.is_open())
            return socket.remote_endpoint().address().to_string();
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        std::cerr << "game_client::ip tried to raise " << e.what()
            << " but is actively supressed" << std::endl;
        return std::nullopt;
    }
}

void game_client::close() noexcept
{
    if (online_mode)
    {
        auto ip_addr = ip();
        if (ip_addr)
            connected_ips.erase(*ip_addr);
    }
    if (socket.is_open())
        socket.close();
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
            std::cerr << "Listening thread raised: " << e.what() << std::endl;
            close();
        }

        if (msg::type(buf) == msg::header::ping)
            ping_recv.notify_one();
        else
        {
#ifndef NDEBUG
            std::cout << "Received from " << uid << ": " << (char)msg::type(buf) <<
            ' ' << msg::data<unsigned short>(buf) << std::endl;
#endif
            q.push_back({uid, buf});
        }
    }
}

void game_client::pinging()
{
    while (socket.is_open())
    {
        std::this_thread::sleep_for(PING_FREQ);
        send(msg::header::ping, msg::PING);
        std::unique_lock ul(ping_m);
        if (ping_recv.wait_for(ul, PING_TIMEOUT) == std::cv_status::timeout)
        {
            std::cerr << "Ping not replied by " << uid << " closing connection...\n";
            close();
        }
    }
}

void game_client::reject() noexcept
{
    try
    {
        send(msg::header::reject, msg::REJECT);
    }
    catch(const std::exception& e)
    {}
    if (socket.is_open())
        socket.close();
}

bool game_client::online_mode;

asio::io_context game_client::context;

game_client::protocol::endpoint game_client::server_endpoint(
    game_client::protocol::v4(), MJ_SERVER_DEFAULT_PORT);

game_client::protocol::acceptor game_client::acceptor(
    game_client::context, game_client::server_endpoint);

std::unordered_set<std::string> game_client::connected_ips;
