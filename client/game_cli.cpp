#define MJ_CLIENT_MODE_CLI
#include "game.hpp"
static void inline sep()
{
    std::cout << "*******************************************\n";
}

void game::start_round_update()
{
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

    mj_print_tile(doras.front());
    std::cout << std::endl;

    if (seat_wind != MJ_EAST)
    {
        sep();
        print_state();
    }

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

    cur_tile = msg::data<mj_tile>(buf);
    mj_add_tile(&hands[player], cur_tile);

    sep();

    std::cout << "Player " << player << " drew ";
    mj_print_tile(cur_tile);
    std::cout << '\n';

    cur_player = player;
    if (cur_player == my_pos)
        after_draw();
}

void game::after_draw()
{
    print_state();

    std::scoped_lock l(class_write_mutex);
    if (in_riichi)
    {

        std::cout << "You are in riichi:\nT) Tsumo\n";
        print_kong_options();
        std::cout << "G) Tsumogiri\n";

        return;
    }
    if (mj_tenpai(hands[my_pos], melds[my_pos], nullptr))
    {

        std::cout << "You are tenpai.\n";
        std::cout << "T) Tsumo\n";
        std::cout << "r) Riichi\n";
        print_kong_options();

    }
    else
        interface.send(msg::header::pass_calls);


    std::cout << "1-" << hands[my_pos].size << ") Discard Tile\n";

}

void game::discard()
{
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
}
void game::check_calls()
{
    mj_size ron = mj_tenpai(hands[my_pos], melds[my_pos], nullptr);

    mj_bool pong = mj_pong_available(hands[my_pos], cur_tile, nullptr);
    mj_bool kong = mj_kong_available(hands[my_pos], cur_tile, nullptr);
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

void game::player_pong()
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
}

void game::player_kong()
{
    cur_player = msg::data<int>(buf);
    std::cout << "Player " << cur_player << " called KONG\n";

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
        print_state();
        std::cout << "1-" << hands[my_pos].size << ": Discard Tile\n";
    }
}

void game::self_kong()
{
    std::cout << "Player " << cur_player << " called KONG\n";
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

void game::payment()
{
    int player = msg::data<int>(buf);
    buf = interface.recv();
    if (msg::type(buf)!=msg::header::this_many_points)
        invalid_msg();
    scores[player] += msg::data<signed short>(buf);

    std::cout << "Player " << player << " won " << msg::data<signed short>(buf) <<
        " points.\n";
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

void game::print_kong_options()
{
    mj_size quads = mj_self_kongs(hands[my_pos], melds[my_pos], c_quads.data());
    for (mj_size i = 0; i < quads; ++i)
    {
        std::cout << static_cast<char>('K' + i) << ") Call Kong with";
        mj_print_tile(c_quads[i]);
        std::cout << '\n';
    }
}
