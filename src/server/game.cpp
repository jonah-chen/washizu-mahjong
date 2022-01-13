#include "game.hpp"
#include "mahjong/interaction.h"
#include "mahjong/yaku.h"
#include <algorithm>
#include <chrono>

game::game(std::ostream &server_log, std::ostream &game_log, players_type &&players, bool heads_up)
    : server_log(server_log), game_log(game_log), players(std::move(players))
{
    reshuffle();

    for (int pos = 0; pos < NUM_PLAYERS; ++pos)
        players[pos].send(msg::header::your_position, pos);

    ping_thread = std::thread(ping);
    ping_thread.detach();
}

inline void game::accept_spectator(protocall::socket &&socket)
{
    spectators.emplace_back(std::move(socket));
}

void game::reconnect(unsigned short uid, protocall::socket &&socket)
{
    for (auto &player : players)
    {
        if (player.uid == uid && !player.socket.is_open())
        {
            player.socket = std::move(socket);
            log(server_log, "Player " + std::to_string(uid) + " reconnected.");
            return;
        }
    }
}

void game::ping_client(client_type &client)
{
    if (!client.socket.is_open())
        return;
    
    msg::buffer buffer;
    unsigned short lucky_number = wall.tiger();

    auto to_terminate = [&client,&buffer,lucky_number](){
        client.recv(buffer, msg::BUFFER_SIZE);
        return msg::type(buffer)!=msg::header::ping||
            msg::data<unsigned short>(buffer)!=lucky_number;
    };

    std::scoped_lock lock(client.mutex);

    client.send(msg::header::ping, lucky_number);

    if (_timeout(1000, to_terminate, true))
    {
        log(server_log, "Client with UID=" + std::to_string(client.uid) + " @" + 
            client.socket.remote_endpoint().address().to_string() + " timed out.");
        client.socket.close();
    }
}

void game::ping()
{
    while (true)
    {
        std::this_thread::sleep_for(PING_FREQ);

        // if spectators don't reconnect before the next ping, they are removed
        for (auto it = spectators.begin(); it != spectators.end();)
        {
            if (!it->socket.is_open())
                it = spectators.erase(it);
            else
                ++it;
        }

        for (auto &client : players)
        {
            std::thread ping_thread(ping_client, std::ref(client));
            ping_thread.detach();
        }
        for (auto &client : spectators)
        {
            std::thread ping_thread(ping_client, std::ref(client));
            ping_thread.detach();
        }
    }
}

void log(std::ostream &os, const std::string &msg)
{
    os << msg << '\n';
}

void game::play(turn_state_type state)
{
    while (true)
    {
        cur_player = dealer;
        for (int i = 0; i < NUM_PLAYERS; ++i)
        {
            for (int i = 0; i < 13; ++i)
                draw();
            cur_player = (cur_player + 1) % NUM_PLAYERS;   
        }

        new_dora();

        while(true)
        {
            draw();

            auto status = _timeout(15000, after_draw, turn_state_type::timeout);
            if (status == turn_state_type::timeout)
            {
            }
            else
            {
                play(status);
            }
        }

    }
}

void game::draw()
{
    auto tile = wall();
    mj_add_tile(&hands[cur_player], tile);

    broadcast(msg::header::this_player_drew, cur_player);
    if (MJ_IS_OPAQUE(tile))
    {
        broadcast(msg::header::tile, MJ_INVALID_TILE, false);
        players[cur_player].send(msg::header::tile, tile);
    }
    else
        broadcast(msg::header::tile, tile);
    
    cur_tile = tile;
}

game::turn_state_type game::after_draw()
{
    msg::buffer buffer;

    while (true)
    {
        players[cur_player].recv(buffer, msg::BUFFER_SIZE);
        msg::header ty = msg::type(buffer);

        switch (ty)
        {
        case msg::header::discard_tile:
            auto discarded = msg::data<card_type>(buffer);
            if (mj_discard_tile(&hands[cur_player], discarded))
            {
                broadcast(msg::header::tile, discarded);
            }
            else
            {
                players[cur_player].send(msg::header::reject, msg::REJECT);
                break;
            }
            return turn_state_type::wait_for_call;

        case msg::header::call_tsumo:
            // check if hand is tsumo able
            unsigned short yakus[MJ_YAKU_ARR_SIZE];
            int fu, fan;
            int seat_wind = (NUM_PLAYERS+cur_player-dealer)%NUM_PLAYERS;
            int score = mj_score(&fu, &fan, yakus, &hands[cur_player], &melds[cur_player], cur_tile, MJ_TRUE, prevailing_wind, seat_wind);
            if (score)
            {
                broadcast(msg::header::this_player_tsumo, cur_player);
                score += bonus_score;
                if (cur_player == dealer)
                {
                    for (int p = 0; p < NUM_PLAYERS; ++p)
                    {
                        if (p == cur_player)
                            payment(p, 6*score+deposit);
                        else
                            payment(p,-4*score);
                    }
                    end_round(true);
                }
                else
                {
                    for (int p = 0; p < NUM_PLAYERS; ++p)
                    {
                        if (p == cur_player)
                            payment(p, 4*score+deposit);
                        else if (p == dealer)
                            payment(p,-2*score);
                        else
                            payment(p,-score);
                    }
                    end_round(false);
                }
            }
            else
            {
                chombo_penalty();
            }
            return turn_state_type::abort;

        case msg::header::call_riichi:
            broadcast(msg::header::this_player_riichi, cur_player);
            payment(cur_player, -MJ_RIICHI_DEPOSIT);
            deposit += MJ_RIICHI_DEPOSIT;
            return turn_state_type::discard;

        case msg::header::call_kong:
            // check closed kong first
            auto *kong = mj_open_kong_available(melds[cur_player], cur_tile);

            if (kong)
            {
                MJ_KONG_TRIPLE(*kong);
                mj_discard_tile(&hands[cur_player], cur_tile);
            }
            else if (mj_closed_kong_available(hands[cur_player], cur_tile))
            {
                mj_discard_tile(&hands[cur_player], cur_tile);
                mj_discard_tile(&hands[cur_player], cur_tile^0b01);
                mj_discard_tile(&hands[cur_player], cur_tile^0b10);
                mj_discard_tile(&hands[cur_player], cur_tile^0b11);
                mj_add_meld(&melds[cur_player], MJ_KONG_TRIPLE(MJ_TRIPLE(cur_tile,cur_player^0b01,cur_player^0b10)));
            }
            else
            {
                players[cur_player].send(msg::header::reject, msg::REJECT);
                break;
            }
            broadcast(msg::header::this_player_kong, cur_player);
            
            new_dora();
            draw(); // rinshan draw
            
            return turn_state_type::discard;
        }
    }
}

void game::new_dora()
{
    dora_tiles.push_back(wall());
    broadcast(msg::header::dora_indicator, dora_tiles.back());
}

void game::chombo_penalty()
{
    if (heads_up)
    {
        payment(cur_player, -MJ_MANGAN*2);
        payment(cur_player^0b1, -MJ_MANGAN*2);
        payment(cur_player^0b10, MJ_MANGAN*2);
        payment(cur_player^0b11, MJ_MANGAN*2);
    }
    else if (cur_player == dealer)
    {
        for (int p = 0; p < NUM_PLAYERS; ++p)
        {
            if (p == cur_player)
                payment(p, -MJ_MANGAN*6);
            else
                payment(p, MJ_MANGAN*2);
        }
    }
    else
    {
        for (int p = 0; p < NUM_PLAYERS; ++p)
        {
            if (p==dealer)
                payment(p, MJ_MANGAN * 2);
            else if (p==cur_player)
                payment(p, -MJ_MANGAN * 4);
            else
                payment(p, MJ_MANGAN);
        }
    }

    end_round(true);
}

void game::payment(int player, score_type score)
{
    scores[player] += score;
    broadcast(msg::header::this_player_won, player);
    broadcast(msg::header::this_many_points, score);
}

void game::end_round(bool repeat)
{
    if (repeat)
        bonus_score += MJ_BONUS_SCORE;
    else
        bonus_score = 0;
    
    reshuffle();
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

void game::reshuffle()
{
    for (auto &p_hand : hands)
        mj_empty_hand(&p_hand);

    for (auto &p_melds : melds)
        mj_empty_melds(&p_melds);
    
    for (auto &p_discards : discards)
        p_discards.clear();
    
    cur_tile = MJ_INVALID_TILE;

    turn_state = turn_state::discard;

    wall.reset();
}

