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
    std::ostream &server_log = std::cout;

    std::thread debug_thread(server_debug_terminal);
    debug_thread.detach();
    
    std::filesystem::create_directory(GAME_LOG_DIR);
    server_log << time() << "SERVER: starting\n";
    while (true)
    {
        std::stringstream ss;
        auto id = game_id();
        ss << GAME_LOG_DIR << "/" << std::setw(4) << std::setfill('0') << id << ;
        game::games.try_emplace(id, id, server_log, ss.str(), true);

        server_log << time() << "SERVER: new game " << id << " started\n";
    }
    
    return 0;
}