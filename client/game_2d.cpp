#define MJ_CLIENT_MODE_2D
#include "renderer/2d.hpp"
#include "game.hpp"

void game::resubmit() const
{
    renderer2d::clear();
    for (int i = 0; i < NUM_PLAYERS; ++i)
        renderer2d::submit(hands[i], (i - my_pos) & 3);
    for (int i = 0; i < NUM_PLAYERS; ++i)
        renderer2d::submit(discards[i], (i - my_pos) & 3);

    glfwPostEmptyEvent();
}

void game::start_round_update()
{
    renderer2d::clear();
    for (int i = 0; i < NUM_PLAYERS; ++i)
        renderer2d::submit(hands[i], (i - my_pos) & 3);
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

    cur_player = player;

    resubmit();
}

void game::discard()
{
    cur_tile = msg::data<mj_tile>(buf);

    mj_discard_tile(&hands[cur_player], cur_tile);
    discards[cur_player].push_back(cur_tile);
    if (cur_player != my_pos)
        check_calls();

    resubmit();
}

void game::check_calls()
{}

void game::player_pong()
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

    resubmit();
}

void game::player_kong()
{
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

    resubmit();
}

void game::self_kong()
{
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

    resubmit();
}

void game::payment()
{
    int player = msg::data<int>(buf);
    buf = interface.recv();
    if (msg::type(buf)!=msg::header::this_many_points)
        invalid_msg();
    scores[player] += msg::data<int>(buf);

    resubmit();
}


