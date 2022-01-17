#include "receiver.hpp"
#include "mahjong/mahjong.h"
#include "mahjong/interaction.h"
#include <iostream>
int main()
{
    R interface(R::protocall::v4(), 10000);
    
    // send a connection request
    msg::buffer msg = interface.recv();
    std::cout << static_cast<char>(msg::type(msg)) << " " << msg::data<unsigned short>(msg) << std::endl;
    interface.send(msg::header::join_as_player, msg::NEW_PLAYER);
    interface.send(msg::header::my_id, msg::data<unsigned short>(msg));

    msg::header h;
    int pos;
    interface.recv(h, pos);
    while (h==msg::header::queue_size)
    {
        std::cout << "You are in the queue with " << pos << "/4 players.\n";
        interface.recv(h, pos);
    }
    if (h==msg::header::your_position)
    {
        std::cout << "You are player " << pos <<
        ". You are " << (pos ? "not" : "") << " the dealer.\n";
    }

    std::array<mj_hand,4> hands;
    for (auto &hand : hands)
        mj_empty_hand(&hand);
    
    mj_tile t;
    int p;
    int cur_player = 0;

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

    while (1) // draw initial hand
    {
        interface.recv(h, t);
        if (h==msg::header::this_player_drew)
        {
            p = t;

            interface.recv(h,t);
            if (h==msg::header::tile)
            {
                mj_add_tile(&hands[p], t);
                std::cout << "Player " << p << " drew ";
                mj_print_tile(t);
                std::cout << '\n';

                if (cur_player == pos)
                {
                    std::cout << "It is your turn. Please choose tile to discard\n";
                    for (int i = 0; i < 4; ++i)
                    {
                        std::cout << "Player " << i << (i==pos ? "[YOU]:\n" : ":\n");
                        if (i==pos)
                        {
                            std::cout << " 0  1  2  3  4  5  6  7  8  9 10 11 12 13\n";
                        }
                        mj_print_hand(hands[i]);
                        std::cout << "\n";
                    }
                    int choice;
                    interface.send(msg::header::pass_calls, msg::NO_INFO);
                    while (true)
                    {
                        std::cin >> choice;
                        mj_tile td = hands[pos].tiles[choice];
                        if (mj_discard_tile(&hands[pos], td))
                        {
                            std::cout << "you try to discard " << choice << "\n";
                            mj_add_tile(&hands[pos], td);
                            interface.send(msg::header::discard_tile, td);
                            break;
                        }
                        else
                            std::cout << "Choice is invalid, try again\n";
                    }
                }
            }
            else
            {
                std::cout << static_cast<char>(h) << " " << static_cast<int>(t) << std::endl;
            }
        }
        else if (h==msg::header::tile)
        {
            if (mj_discard_tile(&hands[cur_player], t))
            {

            }
            else
            {
                hands[cur_player].size--; // discard opq tile
            }
            std::cout << "Player " << cur_player << " discarded ";
            mj_print_tile(t);
            std::cout << '\n';

            cur_player = (cur_player + 1) % 4;
        }
        else
        {
            std::cout << static_cast<char>(h) << " " << static_cast<int>(t) << std::endl;
        }
    }
}