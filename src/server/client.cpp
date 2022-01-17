#include "client.hpp"

game_client::game_client(queue_type &shared_q, unsigned short &game_id, bool &as_player) 
    : q(shared_q), uid(next_uid()), socket(context)
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
    msg::buffer conn_req, conn_id;
    try 
    {
        socket.send(asio::buffer(msg::buffer_data(msg::header::your_id, uid), msg::BUFFER_SIZE));
        conn_req = recv(CONNECTION_TIMEOUT);
        conn_id  = recv(CONNECTION_TIMEOUT);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        socket.close();
        return;
    }

    if (msg::type(conn_id) != msg::header::your_id)
    {
        reject();
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
        reject();
        return;
    }

    listener = std::thread(&game_client::listening, this);
    pinger = std::thread(&game_client::pinging, this);
    listener.detach();
    pinger.detach();
}

game_client::game_client(game_client &&other)
    :   listener(std::move(other.listener)),
        pinger(std::move(other.pinger)),
        socket(std::move(other.socket)),
        uid(other.uid), q(other.q) {}

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
            socket.close();
        }

        if (msg::type(buf) == msg::header::ping)
            ping_recv.notify_one();
        else
            q.push_back({uid, buf});
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
            std::cerr << "PING NOT REPLIED TO " << uid << "\n";
            socket.close();
        }
    }
}

void game_client::reject()
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


asio::io_context game_client::context;

game_client::protocall::endpoint game_client::server_endpoint(game_client::protocall::v4(), 10000);

game_client::protocall::acceptor game_client::acceptor(game_client::context, game_client::server_endpoint);