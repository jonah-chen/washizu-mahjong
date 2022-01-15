#include "game.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>
#include <iomanip>

constexpr char const *NETWORK_CONFIG_PATH = "network.cfg";
constexpr char const *GAME_LOG_DIR = "logs";
constexpr char const *GAME_LOG_FILE = "game.log";

auto time()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    return std::put_time(std::localtime(&now_c), "[%T] ");
}

void server_debug_terminal()
{
    std::string s;
    while (s != "/exit")
    {
        std::cin >> s;
        std::cout << time() << "DEBUG: " << s << "\n";
    }
    std::cout << time() << "SERVER: exiting due to terminal input" << std::endl;
    exit(0);
}

void reject_socket(game::protocall::socket &socket)
{
    asio::write(socket, asio::buffer(msg::buffer_data(
        msg::header::reject, msg::REJECT), msg::BUFFER_SIZE));
    socket.close();
}

unsigned short game_id()
{
    static unsigned short counter = 0x3f40;

    if (++counter == msg::NEW_PLAYER)
        ++counter;
    return counter;
}

int main(int argc, char **argv)
{
    std::ofstream game_log(GAME_LOG_FILE);
    std::ostream &server_log = std::cout;

    asio::io_context io_context;
    game::protocall::endpoint endpoint(game::protocall::v4(), 10000);
    game::protocall::acceptor acceptor(io_context, endpoint);

    std::thread debug_thread(server_debug_terminal);
    debug_thread.detach();

    std::map<unsigned short, game> games;
    std::unordered_set<std::string> connected_ips;

    // print the current time

    server_log << time() << "SERVER: starting\n";
    while (true)
    {
        game::players_type queue;
        queue.reserve(game::NUM_PLAYERS);

        // handle connection requests
        while (queue.size() < game::NUM_PLAYERS)
        {
            game::protocall::socket socket(io_context);
            acceptor.accept(socket);


            std::string ip = socket.remote_endpoint().address().to_string();
            server_log << time() << "SERVER: " << ip << " tried to connect\n";

#ifdef RELEASE
            if (connected_ips.find(ip) != connected_ips.end())
            {
                reject_socket(socket);
                continue;
            }
#endif

            msg::buffer conn_req;
            asio::read(socket, asio::buffer(conn_req, msg::BUFFER_SIZE));

            if (msg::type(conn_req) == msg::header::join_as_player)
            {
                auto id = msg::data<unsigned short>(conn_req);
                if (id == msg::NEW_PLAYER)
                    queue.push_back(std::move(socket));
                else if (games.find(id) == games.end())
                    reject_socket(socket);
                else // reconnection request
                {
                    asio::read(socket, asio::buffer(conn_req, msg::BUFFER_SIZE));
                    unsigned short uid = msg::data<unsigned short>(conn_req);

                    games.at(id).reconnect(uid, std::move(socket));
                }
            }
            else if (msg::type(conn_req) == msg::header::join_as_spectator)
            {
                auto game_id = msg::data<unsigned short>(conn_req);
                if (games.find(game_id) == games.end())
                {
                    continue;
                }
                games.at(game_id).accept_spectator(std::move(socket));

#ifdef RELEASE
                connected_ips.insert(ip);
#endif
            }
            else
            {
                reject_socket(socket);
            }

            for (auto &client : queue)
                client.send(msg::header::queue_size, queue.size());
        }

    // new game time
        std::stringstream ss;
        ss << GAME_LOG_DIR << "/" << std::setw(4) << std::setfill('0') << game_id() << ".log";
        games.try_emplace(game_id(), server_log, ss.str(), std::move(queue), true);

        server_log << time() << "SERVER: new game " << game_id() << " started\n";
    }
    
    return 0;
}