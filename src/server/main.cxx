#include "game.hpp"
#include "extra.hpp"
#include <iostream>
#include <filesystem>

constexpr char const *NETWORK_CONFIG_PATH = "network.cfg";
constexpr char const *GAME_LOG_DIR = "logs";
constexpr char const *GAME_LOG_SUFFIX = ".log";


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
    time(server_log) << "SERVER: starting on port " << MJ_SERVER_DEFAULT_PORT;

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

        time(server_log) << "SERVER: new game " << id << " started" << std::endl;
    }

    return 0;
}