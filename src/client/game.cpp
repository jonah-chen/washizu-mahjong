#include "game.hpp"
#include <sstream>

static void inline sep()
{
    std::cout << "----------------------------------------\n";
}

game::game()
{
    buf = interface.recv();

    if (msg::type(buf)!=msg::header::your_id)
    {
        throw server_exception("Could not acquire id from server.");
    }
    my_uid = msg::data<msg::id_type>(buf);

    interface.send(msg::header::join_as_player, msg::NEW_PLAYER);
    interface.send(msg::header::my_id, my_uid);

    discard_pile.reserve(24);

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
    start_round();
}

void game::start_round()
{
    buf = interface.recv();
    if (msg::type(buf)!=msg::header::new_round)
    {
        std::cerr << "Cannot start round.\n";
        invalid_msg();
    }
    prevailing_wind = msg::data<unsigned short>(buf) >> 2;
    round_no = msg::data<unsigned short>(buf) & 3;
    seat_wind = (4 + my_pos - round_no) & 3;

    for (auto &hand : hands)
        mj_empty_hand(&hand);
    
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
                throw 0;
            mj_add_tile(&hands[p],t);
        }
    }
}

void game::draw(int player)
{
    buf = interface.recv();
    if (msg::type(buf)!=msg::header::tile)
    {
        std::cerr << "You cannot draw something that is not a tile.\n";
        invalid_msg();
    }
    mj_tile t = msg::data<mj_tile>(buf);

}

void game::turn()
{
    buf = interface.recv();
    switch(msg::type(buf))
    {
    case msg::header::this_player_drew:
        draw(msg::data<int>(buf));
    case msg::header::tile:
        std::cout << "Player " << cur_player << " discarded ";
        mj_print_tile(msg::data<mj_tile>(buf));
        std::cout << '\n';

    case msg::header::tsumogiri_tile:



    default:
        break;
    }
}

void game::invalid_msg() const
{
    std::stringstream ss;
    ss << "Unexpected message from server " << 
        static_cast<char>(msg::type(buf)) << " " << 
        msg::data<unsigned short>(buf) << ".";
    throw server_exception(ss.str());
}
