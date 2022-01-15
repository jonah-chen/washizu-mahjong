#include "handler.hpp"
#include <fstream>
#include <iostream>

bool start_server(std::string const &game_log_dir)
{
    std::ifstream config_file(NETWORK_CONFIG_PATH);
    if (!config_file.is_open())
    {
        std::cerr << "Could not open config file: " << NETWORK_CONFIG_PATH << std::endl;
        return false;
    }
    std::string line;
    if (!std::getline(config_file, line))
    {
        std::cerr << "Could not read config file: " << NETWORK_CONFIG_PATH << std::endl;
        return false;
    }
    std::stringstream ss(line);
    std::string ip;
    unsigned short port;
    ss >> ip >> port;

    asio::io_context io_context;
    game::protocall::endpoint endpoint(game::protocall::v4(), port);
    game::protocall::acceptor acceptor(io_context, endpoint);

}
