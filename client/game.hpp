/**
 * Client side implementation of the game. This class is responsible for
 * managing the game state, game logic, and communication with the server.
 * It is not responsible for rendering the game apart from command line output.
 *
 * Usage: Please define the mode as CLI, 2D, 3D, or RTX before including the
 * header. Also, constructing the game object requires an input stream. For
 * command line mode, the input stream should be std::cin. Otherwise, the input
 * stream should inherit from std::istream and must have an overload >> for
 * std::string.
 *
 * This module is header only, so no source files are required to be compiled.
 */

#ifndef MJ_CLIENT_GAME_HPP
#define MJ_CLIENT_GAME_HPP

#if !defined(MJ_CLIENT_MODE_CLI)    &&\
    !defined(MJ_CLIENT_MODE_2D)     &&\
    !defined(MJ_CLIENT_MODE_3D)     &&\
    !defined(MJ_CLIENT_MODE_RTX)
#error MJ_CLIENT_MODE_ {CLI,2D,3D,RTX} must be defined
#endif

#if defined(MJ_CLIENT_MODE_2D) ||\
    defined(MJ_CLIENT_MODE_3D) ||\
    defined(MJ_CLIENT_MODE_RTX)

#ifndef MJ_RENDERER
#error A renderer must be included before a 2D, 3D, or RTX game
#endif

#include "input/input.hpp"

#ifdef MJ_CLIENT_MODE_2D
namespace input = input_2d;
#else
namespace input = input_3d;
#endif

#endif

#ifndef NDEBUG
#define DEBUG_PRINT(x) std::cout << x
#else
#define DEBUG_PRINT(x)
#endif

#include "mahjong/mahjong.h"
#include "mahjong/interaction.h"
#include "utils/optim.hpp"
#include "receiver.hpp"
#include <vector>
#include <functional>
/**
 * The exception that is thrown when something goes wrong with the communication
 * with the server.
 */
class server_exception : public std::exception
{
public:
    static constexpr int ERROR_CODE = 7;
    explicit server_exception(std::string const &msg) : msg(msg) {}
    const char *what() const noexcept override { return msg.c_str(); }
private:
    std::string msg;
};


/**
 * The class that manages the game.
 */
class game
{
public:
    static constexpr int NUM_PLAYERS            = 4;
    static constexpr int MAX_DISCARD_PER_PLAYER = 24;
    static constexpr int MAX_CHOWS              = 16;
    static constexpr int MAX_QUADS              = 4;
    static constexpr int MAX_DORAS              = 5;
    static constexpr int STARTING_PTS           = 30000;
    using card_type         = mj_tile;
    using score_type        = int;
    using discards_allocator= optim<MAX_DISCARD_PER_PLAYER>::allocator<card_type>;
    using discards_type     = std::vector<card_type, discards_allocator>;

public:
    template <typename IPType>
    game(std::function<void(std::string&)> const &get, IPType ip, unsigned short port)
        : interface(ip, port)
    {
        connect_to_server();
        cmd_thread = std::thread(&game::command, this, std::ref(get));
        cmd_thread.detach();
    }

    void connect_to_server();

    /**
     * Processing one turn instruction from the server by waiting to receive
     * a message then running the appropriate function base on the type of
     * command.
     *
     * @return true if the game is still running, false if the game is over.
     */
    bool turn();

private:
    R interface;

    std::array<mj_hand, NUM_PLAYERS> hands {};
    std::array<mj_meld, NUM_PLAYERS> melds {};
    std::array<score_type, NUM_PLAYERS> scores {};
    std::array<discards_type, NUM_PLAYERS> discards {};
    std::vector<card_type, optim<MAX_DORAS*2>::allocator<card_type>> doras {};

    int my_pos;
    msg::id_type my_uid;
    msg::buffer buf;
    int prevailing_wind;
    int round_no;
    int seat_wind;
    int cur_player;
    bool in_riichi;
    mj_tile cur_tile;

    std::array<mj_pair, MAX_CHOWS> c_pairs;
    std::array<mj_tile, MAX_QUADS> c_quads;

    std::thread cmd_thread;
    std::mutex class_write_mutex;

private:
    /**
     * Start a new round by setting the prevailing wind, seat wind, and round
     * number. Also, reset the hands, melds, discards, and other round or turn
     * specific states.
     */
    void start_round();
    void start_round_update();

    /**
     * Expect the draw of a tile from the server. If the tile is
     * MJ_INVALID_TILE, then it must be an opaque tile drawn by another player.
     * Then, increase the player's hand size by one tile. Otherwise, just add
     * the tile to the player's hand.
     *
     * Call after_draw after handling the drawn tile if it is your draw.
     */
    void draw();

    /**
     * Handles the game after your draw. Includes handling Riichi, kong, tsumo,
     * and discarding.
     */
    void after_draw();

    void discard();

    /**
     * After an opponent discards a tile, check for the calls you can make
     * including ron, pong, kong, and chow.
     */
    void check_calls();

    /**
     * Completes a pong after receiving the pong call from the server.
     */
    void player_pong();

    /**
     * Completes a kong after receiving the kong call from the server. The kong
     * is a kong called from another player, not from ones own hand.
     */
    void player_kong();

    /**
     * Completes a kong after receiving the kong call from the server. The kong
     * is a kong called from a player's own hand, not from another player.
     */
    void self_kong();

    /**
     * Completes a payment instructed by the server.
     */
    void payment();

private:
    /**
     * Process an invalid message from the server by throwing an exception
     */
    void invalid_msg() const
    {
        std::stringstream ss;
        ss << "Unexpected message from server " <<
            static_cast<char>(msg::type(buf)) << " " <<
            msg::data<unsigned short>(buf) << ".";
        throw server_exception(ss.str());
    }

#ifdef MJ_CLIENT_MODE_CLI
    /**
     * Prints the current state of the game.
     */
    void print_state() const;

    void print_kong_options();
#else
    void resubmit() const;
#endif

private:
    void command(std::function<void(std::string&)> const &get);
};

#endif
