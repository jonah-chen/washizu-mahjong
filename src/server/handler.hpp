#pragma once

#include "game.hpp"

#include <array>
#include <unordered_map>
#include <vector>
#include <thread>

/**
 * The handler is responsible for handling all connections. This means, 
 * both players and spectators. The ownership of the connections is
 * passed to the game once enough players have started the game.
 * 
 * The handler is a singleton.
 * 
 */
class handler
{
public: /* Constants */
    static int constexpr max_length = 1024;
public: /* Types */
    using game_type = game;
    using protocall = typename game_type::protocall;
    using player_queue = game_type::players_type;
    using code_type = std::string;
public:
    handler(handler const &) = delete;
    handler &operator=(handler const &) = delete;

    void accept();
    code_type next_code();

private:
    asio::io_context io_context_;
    protocall::acceptor acceptor_;

    std::unordered_map<code_type, game_type> games_;

    static asio::ip::address server_ip_;
    static unsigned short server_port_;

private:
    static handler instance;
    handler();
};