#ifndef MJ_SERVER_GAME_HPP
#define MJ_SERVER_GAME_HPP

#include "deck.hpp"
#include "client.hpp"
#include "utils/optim.hpp"

#include <fstream>
#include <array>
#include <list>
#include <vector>
#include <unordered_map>
#include <map>

/**
 * @brief Set the server to online mode, meaning it will verify there is at most
 * one client connected per IP.
 */
void inline online_mode() { game_client::online_mode = true; }

/**
 * @brief Set the server to offline mode, meaning it will not verify there is
 * at most one client connected per IP. This is useful for testing, but should
 * not be used in production as it is easy to cheat.
 */
void inline offline_mode() { game_client::online_mode = false; }

/**
 * @brief This enum class is used to represent the game state.
 */
enum class turn_state {
    /* Game handling states */
    game_over, start_round,
     /* Normal Gameplay */
    draw, self_call, discard, opponent_call,
    /* Special Gameplay */
    after_kong,
    /* End of round */
    next, renchan, exhaustive_draw,
    /* Bad Stuff happened */
    tsumogiri, chombo
};


class game
{
public:
    static constexpr int NUM_PLAYERS            = 4;
    static constexpr int MAX_DISCARD_PER_PLAYER = 24;
    static constexpr int MAX_DORAS              = 5;
    using protocol          = asio::ip::tcp;
    using client_type       = game_client;
    using client_ptr        = std::unique_ptr<client_type>;
    using players_allocator = optim<NUM_PLAYERS>::allocator<client_ptr>;
    using players_type      = std::vector<client_ptr, players_allocator>;
    using spectators_type   = std::list<client_ptr>;
    using message_type      = identified_msg;
    using flag_type         = unsigned short;
    using deck_type         = deck;
    using card_type         = typename deck_type::card_type;
    using score_type        = int;
    using discards_allocator= optim<MAX_DISCARD_PER_PLAYER>::allocator<card_type>;
    using discards_type     = std::vector<card_type, discards_allocator>;
    using state_type        = turn_state;
    using clock_type        = typename client_type::clock_type;
    using game_id_type      = unsigned short;
    using doras_allocator   = optim<MAX_DORAS*2>::allocator<card_type>;

public:
    static constexpr std::chrono::duration
        CONNECTION_TIMEOUT      = std::chrono::milliseconds(400),
        SELF_CALL_TIMEOUT       = std::chrono::milliseconds(60000),
        DISCARD_TIMEOUT         = std::chrono::milliseconds(60000),
        OPPONENT_CALL_TIMEOUT   = std::chrono::milliseconds(60000),
        TENPAI_TIMEOUT          = std::chrono::milliseconds(60000),
        END_TURN_DELAY          = std::chrono::milliseconds(2000),
        NEW_ROUND_DELAY         = std::chrono::milliseconds(12600);

    static constexpr flag_type
        RIICHI_FLAG             = 0x0001,
        DOUBLE_RIICHI_FLAG      = 0x0002,
        ANY_RIICHI_FLAG         = RIICHI_FLAG | DOUBLE_RIICHI_FLAG,
        IPPATSU_FLAG            = 0x0004;

    static constexpr flag_type
        HEADS_UP_FLAG           = 0x0001,
        FIRST_TURN_FLAG         = 0x0002,
        CLOSED_KONG_FLAG        = 0x0004,
        OTHER_KONG_FLAG         = 0x0008,
        KONG_FLAG               = 0x000c;

    static std::unordered_map<game_id_type, game> games;

    static std::array<char, 5> suit;

    static std::array<char, 4> directions;

    static std::array<char, 4> delim;

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
    /* Message handling */
    players_type                            players;
    spectators_type                         spectators;
    std::condition_variable                 timeout_cv;
    std::mutex                              timeout_m;
    msg::queue<message_type>                messages { timeout_cv };
    std::map<client_type::id_type, int>     player_id_map;

    /* Player state */
    std::array<mj_hand, NUM_PLAYERS>        hands   {};
    std::array<mj_meld, NUM_PLAYERS>        melds   {};
    std::array<score_type, NUM_PLAYERS>     scores  {};
    std::array<discards_type, NUM_PLAYERS>  discards{};
    std::array<flag_type, NUM_PLAYERS>      flags;

    /* Game level states */
    unsigned short  game_id;
    std::ostream &  server_log;
    std::ofstream   game_log;

    /* Round level states */
    deck_type                               wall;
    std::vector<card_type, doras_allocator> dora_tiles;
    int                                     prevailing_wind { MJ_EAST };
    int                                     dealer          { 0 };
    flag_type                               game_flags;

    /* Turn level states */
    int             cur_player  { 0 };
    state_type      cur_state   { state_type::start_round };
    card_type       cur_tile    { MJ_INVALID_TILE };
    score_type      deposit     {};
    score_type      bonus_score {};
    unsigned short  round       {};

    /* Aux Objects */
    std::thread main_thread;
    std::mutex  spectator_mutex;
    std::mutex  rng_mutex;

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

private:
    /**
     * @brief Tries to fetch the first message that is sent by the current player.
     *
     * @tparam TimepointType chrono is stupid.
     * @param until The time to wait until before timing out.
     * @return The first message that is sent by the current player, or timeout
     * TIMEOUT if timed out.
     */
    template <typename TimepointType>
    msg::buffer fetch_cur(TimepointType until)
    {
        while (true)
        {
            std::unique_lock ul(timeout_m);
            std::cout << messages.empty();
            if (!timeout_cv.wait_until(ul, until, [this]() { return !messages.empty(); }))
                return msg::buffer_data(msg::header::timeout, msg::TIMEOUT);

            auto msg = messages.pop_front();

            if (msg.id == players[cur_player]->uid)
                return msg.data;
        }
    }
};

#endif
