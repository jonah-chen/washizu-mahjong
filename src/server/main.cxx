#include "game.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>
#include <iomanip>
#include <filesystem>

constexpr char const *NETWORK_CONFIG_PATH = "network.cfg";
constexpr char const *GAME_LOG_DIR = "logs";
constexpr char const *GAME_LOG_SUFFIX = ".log";

auto time()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    return std::put_time(std::localtime(&now_c), "[%T] ");
}

void server_debug_terminal()
{
    std::string s;
    while (s != "__exit")
    {
        std::cin >> s;
        if (s == "count")
            std::cout << time() << "Running games: " << game::games.size() << std::endl;
        else if (s == "ip" && game_client::online_mode)
        {
            std::cin >> s;
            if (s == "list")
            {
                std::cout << time() << game_client::connected_ips.size() <<
                    " connected IPs:\n";
                for (auto const &ip : game_client::connected_ips)
                    std::cout << ip << std::endl;
            }
            else if (s == "remove")
            {
                std::cin >> s;
                if (game_client::connected_ips.erase(s))
                    std::cout << time() << "Removed IP: " << s << std::endl;
            }
            else if (s == "count")
                std::cout << "Connected IPs: " <<
                    game_client::connected_ips.size() << std::endl;
        }
        else
            std::cout << time() << "DEBUG: " << s << " not a command yet\n";
    }
    std::cout << time() << "SERVER: exiting due to terminal input" << std::endl;
    exit(0);
}

void reject_socket(game::protocol::socket &socket)
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
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
    {
        std::cout << "Usage: " << argv[0] << "--online" << std::endl;
        return 1;
    }
    offline_mode();
    std::ostream &server_log = std::cout;

    std::thread debug_thread(server_debug_terminal);
    debug_thread.detach();

    std::filesystem::create_directory(GAME_LOG_DIR);
    server_log << time() << "SERVER: starting on port " << MJ_SERVER_DEFAULT_PORT;

    if (argc >= 2 && strcmp(argv[1], "--online") == 0)
    {
        online_mode();
        server_log << " (online mode)" << std::endl;
    }
    else
    {
        offline_mode();
        server_log << " (offline mode)" << std::endl;
    }

    while (true)
    {
        std::stringstream ss;
        auto id = game_id();
        ss << GAME_LOG_DIR << "/" << std::setw(4) << std::setfill('0') <<
            id << GAME_LOG_SUFFIX;
        game::games.try_emplace(id, id, server_log, ss.str(), true);

        server_log << time() << "SERVER: new game " << id << " started" << std::endl;
    }

    return 0;
}