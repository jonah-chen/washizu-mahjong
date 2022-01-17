#include "game.hpp"
#include "mahjong/interaction.h"
#include "mahjong/yaku.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

/**
 * The constructor:
 * Create the references for the server log, create the file for the game log.
 * Set the flags for the begining of the game and the unique ID for the game.
 * Reserve enough space (hopefully) to fit the vectors so they don't need to be
 * resized later.
 * Wait for enough players to join the game, while accepting new requests.
 * 
 * Once enough players join, the players are assigned seats randomly, and
 * the seats are sent to them.
 * 
 * Then, the game thread starts with the shuffling of the tiles and the initial
 * draws.
 */
game::game(unsigned short id, std::ostream &server_log, 
        std::string const &game_log_dir, bool heads_up)
        :   game_id(id), 
            server_log(server_log),
            game_log(game_log_dir),
            game_flags(heads_up ? HEADS_UP_FLAG : 0)
{
    if (game_log.fail())
        throw std::system_error(std::error_code(), "Cannot create game log file.");

    players.reserve(NUM_PLAYERS);
    for (auto &discard_pile : discards)
        discard_pile.reserve(20);

    while (players.size() < NUM_PLAYERS)
    {
        bool as_player; unsigned short g_id;
        auto new_player = std::make_unique<client_type>(messages, g_id, as_player);
        if (!new_player->is_open())
            continue;
        if (play)
        {
            if (g_id == msg::NEW_PLAYER)
                players.emplace_back(std::move(new_player));
            else if (games.find(g_id) != games.end())
                games.at(g_id).reconnect(std::move(new_player));
            else
                new_player->reject();
        }
        else
        {
            accept_spectator(std::move(new_player));
        }

        for (auto &player : players)
            player->send(msg::header::queue_size, players.size());
    }

    std::shuffle(players.begin(), players.end(), wall.engine());

    for (int pos = 0; pos < NUM_PLAYERS; ++pos)
    {
        players[pos]->send(msg::header::your_position, pos);
        player_id_map[players[pos]->uid] = pos;
    }
    
    main_thread = std::thread(&game::play, this);
    main_thread.detach();
}

/**
 * Accept Spectator by locking the spectator list first before
 * emplacing the new client as a spectator.
 */
void game::accept_spectator(client_ptr &&client)
{
    std::scoped_lock lock(spectator_mutex);
    spectators.emplace_back(std::move(client));
}

/**
 * Perform a reconnection by checking if the original socket is closed.
 * If so, it will reconnect the player. Otherwise, it does nothing.
 */
void game::reconnect(client_ptr &&client)
{
    for (auto &player : players)
    {
        if (player->uid == client->uid && !player->is_open())
        {
            player = std::move(client);
            server_log << "Player " << std::to_string(player->uid) << " reconnected.";
            return;
        }
    }
}

/******************************************************************************/

/**
 * Draws a random tile from the wall by first locking the RNG.
 * 
 * If the tile is MJ_INVALID_TILE, that means the game is over and it is an
 * exhaustive draw tie.
 * If there are not eough tiles in the wall for it to still be the first turn, 
 * it will update the first turn flag.
 * It also updates the Ippatsu flag, because the turn has gotten around once
 * to the player who called riichi.
 * 
 * It will first broadcast the player who drew the tile to everyone.
 * Then, it will broadcast the tile to everyone if it is transparent.
 * If it is opaque, the tile will be sent to the player who drew it only, and
 * everyone else will receive a MJ_INVALID_TILE to indicate that the tile
 * was opaque.
 * 
 * It then sets the current tile state to the tile that was drawn.
 */
void game::draw()
{
    std::unique_lock lock(rng_mutex);
    auto tile = wall();
    lock.unlock();

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
        players[cur_player]->send(msg::header::tile, tile);
    }
    else
        broadcast(msg::header::tile, tile);
    
    cur_state = state_type::self_call;
    
    cur_tile = tile;

    log_cur("drew");
}

/**
 * Draws a new dora and broadcasts it to everyone. This does not change the
 * number of live tiles in the wall.
 */
void game::new_dora()
{
    std::unique_lock lock(rng_mutex);
    dora_tiles.push_back(wall.draw_dora());
    lock.unlock();

    broadcast(msg::header::dora_indicator, dora_tiles.back());
}

/**
 * Performs payment to the provided player with a given score. Positive means
 * points are given to the player.
 * 
 * It first broadcasts which player is receiving points, then how many points
 * the player receives.
 */
void game::payment(int player, score_type score)
{
    scores[player] += score;
    broadcast(msg::header::this_player_won, player);
    broadcast(msg::header::this_many_points, score);
}

/**
 * Initiales a call of KONG by the current player.
 * 
 * It checks for kong from pong first. If that is possible, the OTHER_KONG_FLAG
 * is enabled in the game flags.
 * 
 * Otherwise it checks for closed kong. If that is possible, the 
 * CLOSED_KONG_FLAG is enabled in the game flags.
 * 
 * Otherwise, the server will send a reject to the player, and false is returned.
 * 
 * If either kong was succesful, the player who called the kong is broadcasted.
 * Along with a tile kong was called with. The function then return true.
 *  
 */
bool game::self_call_kong(game::card_type with)
{
    auto *kong = mj_open_kong_available(melds[cur_player], with);

    if (kong)
    {
        *kong = MJ_KONG_TRIPLE(*kong);
        mj_discard_tile(&hands[cur_player], with);
        game_flags |= OTHER_KONG_FLAG;
    }
    else if (mj_closed_kong_available(hands[cur_player], with))
    {
        mj_discard_tile(&hands[cur_player], with);
        mj_discard_tile(&hands[cur_player], with^0b01);
        mj_discard_tile(&hands[cur_player], with^0b10);
        mj_discard_tile(&hands[cur_player], with^0b11);
        mj_add_meld(&melds[cur_player], MJ_KONG_TRIPLE(MJ_TRIPLE(
                with,with^0b01,with^0b10)));
        game_flags |= CLOSED_KONG_FLAG;
    }
    else
    {
        players[cur_player]->send(msg::header::reject, msg::REJECT);
        return false;
    }
    broadcast(msg::header::this_player_kong, cur_player);
    broadcast(msg::header::tile, with);
    return true;
}

/**
 * The server first broadcasts the player who declared the tsumo.
 * The server then broadcasts the player's closed hand as a stream.
 * 
 * Then the game calculates the players yakus. If the player has no yakus, the
 * player who declared tsumo will be awarded with the chombo penalty. 
 * 
 * Otherwise, the number of fu of the hand is broadcasted.
 * Then, the yakus are broadcasted as a stream.
 * 
 * Finally, the payments are broadcasted. If the player who declared the tsumo
 * is the dealer, the game will move into the renchan state. Otherwise,
 * the game moves into the "next" state.
 */
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

    if (yakus[MJ_YAKU_RICHII])
    {
        auto doras = dora_tiles.size();
        for (int i = 0; i < doras; ++i)
            dora_tiles.push_back(wall.draw_dora());
    }

    for (auto &indicator : dora_tiles)
    {
        yakus[MJ_YAKU_DORA] += std::count_if(hands[cur_player].tiles, 
        hands[cur_player].tiles+hands[cur_player].size, 
        [indicator](const card_type &tile){
            return calc_dora(indicator) == MJ_ID_128(tile);
        });
    }

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
                    payment(p, 6*score+ deposit + 3*bonus_score);
                else
                    payment(p, -2*score - bonus_score);
            }
            return state_type::renchan;
        }
        else
        {
            for (int p = 0; p < NUM_PLAYERS; ++p)
            {
                if (p == cur_player)
                    payment(p, 4*score+ deposit + 3*bonus_score);
                else if (p == dealer)
                    payment(p,-2*score - bonus_score);
                else
                    payment(p,-score - bonus_score);
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
            break;
        case state_type::self_call:
            cur_state = self_call();
            break;
        case state_type::discard:
            cur_state = discard();
            break;
        case state_type::opponent_call:
            cur_state = opponent_call();
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

/**
 * If the prevailing wind is the west wind, then the game is over.
 * Otherwise, the round starts by emptying players hands, melds, and discards.
 * 
 * The wall is then reset, and every player draws 13 tiles and a new dora is set
 * for the round.
 * 
 * The first turn flag is set, and the current tile is MJ_INVALID_TILE. Then,
 * the dealer is set as the current player and the game state is changed to 
 * the draw state.
 */
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

    wall.reset();

    cur_player = dealer;
    for (int i = 0; i < NUM_PLAYERS; ++i)
    {
        for (int j = 0; j < 13; ++j)
            draw();
        cur_player = (cur_player + 1) % NUM_PLAYERS;   
    }

    new_dora();

    game_flags |= FIRST_TURN_FLAG;

    cur_tile = MJ_INVALID_TILE;
    cur_state = state_type::draw;
}

/**
 * This method timeouts. This method returns the next state.
 * 
 * There are as many retrues until timeout. If timeout, the next state is tsumogiri.
 * 
 * The methods waits for a message from the current player that is about a kong,
 * tsumo, riichi, pass, or discard. 
 * 
 * If a kong is called, the server expects an auxiliary message about a tile
 * the kong is called with. This tile is broadcasted if the kong is valid. 
 */
game::state_type game::self_call()
{
    msg::buffer buffer, aux;
    auto timeout_time = clock_type::now() + SELF_CALL_TIMEOUT;


    while(true) /* We allow retrys until timeout */
    {
        msg::buffer buffer = fetch_cur(timeout_time);
        msg::header ty = msg::type(buffer);
        switch (ty)
        {
        case msg::header::call_kong:
            aux = fetch_cur(timeout_time);
            if (msg::type(aux)==msg::header::call_with_tile && self_call_kong(msg::data<card_type>(aux)))
                return state_type::after_kong;
            else
            {
                players[cur_player]->send(msg::header::reject, msg::REJECT);
                server_log << "Player with UID=" << std::to_string(players[cur_player]->uid) 
                    << " @" << players[cur_player]->ip() 
                    << " called an invalid kong." << "\n";
                break; /* from switch, try again */
            }

        case msg::header::call_tsumo:
            return call_tsumo();

        case msg::header::call_riichi:
            for (auto *i = melds[cur_player].melds; i < melds[cur_player].melds + melds[cur_player].size; ++i)
            {
                if (MJ_IS_OPEN(*i)) /* Riichi not allowed on open hands */
                {
                    players[cur_player]->send(msg::header::reject, msg::REJECT);
                    break;
                }
            }
            broadcast(msg::header::this_player_riichi, cur_player);
            payment(cur_player, -MJ_RIICHI_DEPOSIT);
            deposit += MJ_RIICHI_DEPOSIT;
            flags[cur_player] |= RIICHI_FLAG;
        case msg::header::pass_calls:
            return state_type::discard;

        case msg::header::discard_tile:
            auto discarded = msg::data<card_type>(buffer);
            if (mj_discard_tile(&hands[cur_player], discarded))
            {
                discards[cur_player].push_back(discarded);
                broadcast(msg::header::tile, discarded);
                cur_tile = discarded;

                log_cur("discarded");
                
                return state_type::opponent_call;
            }
            else
            {
                players[cur_player]->send(msg::header::reject, msg::REJECT);
                break; /* from switch, try again */
            }
        }
    }
}

game::state_type game::discard()
{
    auto timeout_time = clock_type::now() + DISCARD_TIMEOUT;
    msg::buffer buffer = fetch_cur(timeout_time);
    auto discarded = msg::data<card_type>(buffer);

    game_flags &= ~KONG_FLAG;

    if (msg::type(buffer) == msg::header::discard_tile && 
        mj_discard_tile(&hands[cur_player], discarded))
    {
        if (discarded != cur_tile && flags[cur_player] & (DOUBLE_RIICHI_FLAG | RIICHI_FLAG))
            return state_type::chombo;
        discards[cur_player].push_back(discarded);
        broadcast(msg::header::tile, discarded);
        cur_tile = discarded;

        log_cur("discarded");

        return state_type::opponent_call;
    }
    else if (msg::type(buffer) != msg::header::timeout) // invalid, not timeout
    {
        players[cur_player]->send(msg::header::reject, msg::REJECT);
        server_log << "Player with UID=" << std::to_string(players[cur_player]->uid) << 
        " @" << players[cur_player]->ip() <<
        " discarded an invalid tile.";
    }
    return state_type::tsumogiri;
}

game::state_type game::opponent_call() 
{
    // sort the players by priority. The player next to play is highest priority.
    std::array<int, NUM_PLAYERS-1> order = {
        static_cast<int>((cur_player+1)%NUM_PLAYERS),
        static_cast<int>((cur_player+2)%NUM_PLAYERS),
        static_cast<int>((cur_player+3)%NUM_PLAYERS)
    };

    std::array<score_type, NUM_PLAYERS> fu_if_ron{}, fan_if_ron{};
    std::array<std::array<unsigned short, MJ_YAKU_ARR_SIZE>, NUM_PLAYERS> yakus_if_ron {};

    for (auto const &p : order)
    {
    // if player is in furiten, continue because cannot ron
        if (std::find_if(discards[p].begin(), discards[p].end(), 
            [&](const card_type &tile) { 
                return MJ_ID_128(tile) == MJ_ID_128(cur_tile); 
            }) != discards[p].end()) 
                continue;
    
    // create temp hand and add the ron tile to it
        mj_hand tmp_hand;
        memcpy(&tmp_hand, &hands[p], sizeof(mj_hand));
        tmp_hand.tiles[tmp_hand.size++] = cur_tile;
        mj_sort_hand(&tmp_hand);

    // check if it can win
        int seat_wind = (NUM_PLAYERS+p-dealer)%NUM_PLAYERS;

        yakus_if_ron[p][MJ_YAKU_RICHII] = flags[p] & (DOUBLE_RIICHI_FLAG | RIICHI_FLAG);
        yakus_if_ron[p][MJ_YAKU_IPPATSU] = (flags[p] & IPPATSU_FLAG) ? 1 : 0;
        yakus_if_ron[p][MJ_YAKU_CHANKAN] = (game_flags & KONG_FLAG) ? 1 : 0;
        yakus_if_ron[p][MJ_YAKU_HOUTEI] = wall.size() ? 0 : 1;

        mj_score(&fu_if_ron[p], &fan_if_ron[p], yakus_if_ron[p].data(), &tmp_hand, 
            &melds[p], cur_tile, MJ_FALSE, prevailing_wind, seat_wind);
    }


    std::array<mj_bool, 10> priority {};
    std::array<std::array<card_type, 3>, 3> call_tiles {};
    std::array<int, 3> num_call_tiles {};

    priority[0] = mj_chow_available(hands[order[0]], cur_tile, nullptr) ? MJ_MAYBE : MJ_FALSE;
    priority[1] = mj_pong_available(hands[order[2]], cur_tile) ? MJ_MAYBE : MJ_FALSE;
    priority[2] = mj_pong_available(hands[order[1]], cur_tile) ? MJ_MAYBE : MJ_FALSE;
    priority[3] = mj_pong_available(hands[order[0]], cur_tile) ? MJ_MAYBE : MJ_FALSE;
    priority[4] = mj_kong_available(hands[order[2]], cur_tile) ? MJ_MAYBE : MJ_FALSE;
    priority[5] = mj_kong_available(hands[order[1]], cur_tile) ? MJ_MAYBE : MJ_FALSE;
    priority[6] = mj_kong_available(hands[order[0]], cur_tile) ? MJ_MAYBE : MJ_FALSE;
    priority[7] = fan_if_ron[order[2]] > 0 ? MJ_MAYBE : MJ_FALSE;
    priority[8] = fan_if_ron[order[1]] > 0 ? MJ_MAYBE : MJ_FALSE;
    priority[9] = fan_if_ron[order[0]] > 0 ? MJ_MAYBE : MJ_FALSE;

    auto timeout_time = clock_type::now() + OPPONENT_CALL_TIMEOUT;
    int max_priority = 9;
    while (true)
    {
        while (priority[max_priority] == MJ_FALSE && max_priority >= 0)
            max_priority--;
        if (max_priority < 0)
        {
            std::this_thread::sleep_for(
                wall.tiger() / static_cast<float>(0xffff) * END_TURN_DELAY);
            return state_type::draw;
        }
        if (priority[max_priority] == MJ_TRUE)
            break;

        std::unique_lock ul(timeout_m);
        if (timeout_cv.wait_until(ul, timeout_time, [this]() { return !messages.empty(); }));
            break;

        auto call = messages.pop_front();
        int player = player_id_map[call.id];
        if (player == cur_player)
            continue;
        int player_p = (std::find(order.begin(), order.end(), player) - order.begin()) / sizeof(int);
        switch(msg::type(call.data))
        {
        case msg::header::call_ron:
            if (priority[player_p+7] == MJ_MAYBE)
                priority[player_p+7] = MJ_TRUE;
        case msg::header::call_kong:
            if (priority[player_p+4] == MJ_MAYBE)
                priority[player_p+4] = MJ_TRUE;
        case msg::header::call_pong:
            if (priority[player_p+1] == MJ_MAYBE)
                priority[player_p+1] = MJ_TRUE;
        case msg::header::call_chow:
            if (player_p == order[0] && priority[0] == MJ_MAYBE)
                priority[0] = MJ_TRUE;
        case msg::header::pass_calls:
            priority[player_p+7] = MJ_FALSE;
            priority[player_p+4] = MJ_FALSE;
            priority[player_p+1] = MJ_FALSE;
        case msg::header::call_with_tile:
            if (num_call_tiles[player_p] < 3)
                call_tiles[player_p][num_call_tiles[player_p]++] = msg::data<card_type>(call.data);
        default:
            break;
        }
    }

    while (priority[max_priority] != MJ_TRUE && max_priority >= 0)
        max_priority--;
    if (max_priority < 0)
    {
        std::this_thread::sleep_for(
            wall.tiger() / static_cast<float>(0xffff) * END_TURN_DELAY);
        return state_type::draw;
    }

    if (max_priority > 6)
    {
        // ron
        int ron_player = order[9-max_priority];
        broadcast(msg::header::this_player_ron, ron_player);
        if (yakus_if_ron[ron_player][MJ_YAKU_RICHII])
        {
            auto doras = dora_tiles.size();
            for (int i = 0; i < doras; ++i)
                dora_tiles.push_back(wall.draw_dora());
        }
        for (auto &indicator : dora_tiles)
        {
            yakus_if_ron[ron_player][MJ_YAKU_DORA] += std::count_if(hands[ron_player].tiles, 
            hands[ron_player].tiles+hands[ron_player].size, 
            [indicator](const card_type &tile){
                return calc_dora(indicator) == MJ_ID_128(tile);
            });
            for (auto *i = melds[ron_player].melds; i < melds[ron_player].melds+melds[ron_player].size; ++i)
            {
                if (MJ_IS_KONG(*i))
                    yakus_if_ron[ron_player][MJ_YAKU_DORA] += calc_dora(indicator) == MJ_ID_128(MJ_FIRST(*i)) ? 4 : 0;
                else
                {
                    yakus_if_ron[ron_player][MJ_YAKU_DORA] += calc_dora(indicator) == MJ_ID_128(MJ_FIRST(*i)) ? 1 : 0;
                    yakus_if_ron[ron_player][MJ_YAKU_DORA] += calc_dora(indicator) == MJ_ID_128(MJ_SECOND(*i)) ? 1 : 0;
                    yakus_if_ron[ron_player][MJ_YAKU_DORA] += calc_dora(indicator) == MJ_ID_128(MJ_THIRD(*i)) ? 1 : 0;
                }
            }
        }

        score_type b_score = mj_basic_score(fu_if_ron[ron_player], 
            fan_if_ron[ron_player]+yakus_if_ron[ron_player][MJ_YAKU_DORA]);

        broadcast(msg::header::this_player_hand, ron_player);

        broadcast(msg::header::closed_hand, msg::START_STREAM);
        for (auto *i = hands[ron_player].tiles; i < hands[ron_player].tiles+hands[ron_player].size; ++i)
            broadcast(msg::header::tile, *i);
        broadcast(msg::header::closed_hand, msg::END_STREAM);

        broadcast(msg::header::fu_count, fu_if_ron[ron_player]);
        broadcast(msg::header::yaku_list, msg::START_STREAM);
        for (int i = 0; i < MJ_YAKU_ARR_SIZE; ++i)
        {
            if (yakus_if_ron[ron_player][i])
            {
                broadcast(msg::header::winning_yaku, i);
                broadcast(msg::header::yaku_fan_count, yakus_if_ron[ron_player][i]);
            }
        }
        broadcast(msg::header::yaku_list, msg::END_STREAM);

        for (auto &p_flags : flags)
            p_flags &= ~IPPATSU_FLAG;

        if (ron_player == dealer)
        {
            payment(ron_player, b_score*6 + deposit + bonus_score*3);
            payment(cur_player, -b_score*6 - bonus_score*3);
            return state_type::renchan;
        }
        else
        {
            payment(ron_player, b_score*4 + deposit + bonus_score*3);
            payment(cur_player, -b_score*4 - bonus_score*3);
            return state_type::next;
        }
    }
    else if (max_priority > 3)
    {}
    else if (max_priority > 0)
    {}
    else
    {}


    std::array<std::future<msg::buffer>,NUM_PLAYERS> future_buffer;
    std::array<msg::buffer, NUM_PLAYERS> buffer;

    for (auto const &p : priority_order)
    {
        if (fan_if_ron[p] || pong_possible[p] || p==priority_order.front() && chow_possible)
        {
            future_buffer[p] = std::async(std::launch::async | std::launch::deferred, 
                &client_type::recv<clock_type::time_point>, players[p].get(), timeout_time);
        }
    }
    // ron is first priority
    for (auto const &p : priority_order)
    {
        if (fan_if_ron[p])
        {
            buffer[p] = future_buffer[p].get();
            if (msg::type(buffer[p]) == msg::header::call_ron)
            {
                broadcast(msg::header::this_player_ron, p);
    // count the doras
                if (yakus_if_ron[p][MJ_YAKU_RICHII])
                {
                    auto doras = dora_tiles.size();
                    for (int i = 0; i < doras; ++i)
                        dora_tiles.push_back(wall.draw_dora());
                }

                for (auto &indicator : dora_tiles)
                {
                    yakus_if_ron[p][MJ_YAKU_DORA] += std::count_if(hands[cur_player].tiles, 
                    hands[cur_player].tiles+hands[cur_player].size, 
                    [indicator](const card_type &tile){
                        return calc_dora(indicator) == MJ_ID_128(tile);
                    });
                }

                score_type b_score = mj_basic_score(fu_if_ron[p], fan_if_ron[p]+yakus_if_ron[p][MJ_YAKU_DORA]);

                broadcast(msg::header::this_player_hand, p);
                broadcast(msg::header::closed_hand, msg::START_STREAM);
                for (auto *i = hands[p].tiles; i < hands[p].tiles+hands[p].size; ++i)
                    broadcast(msg::header::tile, *i);
                broadcast(msg::header::closed_hand, msg::END_STREAM);

                broadcast(msg::header::fu_count, fu_if_ron[p]);
                broadcast(msg::header::yaku_list, msg::START_STREAM);
                for (int i = 0; i < MJ_YAKU_ARR_SIZE; ++i)
                {
                    if (yakus_if_ron[p][i])
                    {
                        broadcast(msg::header::winning_yaku, i);
                        broadcast(msg::header::yaku_fan_count, yakus_if_ron[p][i]);
                    }
                }
                broadcast(msg::header::yaku_list, msg::END_STREAM);

                for (auto &p_flags : flags)
                    p_flags &= ~IPPATSU_FLAG;

                if (p == dealer)
                {
                    payment(p, b_score*6 + deposit + bonus_score*3);
                    payment(cur_player, -b_score*6 - bonus_score*3);
                    return state_type::renchan;
                }
                else
                {
                    payment(p, b_score*4 + deposit + bonus_score*3);
                    payment(cur_player, -b_score*4 - bonus_score*3);
                    return state_type::next;
                }
            }
        }
    }
    // pong is second priority
    for (auto const &p : priority_order)
    {
        if (pong_possible[p])
        {
            if (future_buffer[p].valid())
                buffer[p] = future_buffer[p].get();
            if (msg::type(buffer[p]) == msg::header::call_pong)
            {
    // pong will take 2 tiles from the players hand.
                msg::buffer pong_tile_buffer;
                std::vector<card_type> pong_tiles;
                while(pong_tiles.size() < 2)
                {
                    pong_tile_buffer = players[p]->recv(timeout_time);
                    card_type tile = msg::data<card_type>(pong_tile_buffer);
                    if (msg::type(pong_tile_buffer) == msg::header::call_with_tile &&
                    mj_discard_tile(&hands[p], tile) && MJ_ID_128(tile) == MJ_ID_128(cur_tile))
                        pong_tiles.push_back(tile);
                    else
                        players[p]->send(msg::header::reject, msg::REJECT);
                }
                broadcast(msg::header::this_player_pong, p);
                for (auto const &tile : pong_tiles)
                    broadcast(msg::header::tile, tile);
                
                mj_add_meld(&melds[p], MJ_OPEN_TRIPLE(MJ_TRIPLE(
                    cur_tile, pong_tiles[0], pong_tiles[1])));
                
                cur_player = p;

                for (auto &p_flags : flags)
                    p_flags &= ~IPPATSU_FLAG;
                return state_type::discard;
            }
            else if (msg::type(buffer[p]) == msg::header::call_kong)
            {
                broadcast(msg::header::this_player_kong, p);
                
                msg::buffer kong_tile_buffer;
                std::vector<card_type> kong_tiles;
                while(kong_tiles.size() < 3)
                {
                    kong_tile_buffer = players[p]->recv(timeout_time);
                    card_type tile = msg::data<card_type>(kong_tile_buffer);
                    if (msg::type(kong_tile_buffer) == msg::header::call_with_tile &&
                    mj_discard_tile(&hands[p], tile) && MJ_ID_128(tile) == MJ_ID_128(cur_tile))
                        kong_tiles.push_back(tile);
                    else
                        players[p]->send(msg::header::reject, msg::REJECT);
                }

                broadcast(msg::header::this_player_kong, p);
                for (auto const &tile : kong_tiles)
                    broadcast(msg::header::tile, tile);

                mj_add_meld(&melds[p], MJ_KONG_TRIPLE(MJ_OPEN_TRIPLE(MJ_TRIPLE(
                    cur_tile, kong_tiles[0], kong_tiles[1]))));
                
                game_flags |= OTHER_KONG_FLAG;
                cur_player = p;

                for (auto &p_flags : flags)
                    p_flags &= ~IPPATSU_FLAG;
                return state_type::after_kong;
            }
        }
    }

    // if no pong or kong, then the current player is the next player.
    cur_player = priority_order.front();
    // chow is third priority
    if (chow_possible)
    {
        if (future_buffer[cur_player].valid())
            buffer[cur_player] = future_buffer[cur_player].get();
        if (msg::type(buffer[cur_player]) == msg::header::call_chow)
        {
            broadcast(msg::header::this_player_chow, cur_player);
            
            msg::buffer chow_tile_buffer;
            std::vector<card_type> chow_tiles;

    // WARNING: chow has no checks
            while(chow_tiles.size() < 2)
            {
                chow_tile_buffer = players[cur_player]->recv(timeout_time);
                card_type tile = msg::data<card_type>(chow_tile_buffer);
                if (msg::type(chow_tile_buffer) == msg::header::call_with_tile &&
                mj_discard_tile(&hands[cur_player], tile))
                    chow_tiles.push_back(tile);
                else
                    players[cur_player]->send(msg::header::reject, msg::REJECT);
            }

            broadcast(msg::header::this_player_chow, cur_player);
            for (auto const &tile : chow_tiles)
                broadcast(msg::header::tile, tile);
            
            mj_add_meld(&melds[cur_player], MJ_OPEN_TRIPLE(MJ_TRIPLE(
                cur_tile, chow_tiles[0], chow_tiles[1])));

            for (auto &p_flags : flags)
                p_flags &= ~IPPATSU_FLAG;

            return state_type::discard;
        }
    }

    // all players pass or timeout
    std::this_thread::sleep_for(
        wall.tiger() / static_cast<float>(0xffff) * END_TURN_DELAY);
    return state_type::draw;
}

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
    broadcast(msg::header::exhaustive_draw, msg::NO_INFO);

    auto timeout_time = clock_type::now() + TENPAI_TIMEOUT;

    std::array<mj_bool, NUM_PLAYERS> tenpai;
    int players_tenpai = 0;

    for (int p = 0; p < NUM_PLAYERS; ++p)
    {
        auto response = players[p]->recv(timeout_time);
        if (msg::type(response) == msg::header::call_tenpai)
        {
            switch(msg::data<unsigned short>(response))
            {
            case msg::TENPAI:
                tenpai[p] = MJ_TRUE;
            case msg::NO_TEN:
                tenpai[p] = MJ_FALSE;
            default:
                tenpai[p] = MJ_MAYBE;
            }
        }
        else
            tenpai[p] = MJ_MAYBE;
    }

    for (int p = 0; p < NUM_PLAYERS; ++p)
    {
        if (tenpai[p] && mj_tenpai(hands[p], melds[p], nullptr))
        {
            broadcast(msg::header::this_player_hand, p);
            broadcast(msg::header::closed_hand, msg::START_STREAM);
            for (auto *it = hands[p].tiles; it < hands[p].tiles+hands[p].size; ++it)
                broadcast(msg::header::tile, *it);
            broadcast(msg::header::closed_hand, msg::END_STREAM);
            tenpai[p] = MJ_TRUE;
            ++players_tenpai;
        }
        else
        {
            tenpai[p] = MJ_FALSE;
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
    {
        discards[cur_player].push_back(cur_tile);
        broadcast(msg::header::tile, cur_tile);
        log_cur("tsumogiri");
    }
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

mj_id game::calc_dora(game::card_type tile)
{
    switch(MJ_SUIT(tile))
    {
    case MJ_WIND:
        return MJ_128_TILE(MJ_WIND, (MJ_ID_128(tile) + 1) % 4);
    case MJ_DRAGON:
        return MJ_128_TILE(MJ_DRAGON, (MJ_ID_128(tile) + 1) % 3);
    default:
        return MJ_128_TILE(MJ_SUIT(tile), (MJ_ID_128(tile) + 1) % 9);
    }
}

void game::log_cur(char const *msg)
{
    char suit[5] = {'m', 'p', 's', 'w', 'd'};
    game_log << cur_player << " " << msg << " " << MJ_NUMBER1(cur_tile) << 
        suit[MJ_SUIT(cur_tile)] << std::endl;
}


std::unordered_map<unsigned short, game> game::games;
