#include "game.hpp"
#include "mahjong/interaction.h"
#include "mahjong/yaku.h"
#include <algorithm>
#include <chrono>

game::game(std::ostream &server_log, std::ostream &game_log, players_type &&players, bool heads_up)
    : server_log(server_log), game_log(game_log), players(std::move(players))
{
    for (int pos = 0; pos < NUM_PLAYERS; ++pos)
        players[pos].send(msg::header::your_position, pos);

    ping_thread = std::thread(&game::ping, this);
    ping_thread.detach();

    main_thread = std::thread(&game::play, this);
    main_thread.detach();
}

inline void game::accept_spectator(protocall::socket &&socket)
{
    std::scoped_lock lock(spectator_mutex);
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

    rng_mutex.lock();
    unsigned short lucky_number = wall.tiger();
    rng_mutex.unlock();

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
        spectators.remove_if([](const client_type &client){
            return !client.socket.is_open();
        });

        for (auto &client : players)
        {
            std::thread ping_thread(&game::ping_client, this, std::ref(client));
            ping_thread.detach();
        }

        spectator_mutex.lock();
        for (auto &client : spectators)
        {
            std::thread ping_thread(&game::ping_client, this, std::ref(client));
            ping_thread.detach();
        }
        spectator_mutex.unlock();
    }
}

void game::log(std::ostream &os, const std::string &msg)
{
    os << msg << '\n';
}

/* helpers */
void game::draw()
{
    rng_mutex.lock();
    auto tile = wall();
    rng_mutex.unlock();

    if (tile == MJ_INVALID_TILE)
    {
        cur_state = state_type::exhaustive_draw;
        return;
    }

    if (wall.size() < MJ_DECK_SIZE - MJ_DEAD_WALL_SIZE - NUM_PLAYERS)
        game_flags &= ~FIRST_TURN_FLAG;

    mj_add_tile(&hands[cur_player], tile);

    flags[cur_player] &= ~IPPATSU_FLAG;

    broadcast(msg::header::this_player_drew, cur_player);
    if (MJ_IS_OPAQUE(tile))
    {
        broadcast(msg::header::tile, MJ_INVALID_TILE, true);
        players[cur_player].send(msg::header::tile, tile);
    }
    else
        broadcast(msg::header::tile, tile);
    
    cur_tile = tile;
}

void game::new_dora()
{
    rng_mutex.lock();
    dora_tiles.push_back(wall.draw_dora());
    rng_mutex.unlock();

    broadcast(msg::header::dora_indicator, dora_tiles.back());
}

void game::payment(int player, score_type score)
{
    scores[player] += score;
    broadcast(msg::header::this_player_won, player);
    broadcast(msg::header::this_many_points, score);
}

bool game::self_call_kong()
{
    auto *kong = mj_open_kong_available(melds[cur_player], cur_tile);

    if (kong)
    {
        *kong = MJ_KONG_TRIPLE(*kong);
        mj_discard_tile(&hands[cur_player], cur_tile);
        game_flags |= OTHER_KONG_FLAG;
    }
    else if (mj_closed_kong_available(hands[cur_player], cur_tile))
    {
        mj_discard_tile(&hands[cur_player], cur_tile);
        mj_discard_tile(&hands[cur_player], cur_tile^0b01);
        mj_discard_tile(&hands[cur_player], cur_tile^0b10);
        mj_discard_tile(&hands[cur_player], cur_tile^0b11);
        mj_add_meld(&melds[cur_player], MJ_KONG_TRIPLE(MJ_TRIPLE(cur_tile,cur_player^0b01,cur_player^0b10)));
        game_flags |= CLOSED_KONG_FLAG;
    }
    else
    {
        players[cur_player].send(msg::header::reject, msg::REJECT);
        return false;
    }
    broadcast(msg::header::this_player_kong, cur_player);
    return true;
}

game::state_type game::call_tsumo()
{
    broadcast(msg::header::this_player_tsumo, cur_player);
    broadcast(msg::header::this_player_hand, cur_player);
    broadcast(msg::header::closed_hand, msg::START_STREAM);
    for (auto *tile = hands[cur_player].tiles; 
        tile < hands[cur_player].tiles+hands[cur_player].size; ++tile)
        broadcast(msg::header::tile, *tile);
    broadcast(msg::header::closed_hand, msg::END_STREAM);

    unsigned short yakus[MJ_YAKU_ARR_SIZE];
    int fu, fan;
    int seat_wind = (NUM_PLAYERS+cur_player-dealer)%NUM_PLAYERS;

    memset(yakus, 0, sizeof(yakus));
    yakus[MJ_YAKU_RICHII] = flags[cur_player] & (DOUBLE_RIICHI_FLAG | RIICHI_FLAG);
    yakus[MJ_YAKU_IPPATSU] = (flags[cur_player] & IPPATSU_FLAG) ? 1 : 0;
    yakus[MJ_YAKU_RINSHAN] = (game_flags & KONG_FLAG) ? 1 : 0;
    yakus[MJ_YAKU_HAITEI] = wall.size() ? 0 : 1;

    int score = mj_score(&fu, &fan, yakus, &hands[cur_player], 
        &melds[cur_player], cur_tile, MJ_TRUE, prevailing_wind, seat_wind);
    
    if (score)
    {
        broadcast(msg::header::fu_count, fu);
        broadcast(msg::header::yaku_list, msg::START_STREAM);
        for (int i = 0; i < MJ_YAKU_ARR_SIZE; ++i)
        {
            if (yakus[i])
            {
                broadcast(msg::header::winning_yaku, i);
                broadcast(msg::header::yaku_fan_count, yakus[i]);
            }
        }
        broadcast(msg::header::yaku_list, msg::END_STREAM);

        score += bonus_score;
        if (cur_player == dealer)
        {
            for (int p = 0; p < NUM_PLAYERS; ++p)
            {
                if (p == cur_player)
                    payment(p, 6*score+deposit);
                else
                    payment(p,-2*score);
            }
            return state_type::renchan;
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
            return state_type::next;
        }
    }
    else
    {
        return state_type::chombo;
    }
}

/* playing */
void game::play()
{
    while (true)
    {
        switch (cur_state)
        {
        case state_type::game_over:
            return;
        case state_type::start_round:
            start_round();
            break;
        case state_type::draw:
            draw();
        case state_type::self_call:
            cur_state = _timeout(SELF_CALL_TIMEOUT, this, &game::self_call, state_type::tsumogiri);
            break;
        case state_type::discard:
            cur_state = _timeout(DISCARD_TIMEOUT, this, &game::discard, state_type::tsumogiri);
            break;
        case state_type::opponent_call:
            cur_state = _timeout(OPPONENT_CALL_TIMEOUT, this, &game::opponent_call, state_type::tsumogiri);
            break;
        case state_type::after_kong:
            new_dora();
            cur_state = state_type::draw;
            break;
        case state_type::next:
            next();
            break;
        case state_type::renchan:
            renchan();
            break;
        case state_type::exhaustive_draw:
            exhaustive_draw();
            break;
        case state_type::tsumogiri:
            tsumogiri();
            break;
        case state_type::chombo:
            chombo_penalty();
            break;
        default:
            break;
        }
    }
}

void game::start_round()
{
    if (prevailing_wind == MJ_WEST)
    {
        cur_state = state_type::game_over;
        return;
    }

    for (auto &p_hand : hands)
        mj_empty_hand(&p_hand);

    for (auto &p_melds : melds)
        mj_empty_melds(&p_melds);
    
    for (auto &p_discards : discards)
        p_discards.clear();
    
    cur_tile = MJ_INVALID_TILE;

    rng_mutex.lock();
    wall.reset();
    rng_mutex.unlock();

    cur_player = dealer;
    for (int i = 0; i < NUM_PLAYERS; ++i)
    {
        for (int j = 0; j < 13; ++j)
            draw();
        cur_player = (cur_player + 1) % NUM_PLAYERS;   
    }

    first_turn = true;

    cur_state = state_type::draw;
}

game::state_type game::self_call()
{
    msg::buffer buffer;

    while(true) /* We allow retrys until timeout */
    {
        players[cur_player].recv(buffer, msg::BUFFER_SIZE);
        msg::header ty = msg::type(buffer);
        switch (ty)
        {
        case msg::header::call_kong:
            if (self_call_kong())
                return state_type::after_kong;
            else
            {
                players[cur_player].send(msg::header::reject, msg::REJECT);
                log(server_log, "Player with UID=" + std::to_string(players[cur_player].uid) + " @" + 
                    players[cur_player].socket.remote_endpoint().address().to_string() + " called an invalid kong.");
                break; /* from switch, try again */
            }

        case msg::header::call_tsumo:
            return call_tsumo();

        case msg::header::call_riichi:
            broadcast(msg::header::this_player_riichi, cur_player);
            payment(cur_player, -MJ_RIICHI_DEPOSIT);
            deposit += MJ_RIICHI_DEPOSIT;
            flags[cur_player] |= RIICHI_FLAG;
            return state_type::discard;

        case msg::header::discard_tile:
            auto discarded = msg::data<card_type>(buffer);
            if (mj_discard_tile(&hands[cur_player], discarded))
            {
                broadcast(msg::header::tile, discarded);
                cur_tile = discarded;
                return state_type::opponent_call;
            }
            else
            {
                players[cur_player].send(msg::header::reject, msg::REJECT);
                break; /* from switch, try again */
            }
        }
    }
}

game::state_type game::discard()
{
    msg::buffer buffer;
    auto discarded = msg::data<card_type>(buffer);

    game_flags &= ~KONG_FLAG;

    if (msg::type(buffer) == msg::header::discard_tile && 
        mj_discard_tile(&hands[cur_player], discarded))
    {
        if (discarded != cur_tile && flags[cur_player] & (DOUBLE_RIICHI_FLAG | RIICHI_FLAG))
            return state_type::chombo;
        broadcast(msg::header::tile, discarded);
        cur_tile = discarded;
        return state_type::opponent_call;
    }
    else
    {
        players[cur_player].send(msg::header::reject, msg::REJECT);
        log(server_log, "Player with UID=" + std::to_string(players[cur_player].uid) + " @" + 
            players[cur_player].socket.remote_endpoint().address().to_string() + " discarded an invalid tile.");
        return state_type::tsumogiri;
    }
}

game::state_type game::opponent_call() 
{
    // ron first
    std::array<score_type, NUM_PLAYERS> score_if_ron;
    std::array<std::array<unsigned short, MJ_YAKU_ARR_SIZE>, NUM_PLAYERS> yakus_if_ron {};

    for (int p = 0; p < NUM_PLAYERS; ++p)
    {
    // iterate over all player
        if (p == cur_player)
            continue;
    // if player is in furiten, continue because cannot ron
        if (std::find_if(discards[p].begin(), discards[p].end(), 
            [&](const card_type &tile) { 
                return MJ_ID_128(tile) == MJ_ID_128(cur_tile); 
            }) != discards[p].end()) continue;
    
    // create temp hand and add the ron tile to it
        mj_hand tmp_hand;
        memcpy(&tmp_hand, &hands[p], sizeof(mj_hand));
        tmp_hand.tiles[tmp_hand.size++] = cur_tile;
        mj_sort_hand(&tmp_hand);

    // check if it can win
        int fu, fan;
        int seat_wind = (NUM_PLAYERS+p-dealer)%NUM_PLAYERS;

        yakus_if_ron[p][MJ_YAKU_RICHII] = flags[p] & (DOUBLE_RIICHI_FLAG | RIICHI_FLAG);
        yakus_if_ron[p][MJ_YAKU_IPPATSU] = (flags[p] & IPPATSU_FLAG) ? 1 : 0;
        yakus_if_ron[p][MJ_YAKU_CHANKAN] = (game_flags & KONG_FLAG) ? 1 : 0;
        yakus_if_ron[p][MJ_YAKU_HOUTEI] = wall.size() ? 0 : 1;

        score_if_ron[p] = mj_score(&fu, &fan, yakus_if_ron[p].data(), &tmp_hand, 
            &melds[p], cur_tile, MJ_FALSE, prevailing_wind, seat_wind);
    }

    // sort the players by priority. The player next to play is highest priority.
    std::array<int, NUM_PLAYERS-1> priority_order = {
        static_cast<int>((cur_player+1)%NUM_PLAYERS),
        static_cast<int>((cur_player+2)%NUM_PLAYERS),
        static_cast<int>((cur_player+3)%NUM_PLAYERS)
    };



    if (1 /* there was a call */)
    for (auto &p_flags : flags)
        p_flags &= ~IPPATSU_FLAG;
} // TODO: implement this.

void game::next()
{
    bonus_score = 0;
    deposit = 0;
    if (++dealer == NUM_PLAYERS)
    {
        dealer = 0;
        ++prevailing_wind;
    }
    cur_state = state_type::start_round;
}

void game::renchan()
{
    bonus_score += MJ_BONUS_SCORE;
    cur_state = state_type::start_round;
}

void game::exhaustive_draw()
{
    std::array<bool, NUM_PLAYERS> tenpai;
    int players_tenpai = 0;
    for (int p = 0; p < NUM_PLAYERS; ++p)
    {
        if (mj_tenpai(hands[p], melds[p], nullptr))
        {
            tenpai[p] = true;
            ++players_tenpai;
        }
        else
        {
            tenpai[p] = false;
            if (flags[p] & (DOUBLE_RIICHI_FLAG | RIICHI_FLAG))
            {
                cur_player = p;
                cur_state = state_type::chombo;
                return;
            }
        }
    }

    switch(players_tenpai)
    {
    case 1:
        for (int p = 0; p < NUM_PLAYERS; ++p)
            payment(p, tenpai[p] ? 3000 : -1000);
        break;
    case 2:
        for (int p = 0; p < NUM_PLAYERS; ++p)
            payment(p, tenpai[p] ? 1500 : -1500);
        break;
    case 3:
        for (int p = 0; p < NUM_PLAYERS; ++p)
            payment(p, tenpai[p] ? 1000 : -3000);
        break;
    }

    if (tenpai[dealer])
        cur_state = state_type::renchan;
    else
    {
        if (++dealer == NUM_PLAYERS)
        {
            dealer = 0;
            ++prevailing_wind;
        }
        bonus_score += MJ_BONUS_SCORE;
        cur_state = state_type::start_round;
    }
}

/* Penalty */
void game::tsumogiri()
{
    if (mj_discard_tile(&hands[cur_player], cur_tile))
        broadcast(msg::header::tile, cur_tile);
    cur_state = state_type::opponent_call;
}

void game::chombo_penalty()
{
    if (game_flags & HEADS_UP_FLAG)
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

    cur_state = state_type::start_round;
}

template<typename SocketType>
typename game_client<SocketType>::id_type game_client<SocketType>::_counter = 8000;


int main()
{
    printf("Hello World!\n");
}
