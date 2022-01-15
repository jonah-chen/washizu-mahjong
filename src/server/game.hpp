#pragma once

#include "deck.hpp"
#include "utils.hpp"
#include "message.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <iostream>
#include <array>
#include <list>
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

    game_client(game_client const &) = delete;
    
    game_client(game_client &&other)
        : socket(std::move(other.socket)), uid(other.uid) {}

    ~game_client() 
    {
        if (socket.is_open())
            socket.close();
    }

    inline bool operator==(const game_client &other) const { return uid == other.uid; }
    inline bool operator!=(const game_client &other) const { return uid != other.uid; }
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

    msg::buffer recv_default()
    {
        msg::buffer buffer;
        asio::read(socket, asio::buffer(buffer, msg::BUFFER_SIZE));
        return buffer;
    }

private:
    static id_type _counter;
};

enum class turn_state {
    game_over, /* Game is over */
    start_round, /* Start of a new round */
    
    /* Normal Gameplay */
    draw,
    self_call, /* Calling richii, closed kong, normal kong on meld */
    discard, /* Discarding */
    opponent_call, /* If opponent can call the tile you discarded */
    
    /* Special Gameplay */
    after_kong,
    
    /* End of round */
    next,
    renchan,
    exhaustive_draw,

    /* Bad Stuff happened */
    tsumogiri,
    chombo,
    abort,
    timeout,
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
    
    static constexpr unsigned long  
        SELF_CALL_TIMEOUT       = 15000,
        DISCARD_TIMEOUT         = 15000,
        OPPONENT_CALL_TIMEOUT   = 15000,
        PING_TIMEOUT            = 1000;
    
    static constexpr unsigned short 
        RIICHI_FLAG             = 0x0001,
        DOUBLE_RIICHI_FLAG      = 0x0002,
        IPPATSU_FLAG            = 0x0004;

    static constexpr unsigned short
        HEADS_UP_FLAG           = 0x0001,
        FIRST_TURN_FLAG         = 0x0002,
        CLOSED_KONG_FLAG        = 0x0004,
        OTHER_KONG_FLAG         = 0x0008,
        KONG_FLAG               = 0x000c;


public:
    using protocall = asio::ip::tcp;
    using client_type = game_client<protocall::socket>;
    using players_type = std::array<client_type, NUM_PLAYERS>;
    using spectators_type = std::list<client_type>;
    using message_type = std::string;
    using io_type = asio::io_context;
    using flag_type = unsigned short;
    using deck_type = deck;
    using card_type = typename deck_type::card_type;
    using score_type = int;
    using discards_type = std::vector<card_type>;
    using state_type = turn_state;
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
            if (!exclusive || player != players[cur_player])
                player.send(header, obj);

        for (auto &spectator : spectators)
            spectator.send(header, obj);
    }

    void play();

    static mj_id calc_dora(const card_type &indicator);

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
    std::array<flag_type, NUM_PLAYERS> flags;

    flag_type game_flags;    
    int prevailing_wind { MJ_EAST };
    int dealer { 0 };
    int cur_player { 0 };
    bool first_turn { true };
    state_type cur_state { state_type::start_round };
    card_type cur_tile { MJ_INVALID_TILE };
    score_type deposit {};
    score_type bonus_score {};
    unsigned short round {};

    std::thread ping_thread;
    std::thread main_thread;
    std::mutex spectator_mutex;
    std::mutex rng_mutex;


private:
    void ping_client(client_type &client);

    /* handle different states */
    void start_round();
    state_type self_call();
    state_type discard(); /* wait for tile to recieved by server */
    state_type opponent_call();

    void after_kong();
    /* End game state */
    void next();
    void renchan();
    void exhaustive_draw();
    /* Player error states */
    void tsumogiri();
    void chombo_penalty();

    /* Helpers */
    void draw();

    void new_dora();

    void payment(int player, score_type score);

    bool self_call_kong();

    state_type call_tsumo();
};