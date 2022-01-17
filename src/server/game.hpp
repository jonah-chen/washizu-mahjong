#pragma once

#include "deck.hpp"
#include "client.hpp"

#include <fstream>
#include <array>
#include <list>
#include <vector>
#include <unordered_map>
#include <memory>

/**
 * @brief This enum class is used to represent the game state.
 */
enum class turn_state {
    game_over, start_round, /* Game handling states */
    draw, self_call, discard, opponent_call, /* Normal Gameplay */
    after_kong, /* Special Gameplay */
    next, renchan, exhaustive_draw, /* End of round */
    tsumogiri, chombo, timeout /* Bad Stuff happened */
};


class game
{
public:
    using protocall         = asio::ip::tcp;
    using client_type       = game_client;
    using client_ptr        = std::unique_ptr<client_type>;
    using players_type      = std::vector<client_ptr>;
    using spectators_type   = std::list<client_ptr>;
    using message_type      = std::string;
    using flag_type         = unsigned short;
    using deck_type         = deck;
    using card_type         = typename deck_type::card_type;
    using score_type        = int;
    using discards_type     = std::vector<card_type>;
    using state_type        = turn_state;
    using clock_type        = std::chrono::steady_clock;
    using game_id_type      = unsigned short;

public:
    static constexpr std::size_t NUM_PLAYERS = 4;

    static constexpr std::chrono::duration 
        CONNECTION_TIMEOUT      = std::chrono::milliseconds(400),
        SELF_CALL_TIMEOUT       = std::chrono::milliseconds(1500),
        DISCARD_TIMEOUT         = std::chrono::milliseconds(7000),
        OPPONENT_CALL_TIMEOUT   = std::chrono::milliseconds(1500),
        TENPAI_TIMEOUT          = std::chrono::milliseconds(1200),
        END_TURN_DELAY          = std::chrono::milliseconds(2000);
    
    static constexpr flag_type 
        RIICHI_FLAG             = 0x0001,
        DOUBLE_RIICHI_FLAG      = 0x0002,
        IPPATSU_FLAG            = 0x0004;

    static constexpr flag_type
        HEADS_UP_FLAG           = 0x0001,
        FIRST_TURN_FLAG         = 0x0002,
        CLOSED_KONG_FLAG        = 0x0004,
        OTHER_KONG_FLAG         = 0x0008,
        KONG_FLAG               = 0x000c;

    static std::unordered_map<game_id_type, game> games;

public:
    game(game_id_type id, std::ostream &server_log, std::string const &game_log_file, bool heads_up);

    ~game() = default;

    /**
     * Perform the ping. If ping cannot be recieved, the client will be disconnected.
     */
    void ping();

    /**
     * Add a spectator to the game.
     */
    void accept_spectator(client_ptr &&socket);

    /**
     * Perform a reconnection.
     */
    void reconnect(client_ptr &&socket);

    /**
     * 1 way communication from the game to all players and spectators.
     */
    template <typename ObjType>
    void broadcast(msg::header header, ObjType obj, bool exclusive=false)
    {
        for (auto &player : players)
            if (!exclusive || player != players[cur_player])
                player->send(header, obj);

        for (auto &spectator : spectators)
            spectator->send(header, obj);
    }

    /**
     * Play the game in a loop based on the state. Should be run on the main
     * thread. The function will return when the game is over. 
     */
    void play();

    /**
     * @brief Calculate the dora based on the indicator
     * 
     * @param indicator The dora indicator.
     * @return The ID (128) of the dora
     */
    static mj_id calc_dora(card_type indicator);

private:
    players_type players;
    spectators_type spectators;

    /* Player state */
    std::array<mj_hand, NUM_PLAYERS> hands {};
    std::array<mj_meld, NUM_PLAYERS> melds {};
    std::array<score_type, NUM_PLAYERS> scores {};
    std::array<discards_type, NUM_PLAYERS> discards {};
    std::array<flag_type, NUM_PLAYERS> flags;

    /* Game state */
    unsigned short game_id;
    deck_type wall;
    std::ostream &server_log;
    std::ofstream game_log;
    flag_type game_flags;   
    std::vector<card_type> dora_tiles; 
    int prevailing_wind { MJ_EAST };
    int dealer { 0 };
    int cur_player { 0 };
    state_type cur_state { state_type::start_round };
    card_type cur_tile { MJ_INVALID_TILE };
    score_type deposit {};
    score_type bonus_score {};
    unsigned short round {};

    /* Aux Objects */
    std::thread main_thread;
    std::mutex spectator_mutex;
    std::mutex rng_mutex;
    std::mutex log_mutex;

private:
    /* handle different states */
    void start_round();
    /* Normal play states */
    state_type self_call();
    state_type discard();
    state_type opponent_call();
    /* Special play states */
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
    bool self_call_kong(card_type with);
    state_type call_tsumo();
    void log_cur(char const *msg);
};
