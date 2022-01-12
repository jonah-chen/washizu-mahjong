#include "game.hpp"
#include "mahjong/interaction.h"
#include "mahjong/yaku.h"
#include <algorithm>
#include <chrono>

game::game(io_type &io, players_type &&players, bool heads_up)
    : players(std::move(players))
{
    reshuffle();

    ping_thread = std::thread(ping);
    ping_thread.detach();
}

inline void game::accept_spectator(protocall::socket &&socket)
{
    spectators.emplace_back(std::move(socket));
}

void game::ping()
{
    using namespace std::chrono_literals;

    auto ping_fn = [](client_type &client) {
        if (client.socket.is_open())
        {
            char buffer[10];
            client.send(asio::buffer(";", 1));
            if (_timeout(1s, [&client,buffer](){
                client.recv(asio::buffer(buffer, 10));
                return false;
            }, true))
            {
                client.socket.close();
            }
        }
    };

    for (auto &client : players)
    {
        std::thread ping_thread(ping_fn, std::ref(client));
        ping_thread.detach();
    }
    for (auto &client : spectators)
    {
        std::thread ping_thread(ping_fn, std::ref(client));
        ping_thread.detach();
    }
}

void log(std::ostream &os, const std::string &msg)
{
    os << msg << std::endl;
}

// void game::player_io(int player)
// {
//     using namespace std::chrono_literals;

//     auto &p_hand = hands[player];
//     auto &p_melds = melds[player];
//     auto &p_discards = discards[player];
//     mj_meld result[64];
//     mj_pair pairs[16];


//     // need to check for ron or tsumo
//     // add the tile to the hand, while preserving a copy
//     auto p_hand_cpy = p_hand;
//     mj_add_tile(&p_hand_cpy, cur_tile);



//     if (cur_player == player)
//     {
        
//         if (mj_kong_available(p_hand, cur_tile))
//         {
//             _timeout(15s, call, call_type::pass);
//         }
//     }

//     else
//     {
//         // check for ron
//         if (std::find_if(p_discards.begin(),p_discards.end(),
//         [this](card_type &discarded_tile) 
//         {
//             return MJ_ID_128(cur_tile) == MJ_ID_128(discarded_tile);
//         }) == p_discards.end())
//         {
//             // check if hand has yaku
            
//         }
//         else {} // player is in furiten

//     }
// }

// void game::broadcast()
// {
//     for (auto &player : players)
//     {
//         send(player, "pin");
//     }

//     for (auto &spectator : spectators)
//     {
//         asio::write(spectator, asio::buffer("asdf"));
//     }


// }

// void game::reshuffle()
// {
//     for (auto &p_hand : hands)
//         mj_empty_hand(&p_hand);

//     for (auto &p_melds : melds)
//         mj_empty_melds(&p_melds);
    
//     for (auto &p_discards : discards)
//         p_discards.clear();
    
//     cur_tile = MJ_INVALID_TILE;

//     turn_state = turn_state::discard;
// }

// game::call_type game::call() const
// {

// }

