#pragma once

#include "deck.hpp"
#include "utils.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <iostream>
#include <array>
#include <vector>

template<typename SocketType>
struct game_client 
{
    using socket_type = SocketType;
    socket_type socket;
    std::mutex mutex;
    game_client(socket_type &&socket)
        : socket(std::move(socket)) {}
    game_client(game_client &&other)
        : socket(std::move(other.socket)), mutex(std::move(other.mutex)) {}
    
    template<typename... Args>
    std::size_t send(Args &&... args)
    {
        std::scoped_lock lock(client.mutex);
        return asio::write(socket, args...);
    }

    template<typename... Args>
    std::size_t recv(Args &&... args)
    {
        std::scoped_lock lock(client.mutex);
        return asio::read(socket, args...);
    }
};


class game
{
public:
    static constexpr std::size_t NUM_PLAYERS = 4;

public:
    using protocall = asio::ip::tcp;
    using client_type = game_client<protocall::socket>;
    using players_type = std::array<client_type, NUM_PLAYERS>;
    using spectators_type = std::vector<client_type>;
    using message_type = std::string;
    using io_type = asio::io_context;
    using deck_type = deck;
    using card_type = typename deck_type::card_type;
    using score_type = int;
    using discards_type = std::vector<card_type>;
    enum class call_type : unsigned char {
        pass, chow, pong, kong, richii, ron, tsumo
    };

public:
    game(io_type &io, players_type &&players, bool heads_up);

    ~game();

    /**
     * Perform the ping. If ping cannot be recieved, the client will be disconnected.
     */
    void ping();

    /**
     * Add a spectator to the game.
     */
    void accept_spectator(protocall::socket &&socket);

    /**
     * Perform a reconnection.
     */
    void reconnect(int player, protocall::socket &&socket);
    
    /**
     * 2 way communication between the player and the game.
     * 
     * A: Turn starts by the server giving draw to the player. If tile is transparent, 
     * the tile will be sent to all players and spectators. Otherwise, only the 
     * player will be sent the tile and the other players will be sent INVALID_TILE.
     * 
     * Then it waits:
     * B: Player can respond with (1) call, (2) discard.
     * 
     * (1) Server checks if call is valid. If not, server will respond with REJECT. This
     * runs in a loop with a single timeout, so it cannot be exploited.
     * 
     * (2) Server checks if the discard is valid. If not, the server will tell the client
     * to crash, because this means it is a bug.
     * If the player timeout, they will discard the last tile they drew by default.
     * 
     * C: Then, this players turn is over and other players are given the chance to call
     * that tile. The call follows with the precedence described by the rules. 
     * Server checks if call is valid. If not, server will respond with REJECT. This
     * runs in a loop with a single timeout, so it cannot be exploited.
     * The turn will then proceed to the player who called, in the discard step B2.
     * 
     * If players timeout, they will by default pass the call.
     * 
     * 
     * D: If the tile cannot be called by any player, a random delay between 0 and 3 seconds
     * is added. Then, the turn proceeds to the next player and this process repeats.
     * 
     */
    void player_io(int player);

    /**
     * 1 way communication from the game to all players and spectators.
     */
    void broadcast();

    void log(std::ostream &os, const std::string &msg);

private:
    players_type players;
    spectators_type spectators;

    deck_type wall;

    std::array<mj_hand, NUM_PLAYERS> hands;
    std::array<mj_meld, NUM_PLAYERS> melds {};
    std::array<score_type, NUM_PLAYERS> scores {};
    std::array<discards_type, NUM_PLAYERS> discards {};
    
    int prevailing_wind { MJ_EAST };
    int dealer { 0 };
    int cur_player { 0 };
    enum class turn_state {
        call, /* Calling richii, closed kong, normal kong on meld */
        discard, /* Discarding */
        wait_for_call /* If opponent can call the tile you discarded */
    } turn_state { turn_state::discard };

    card_type cur_tile { MJ_INVALID_TILE };

    bool heads_up;

    std::thread ping_thread;

private:
    template<typename... Args>
    std::size_t send(client_type &client, Args &&... args)
    {
        std::scoped_lock lock(client.mutex);
        return asio::write(client.socket, args...);
    }

    template<typename... Args>
    std::size_t recv(client_type &client, Args &&... args)
    {
        std::scoped_lock lock(client.mutex);
        return asio::read(client.socket, args...);
    }

    void reshuffle();

    call_type call() const;
};