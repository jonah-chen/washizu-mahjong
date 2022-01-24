#include "extra.hpp"
#include "game.hpp"
#include <iomanip>

std::ostream &time(std::ostream &os)
{
    using namespace std::chrono;
    auto now = system_clock::to_time_t(system_clock::now());
    os << std::put_time(std::localtime(&now), "[%T] ");
    return os;
}


void server_debug_terminal()
{
    std::string s;
    while (s != "__exit")
    {
        std::cin >> s;
        if (s == "count")
            time(std::cout) << "Running games: " << game::games.size() << std::endl;
        else if (s == "ip" && game_client::online_mode)
        {
            std::cin >> s;
            if (s == "list")
            {
                time(std::cout) << game_client::connected_ips.size() <<
                    " connected IPs:\n";
                for (auto const &ip : game_client::connected_ips)
                    std::cout << ip << std::endl;
            }
            else if (s == "remove")
            {
                std::cin >> s;
                if (game_client::connected_ips.erase(s))
                    time(std::cout) << "Removed IP: " << s << std::endl;
            }
            else if (s == "count")
                std::cout << "Connected IPs: " <<
                    game_client::connected_ips.size() << std::endl;
        }
        else
            time(std::cout) << "DEBUG: " << s << " not a command yet\n";
    }
    time(std::cout) << "SERVER: exiting due to terminal input" << std::endl;
    exit(0);
}

unsigned short game_id()
{
    static unsigned short counter = 0x3f40;

    if (++counter == msg::NEW_PLAYER)
        ++counter;
    return counter;
}
