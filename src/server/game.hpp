#pragma once

#include "deck.hpp"
#include "client.hpp"

#include <fstream>
#include <array>
#include <list>
#include <vector>
#include <unordered_map>

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

/**
 * @brief The mapped vector allows the shuffling of immovable tiles.
 * 
 * @details The objects are stored in a vector and they are never moved. The
 * shuffling is done by a secondary array, which dictates the order of the
 * objects when they are indexed. The secondary array can be shuffled because
 * indices are moveable.
 * 
 * @note The iterators have not been overloaded, so the order of the iterator is
 * not guaranteed.
 * 
 * @tparam T The type of the immovable objects to store.
 */
template<typename T>
class mapped_vector : public std::vector<T>
{
public:
    mapped_vector() : std::vector<T>() {};

    template<typename... Args>
    T &emplace_back(Args &&... args)
    {
        auto &ret = std::vector<T>::emplace_back(std::forward<Args>(args)...);
        indices.push_back(indices.size());
        return ret;
    }

    T &operator[](std::size_t idx)
    {
        return std::vector<T>::operator[](indices[idx]);
    }

    const T &operator[](std::size_t idx) const
    {
        return std::vector<T>::operator[](indices[idx]);
    }

    constexpr std::size_t size() const
    {
        return indices.size();
    }

    void pop_back()
    {
        std::size_t szm1 = indices.size() - 1;
        std::remove(indices.begin(), indices.end(), szm1);
        std::vector<T>::pop_back();
    }

    template<typename RNGType>
    void shuffle(RNGType &rng)
    {
        std::shuffle(indices.begin(), indices.end(), rng);
    }

private:
    std::vector<std::size_t> indices;
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

    static constexpr std::chrono::duration 
        CONNECTION_TIMEOUT      = std::chrono::milliseconds(400),
        SELF_CALL_TIMEOUT       = std::chrono::milliseconds(1500),
        DISCARD_TIMEOUT         = std::chrono::milliseconds(1500),
        OPPONENT_CALL_TIMEOUT   = std::chrono::milliseconds(1500),
        TENPAI_TIMEOUT          = std::chrono::milliseconds(1200),
        END_TURN_DELAY          = std::chrono::milliseconds(2000);
    
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

    static std::unordered_map<unsigned short, game> games;

public:
    using protocall = asio::ip::tcp;
    using client_type = game_client;
    using players_type = mapped_vector<client_type>;
    using spectators_type = std::list<client_type>;
    using message_type = std::string;
    using flag_type = unsigned short;
    using deck_type = deck;
    using card_type = typename deck_type::card_type;
    using score_type = int;
    using discards_type = std::vector<card_type>;
    using state_type = turn_state;
    using clock_type = std::chrono::steady_clock;

public:
    game(unsigned short id, std::ostream &server_log, std::string const &game_log_file, bool heads_up);

    ~game() = default;

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
};
