#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) std::cout << x
#else 
#define DEBUG_PRINT(x)
#endif 

#include "game.hpp"
#include "mahjong/yaku.h"
#include <sstream>

static void inline sep()
{
    std::cout << "*******************************************\n";
}

game::game()
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
            std::cout << "You are in the queue with " << my_pos << "/4 players.\n";
        else if (msg::type(buf)!=msg::header::your_position)
            invalid_msg();
        else
        {
            std::cout << "You are player " << my_pos << ". You are " << 
                (my_pos ? "not" : "") << " the dealer.\n";
            break;
        }
    }

    std::fill(scores.begin(), scores.end(), STARTING_PTS);

    cmd_thread = std::thread(&game::command, this);
    cmd_thread.detach();
}

void game::start_round()
{
    prevailing_wind = msg::data<unsigned short>(buf) >> 2;
    round_no = msg::data<unsigned short>(buf) & 3;
    seat_wind = (my_pos - round_no) & 3;

    for (auto &hand : hands)
        mj_empty_hand(&hand);
    for (auto &discards : discards)
        discards.clear();
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
    sep();
    std::cout << "Scores:";
    for (auto &score : scores)
        std::cout << ' ' << score;
    std::cout << "\nDora indicator: ";

    mj_print_tile(t);
    std::cout << std::endl;

    cur_player = round_no;
    in_riichi = false;
}

void game::draw()
{
    int player = msg::data<int>(buf);
    buf = interface.recv();
    if (msg::type(buf)!=msg::header::tile)
    {
        std::cerr << "You cannot draw something that is not a tile.\n";
        invalid_msg();
    }

    sep();

    cur_tile = msg::data<mj_tile>(buf);
    mj_add_tile(&hands[player], cur_tile);
    std::cout << "Player " << player << " drew ";
    mj_print_tile(cur_tile);
    std::cout << '\n';    
    cur_player = player;
    if (cur_player == my_pos)
        after_draw();
}

void game::turn()
{
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
    {
        cur_tile = msg::data<mj_tile>(buf);
        std::cout << "Player " << cur_player << 
            (msg::type(buf)==msg::header::tsumogiri_tile ? 
            " tsumogiri " : " discarded ");
        mj_print_tile(cur_tile);
        std::cout << '\n';

        mj_discard_tile(&hands[cur_player], cur_tile);
        discards[cur_player].push_back(cur_tile);
        if (cur_player != my_pos)
            check_calls();
        break;
    }
    case msg::header::this_player_pong: case msg::header::this_player_chow:
    {
        cur_player = msg::data<int>(buf);
        std::cout << "Player " << cur_player << " called " << 
            (msg::type(buf)==msg::header::this_player_pong ? "PONG\n" : "CHOW\n");
        
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
        
        if (cur_player == my_pos)
        {
            print_state();
            std::cout << "1-" << hands[my_pos].size << ": Discard Tile\n";
        }
        break;
    }
    case msg::header::this_player_kong:
    {
        if (cur_player == msg::data<int>(buf)) // self call kong
        {
            std::cout << "Player " << cur_player << " called KONG\n";
            buf = interface.recv();
            if (msg::type(buf)!=msg::header::tile)
            {
                std::cerr << "You cannot call something that is not a tile.\n";
                invalid_msg();
            }
            mj_tile t = msg::data<mj_tile>(buf);
            auto *k = mj_open_kong_available(melds[cur_player], t);
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
        else // not self call kong
        {
            cur_player = msg::data<int>(buf);
            std::cout << "Player " << cur_player << " called KONG\n";
            buf = interface.recv();
            std::array<mj_tile, 3> meld_tiles;
            for (int i = 0; i < 3; ++i)
            {
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
                print_state();
                std::cout << "1-" << hands[my_pos].size << ": Discard Tile\n";
            }
        }
        break;
    }
    case msg::header::this_player_riichi:
        in_riichi = (cur_player == my_pos) ? true : in_riichi;
        std::cout << "Player " << cur_player << " called RIICHI\n";
        break;
    default:
        std::cout << "Unknown Message from server: " << static_cast<char>(msg::type(buf)) << 
            msg::data<unsigned short>(buf) << std::endl;
        break;
    }
}

void game::invalid_msg() const
{
    std::stringstream ss;
    while(1)
    ss << "Unexpected message from server " << 
        static_cast<char>(msg::type(buf)) << " " << 
        msg::data<unsigned short>(buf) << ".";
    throw server_exception(ss.str());
}

void game::check_calls()
{
    auto ron = mj_tenpai(hands[my_pos], melds[my_pos], nullptr);

    auto pong = mj_pong_available(hands[my_pos], cur_tile, nullptr);
    auto kong = mj_kong_available(hands[my_pos], cur_tile, nullptr);
    mj_size chow = (((my_pos - cur_player) & 3) == 1) ? 
        mj_chow_available(hands[my_pos], cur_tile, c_pairs.data()) : 0;

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
}

void game::after_draw()
{
    print_state();
    std::scoped_lock l(class_write_mutex);
    if (in_riichi)
    {
        std::cout << "You are in riichi:\nT) Tsumo\n";
        if (mj_closed_kong_available(hands[my_pos], cur_tile) || 
            mj_open_kong_available(melds[my_pos], cur_tile))
            std::cout << "K) Call Kong\n";
        std::cout << "G) Tsumogiri\n";
        return;
    }
    if (mj_tenpai(hands[my_pos], melds[my_pos], nullptr))
    {
        std::cout << "You are tenpai.\n";
        std::cout << "T) Tsumo\n";
        std::cout << "r) Riichi\n";
        if (mj_closed_kong_available(hands[my_pos], cur_tile) || 
            mj_open_kong_available(melds[my_pos], cur_tile))
            std::cout << "K) Call Kong\n";
    }
    else
        interface.send(msg::header::pass_calls);

    std::cout << "1-" << hands[my_pos].size << ") Discard Tile\n";
}

void game::print_state() const
{    
    for (int i = 0; i < 4; ++i)
    {
        std::cout << "Player " << i << (i==my_pos ? " [YOU]:\n" : ":\n");
        if (i == my_pos)
            std::cout << " 1  2  3  4  5  6  7  8  9 10 11 12 13 14\n";
        mj_print_hand(hands[i]);
        if (i == my_pos && cur_tile != MJ_INVALID_TILE)
        {
            for (int p = 0; hands[my_pos].tiles[p] != cur_tile && p < hands[my_pos].size; ++p)
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

void game::payment()
{
    int player = msg::data<int>(buf);
    buf = interface.recv();
    if (msg::type(buf)!=msg::header::this_many_points)
        invalid_msg();
    scores[player] += msg::data<int>(buf);
    std::cout << "Player " << player << " won " << msg::data<int>(buf) << " points.\n";
}

void game::command()
{
    std::unique_lock ul(class_write_mutex);
    DEBUG_PRINT("You can now use commands.\n");
    ul.unlock();
    std::string cmd;
    while (cmd != "quit")
    {
        std::cin >> cmd;

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
