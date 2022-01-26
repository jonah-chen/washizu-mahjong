#define MJ_CLIENT_MODE_CLI
#include "game.hpp"

void game::connect_to_server()
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
            break;
        }
    }
    std::fill(scores.begin(), scores.end(), STARTING_PTS);
}

void game::start_round()
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

    cur_player = round_no;
    in_riichi = false;

    start_round_update();
}


void game::command(std::function<void(std::string&)> const &get)
{
    std::string cmd;
    while (cmd != "quit")
    {
        get(cmd);

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

bool game::turn()
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
        discard();
        break;
    case msg::header::this_player_pong: case msg::header::this_player_chow:
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