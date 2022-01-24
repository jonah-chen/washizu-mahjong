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

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) std::cout << x
#else
#define DEBUG_PRINT(x)
#endif

#include "mahjong/mahjong.h"
#include "mahjong/interaction.h"
#include "utils/optim.hpp"
#include "receiver.hpp"
#include <vector>

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
    using discards_type     = std::vector<card_type,
        optim<MAX_DISCARD_PER_PLAYER>::allocator<card_type>>;

public:
    template <typename IPType>
    game(std::istream &in, IPType ip, unsigned short port)
        : interface(ip, port)
    {
        buf = interface.recv();

        if (msg::type(buf)!=msg::header::your_id)
            throw server_exception("Could not acquire id from server.");

        my_uid = msg::data<msg::id_type>(buf);

        interface.send(msg::header::join_as_player, msg::NEW_PLAYER);
        interface.send(msg::header::my_id, my_uid);

        for (auto &d_pile : discards)
            d_pile.reserve(MAX_DISCARD_PER_PLAYER);

        while (true)
        {
            buf = interface.recv();
            my_pos = msg::data<int>(buf);

            if (msg::type(buf)==msg::header::queue_size)
                std::cout << "You are in the queue with " << my_pos <<
                    "/4 players.\n";
            else if (msg::type(buf)!=msg::header::your_position)
                invalid_msg();
            else
            {
#ifdef MJ_CLIENT_MODE_CLI
                std::cout << "You are player " << my_pos << ". You are" <<
                    (my_pos ? " not " : " ") << "the dealer.\n";
#endif
                break;
            }
        }
        std::fill(scores.begin(), scores.end(), STARTING_PTS);

        cmd_thread = std::thread(&game::command, this, std::ref(in));
        cmd_thread.detach();
    }

    /**
     * Processing one turn instruction from the server by waiting to receive
     * a message then running the appropriate function base on the type of
     * command.
     *
     * @return true if the game is still running, false if the game is over.
     */
    bool turn()
    {
        if (!interface.is_open())
            return false;

        buf = interface.recv();
        switch(msg::type(buf))
        {
        case msg::header::new_round:
            start_round();
            break;
        case msg::header::this_player_won:
            payment();
            break;
        case msg::header::this_player_drew:
            draw();
            break;
        case msg::header::tile: case msg::header::tsumogiri_tile:
            cur_tile = msg::data<mj_tile>(buf);

#ifdef MJ_CLIENT_MODE_CLI
            std::cout << "Player " << cur_player <<
                (msg::type(buf)==msg::header::tsumogiri_tile ?
                " tsumogiri " : " discarded ");
            mj_print_tile(cur_tile);
            std::cout << '\n';
#endif

            mj_discard_tile(&hands[cur_player], cur_tile);
            discards[cur_player].push_back(cur_tile);
            if (cur_player != my_pos)
                check_calls();
            break;
        case msg::header::this_player_pong: case msg::header::this_player_chow:
            cur_player = msg::data<int>(buf);

#ifdef MJ_CLIENT_MODE_CLI
            std::cout << "Player " << cur_player << " called " <<
                (msg::type(buf)==msg::header::this_player_pong ? "PONG\n" : "CHOW\n");
#endif

            player_pong();
            break;
        case msg::header::this_player_kong:
            if (cur_player == msg::data<int>(buf))
                self_kong();
            else
            {
                cur_player = msg::data<int>(buf);
                player_kong();
            }
            break;
        case msg::header::this_player_riichi:
            in_riichi = (cur_player == my_pos) ? true : in_riichi;

#ifdef MJ_CLIENT_MODE_CLI
            std::cout << "Player " << cur_player << " called RIICHI\n";
#endif

            break;
        case msg::header::dora_indicator:
            doras.push_back(msg::data<mj_tile>(buf));
            break;
        default:
            std::cerr << "Unknown Message from server: " <<
                static_cast<char>(msg::type(buf)) <<
                msg::data<unsigned short>(buf) << std::endl;
            return false;
        }

        return true;
    }

    constexpr auto const &hand() const { return hands; }
    constexpr auto const &discard() const { return discards; }
    constexpr auto const &meld() const { return melds; }
    constexpr auto const &dora() const { return doras; }
    constexpr auto const &score() const { return scores; }

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
    void start_round()
    {
        prevailing_wind = msg::data<unsigned short>(buf) >> 2;
        round_no = msg::data<unsigned short>(buf) & 3;
        seat_wind = (my_pos - round_no) & 3;

        for (auto &p_hand : hands)
            mj_empty_hand(&p_hand);
        for (auto &p_discards : discards)
            p_discards.clear();
        for (auto &p_melds : melds)
            mj_empty_melds(&p_melds);

        msg::header h;
        mj_tile t;
        int p;

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 13; ++j)
            {
                interface.recv(h,p);
                if (h == msg::header::this_player_drew)
                    interface.recv(h,t);
                else
                    throw server_exception("Round started improperly.");
                mj_add_tile(&hands[p],t);
            }
        }

        interface.recv(h, t);
        if (h != msg::header::dora_indicator)
            throw server_exception("Round started improperly.");

        doras.reserve(MAX_DORAS*2);
        doras.push_back(t);
    #ifdef MJ_CLIENT_MODE_CLI
        sep();

        switch (prevailing_wind)
        {
        case MJ_EAST: std::cout << "EAST "; break;
        case MJ_SOUTH: std::cout << "SOUTH "; break;
        case MJ_WEST: std::cout << "WEST "; break;
        case MJ_NORTH: std::cout << "NORTH "; break;
        default: throw server_exception("Invalid prevailing wind.");
        }
        std::cout << round_no + 1 << " ROUND: ";
        switch (seat_wind)
        {
        case MJ_EAST: std::cout << "YOU ARE THE EAST PLAYER\n"; break;
        case MJ_SOUTH: std::cout << "YOU ARE THE SOUTH PLAYER\n"; break;
        case MJ_WEST: std::cout << "YOU ARE THE WEST PLAYER\n"; break;
        case MJ_NORTH: std::cout << "YOU ARE THE NORTH PLAYER\n"; break;
        default: throw 2;
        }

        std::cout << "Scores:";
        for (auto &p_score : scores)
            std::cout << ' ' << p_score;
        std::cout << "\nDora indicator: ";

        mj_print_tile(t);
        std::cout << std::endl;

        if (seat_wind != MJ_EAST)
        {
            sep();
            print_state();
        }
    #endif
        cur_player = round_no;
        in_riichi = false;
    }

    /**
     * Expect the draw of a tile from the server. If the tile is
     * MJ_INVALID_TILE, then it must be an opaque tile drawn by another player.
     * Then, increase the player's hand size by one tile. Otherwise, just add
     * the tile to the player's hand.
     *
     * Call after_draw after handling the drawn tile if it is your draw.
     */
    void draw()
    {
        int player = msg::data<int>(buf);
        buf = interface.recv();
        if (msg::type(buf)!=msg::header::tile)
        {
            std::cerr << "You cannot draw something that is not a tile.\n";
            invalid_msg();
        }

        cur_tile = msg::data<mj_tile>(buf);
        mj_add_tile(&hands[player], cur_tile);

    #ifdef MJ_CLIENT_MODE_CLI
        sep();

        std::cout << "Player " << player << " drew ";
        mj_print_tile(cur_tile);
        std::cout << '\n';
    #endif
        cur_player = player;
        if (cur_player == my_pos)
            after_draw();
    }

    /**
     * Handles the game after your draw. Includes handling Riichi, kong, tsumo,
     * and discarding.
     */
    void after_draw()
    {
#ifdef MJ_CLIENT_MODE_CLI
        print_state();
#endif
        std::scoped_lock l(class_write_mutex);
        if (in_riichi)
        {
#ifdef MJ_CLIENT_MODE_CLI
            std::cout << "You are in riichi:\nT) Tsumo\n";
            print_kong_options();
            std::cout << "G) Tsumogiri\n";
#endif
            return;
        }
        if (mj_tenpai(hands[my_pos], melds[my_pos], nullptr))
        {
#ifdef MJ_CLIENT_MODE_CLI
            std::cout << "You are tenpai.\n";
            std::cout << "T) Tsumo\n";
            std::cout << "r) Riichi\n";
            print_kong_options();
#endif
        }
        else
            interface.send(msg::header::pass_calls);

#ifdef MJ_CLIENT_MODE_CLI
        std::cout << "1-" << hands[my_pos].size << ") Discard Tile\n";
#endif
    }

    /**
     * After an opponent discards a tile, check for the calls you can make
     * including ron, pong, kong, and chow.
     */
    void check_calls()
    {
        mj_size ron = mj_tenpai(hands[my_pos], melds[my_pos], nullptr);

        mj_bool pong = mj_pong_available(hands[my_pos], cur_tile, nullptr);
        mj_bool kong = mj_kong_available(hands[my_pos], cur_tile, nullptr);
        mj_size chow = (((my_pos - cur_player) & 3) == 1) ?
            mj_chow_available(hands[my_pos], cur_tile, c_pairs.data()) : 0;

#ifdef MJ_CLIENT_MODE_CLI
        if (ron)
            std::cout << "R) Call Ron\n";
        if (pong)
            std::cout << "P) Call Pong\n";
        if (kong)
            std::cout << "K) Call Kong\n";

        for (mj_size i = 0; i < chow; ++i)
        {
            std::cout << static_cast<char>('c' + i) << ") Call Chow with ";
            mj_print_pair(c_pairs[i]);
            std::cout << '\n';
        }
        if (ron || pong || kong || chow)
            std::cout << "p) Pass\n";
#endif
    }

    /**
     * Completes a pong after receiving the pong call from the server.
     */
    void player_pong()
    {
        std::array<mj_tile, 3> meld_tiles {cur_tile};
        for (int i = 1; i < 3; ++i)
        {
            buf = interface.recv();
            if (msg::type(buf)!=msg::header::tile)
            {
                std::cerr << "You cannot call something that is not a tile.\n";
                invalid_msg();
            }
            meld_tiles[i] = msg::data<mj_tile>(buf);
            if (!mj_discard_tile(&hands[cur_player], meld_tiles[i]))
                hands[cur_player].size--;
        }
        std::sort(meld_tiles.begin(), meld_tiles.end());

        mj_add_meld(&melds[cur_player], MJ_OPEN_TRIPLE(MJ_TRIPLE(
            meld_tiles[0], meld_tiles[1], meld_tiles[2])));

#ifdef MJ_CLIENT_MODE_CLI
        if (cur_player == my_pos)
        {
            print_state();
            std::cout << "1-" << hands[my_pos].size << ": Discard Tile\n";
        }
#endif
    }

    /**
     * Completes a kong after receiving the kong call from the server. The kong
     * is a kong called from another player, not from ones own hand.
     */
    void player_kong()
    {
#ifdef MJ_CLIENT_MODE_CLI
        std::cout << "Player " << cur_player << " called KONG\n";
#endif

        std::array<mj_tile, 3> meld_tiles;
        for (int i = 0; i < 3; ++i)
        {
            buf = interface.recv();
            if (msg::type(buf)!=msg::header::tile)
            {
                std::cerr << "You cannot call something that is not a tile.\n";
                invalid_msg();
            }
            meld_tiles[i] = msg::data<mj_tile>(buf);
            if (!mj_discard_tile(&hands[cur_player], meld_tiles[i]))
                hands[cur_player].size--;
        }

        mj_add_meld(&melds[cur_player], MJ_KONG_TRIPLE(MJ_TRIPLE(
            meld_tiles[0], meld_tiles[1], meld_tiles[2])));

        if (cur_player == my_pos)
        {
#ifdef MJ_CLIENT_MODE_CLI
            print_state();
            std::cout << "1-" << hands[my_pos].size << ": Discard Tile\n";
#endif
        }
    }

    /**
     * Completes a kong after receiving the kong call from the server. The kong
     * is a kong called from a player's own hand, not from another player.
     */
    void self_kong()
    {
#ifdef MJ_CLIENT_MODE_CLI
        std::cout << "Player " << cur_player << " called KONG\n";
#endif
        buf = interface.recv();
        if (msg::type(buf)!=msg::header::tile)
        {
            std::cerr << "You cannot call something that is not a tile.\n";
            invalid_msg();
        }
        mj_tile t = msg::data<mj_tile>(buf);
        auto *k = mj_open_kong_available(&melds[cur_player], t);
        if (k)
        {
            *k = MJ_KONG_TRIPLE(*k);
        }
        else
        {
            mj_discard_tile(&hands[cur_player], t^1);
            mj_discard_tile(&hands[cur_player], t^2);
            mj_discard_tile(&hands[cur_player], t^3);
            mj_add_meld(&melds[cur_player], MJ_KONG_TRIPLE(MJ_TRIPLE(
                t, t^1, t^2)));
        }
    }

    /**
     * Completes a payment instructed by the server.
     */
    void payment()
    {
        int player = msg::data<int>(buf);
        buf = interface.recv();
        if (msg::type(buf)!=msg::header::this_many_points)
            invalid_msg();
        scores[player] += msg::data<int>(buf);
#ifdef MJ_CLIENT_MODE_CLI
        std::cout << "Player " << player << " won " << msg::data<int>(buf) <<
            " points.\n";
#endif
    }

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
    void print_state() const
    {
        for (int i = 0; i < 4; ++i)
        {
            std::cout << "Player " << i << (i==my_pos ? " [YOU]:\n" : ":\n");
            if (i == my_pos)
                std::cout << " 1  2  3  4  5  6  7  8  9 10 11 12 13 14\n";
            mj_print_hand(hands[i]);
            if (i == my_pos && cur_tile != MJ_INVALID_TILE)
            {
                for (int p = 0; p < hands[my_pos].size && hands[my_pos].tiles[p] != cur_tile; ++p)
                    std::cout << "   ";
                std::cout << " !\n";
            }

            mj_print_meld(melds[i]);
            std::cout << '\n';
            std::cout << "Discards: ";
            for (auto &t : discards[i])
            {
                mj_print_tile(t);
            }
            std::cout << "\n----------------------------------------\n";
        }
    }

    static void inline sep()
    {
        std::cout << "*******************************************\n";
    }

    void print_kong_options()
    {
        mj_size quads = mj_self_kongs(hands[my_pos], melds[my_pos], c_quads.data());
        for (mj_size i = 0; i < quads; ++i)
        {
            std::cout << static_cast<char>('K' + i) << ") Call Kong with";
            mj_print_tile(c_quads[i]);
            std::cout << '\n';
        }
    }
#endif

private:
    void command(std::istream &in)
    {
        std::string cmd;
        while (cmd != "quit")
        {
            in >> cmd;

            std::scoped_lock l(class_write_mutex);
            switch (cmd[0])
            {
            case 'p':
                interface.send(msg::header::pass_calls);
                DEBUG_PRINT("Tried to pass calls.\n");
                break;
            case 'R':
                interface.send(msg::header::call_ron);
                DEBUG_PRINT("Tried to call Ron\n");
                break;
            case 'T':
                interface.send(msg::header::call_tsumo);
                DEBUG_PRINT("Tried to call Tsumo\n");
                break;
            case 'r':
                interface.send(msg::header::call_riichi);
                DEBUG_PRINT("Tried to call Riichi\n");
                break;
            case 'P':
            {
                mj_pair pong_tiles;
                if (mj_pong_available(hands[my_pos], cur_tile, &pong_tiles))
                {
                    interface.send(msg::header::call_pong);
                    interface.send(msg::header::call_with_tile, MJ_FIRST(pong_tiles));
                    interface.send(msg::header::call_with_tile, MJ_SECOND(pong_tiles));
                    DEBUG_PRINT("Tried to call Pong\n");
                }
                else
                    DEBUG_PRINT("No pong available\n");
                break;
            }
            case 'c'...'l':
            {
                int c = cmd[0] - 'c';
                if (c >= 0)
                {
                    interface.send(msg::header::call_chow);
                    interface.send(msg::header::call_with_tile, MJ_FIRST(c_pairs[c]));
                    interface.send(msg::header::call_with_tile, MJ_SECOND(c_pairs[c]));
                    DEBUG_PRINT("Tried to call Chow\n");
                }
                else
                    DEBUG_PRINT("Invalid Chow\n");
                break;
            }
            case 'K' ... 'N':
            {
                mj_triple kong_tiles;
                if (cur_player != my_pos && mj_kong_available(hands[my_pos], cur_tile, &kong_tiles))
                {
                    interface.send(msg::header::call_kong);
                    interface.send(msg::header::call_with_tile, MJ_FIRST(kong_tiles));
                    interface.send(msg::header::call_with_tile, MJ_SECOND(kong_tiles));
                    interface.send(msg::header::call_with_tile, MJ_THIRD(kong_tiles));
                    DEBUG_PRINT("Tried to call Kong\n");
                }
                else if (cur_player == my_pos)
                {
                    int c = cmd[0] - 'K';
                    if (c >= 0)
                    {
                        interface.send(msg::header::call_kong);
                        interface.send(msg::header::call_with_tile, c_quads[c]);
                        DEBUG_PRINT("Tried to call Kong\n");
                    }
                    else
                        DEBUG_PRINT("Invalid Kong\n");
                }
                else
                    DEBUG_PRINT("No kong available\n");
                break;
            }
            case 'G':
                interface.send(msg::header::discard_tile, cur_tile);
                DEBUG_PRINT("Tried to tsumogiri\n");
                break;
            case '0' ... '9':
            {
                int t = std::stoi(cmd)-1;
                if (t < 0 || t >= hands[my_pos].size)
                {
                    std::cout << "Invalid tile\n";
                    break;
                }
                interface.send(msg::header::discard_tile,
                    hands[my_pos].tiles[t]);
                DEBUG_PRINT("Tried to discard " << cmd << '\n');
                break;
            }
            default:
                std::cout << "Invalid command. Use \"quit\" to quit.\n";
            }
        }
    }
};

#endif
