#pragma once

#include "deck.hpp"
#include "utils.hpp"
#include "message.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <iostream>
#include <array>
#include <vector>

template<typename SocketType>
struct game_client 
{
    using socket_type = SocketType;
    using id_type = unsigned short;

    id_type uid;
    socket_type socket;
    std::mutex mutex;
    
    game_client(socket_type &&socket)
        : socket(std::move(socket)), uid(_counter) { ++_counter; }
    game_client(game_client &&other)
        : socket(std::move(other.socket)), mutex(std::move(other.mutex)), uid(other.uid) {}

    inline bool operator==(const game_client &other) const { return uid == other.uid; }

    template <typename ObjType>
    std::size_t send(msg::header header, ObjType obj)
    {
        return asio::write(socket, asio::buffer(msg::buffer_data(header, obj), msg::BUFFER_SIZE));
    }
    template<typename... Args>
    std::size_t recv(Args &&... args)
    {
        return asio::read(socket, asio::buffer(args...));
    }

private:
    static id_type _counter = 7000;
};

enum class turn_state {
    abort,
    timeout,
    call, /* Calling richii, closed kong, normal kong on meld */
    discard, /* Discarding */
    wait_for_call /* If opponent can call the tile you discarded */
};


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
class game
{
public:
    static constexpr std::size_t NUM_PLAYERS = 4;
    static constexpr std::chrono::duration PING_FREQ = std::chrono::seconds(30);

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
    using turn_state_type = turn_state;
    enum class call_type : unsigned char {
        pass, chow, pong, kong, richii, ron, tsumo
    };

public:
    game(std::ostream &server_log, std::ostream &game_log, players_type &&players, bool heads_up);

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
    void reconnect(unsigned short uid, protocall::socket &&socket);

    /**
     * 1 way communication from the game to all players and spectators.
     */
    template <typename ObjType>
    void broadcast(msg::header header, ObjType obj, bool exclusive=false)
    {
        for (auto &player : players)
            if (exclusive || player.uid != uid)
                player.send(header, obj);

        for (auto &spectator : spectators)
            spectator.send(header, obj);
    }

    void play(turn_state_type state);

    void log(std::ostream &os, const std::string &msg);

private:

    std::ostream &server_log, &game_log;

    players_type players;
    spectators_type spectators;

    deck_type wall;

    std::array<mj_hand, NUM_PLAYERS> hands;
    std::array<mj_meld, NUM_PLAYERS> melds {};
    std::array<score_type, NUM_PLAYERS> scores {};
    std::array<discards_type, NUM_PLAYERS> discards {};
    std::vector<card_type> dora_tiles;
    
    int prevailing_wind { MJ_EAST };
    int dealer { 0 };
    int cur_player { 0 };
    turn_state_type turn_state { turn_state::discard };

    card_type cur_tile { MJ_INVALID_TILE };

    bool heads_up;

    score_type deposit {};
    score_type bonus_score {};

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

    void ping_client(client_type &client);

    void draw();

    void new_dora();

    turn_state_type after_draw();

    void chombo_penalty();

    void payment(int player, score_type score);

    void end_round(bool repeat);
};