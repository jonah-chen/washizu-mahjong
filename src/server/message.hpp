/**
 * This file describes the message structure of all communications between
 * the server and the client within the game. 
 * 
 * This file is the only file in the server folder that the client should use, 
 * because it defines how the server communicates with the client.
 */

#pragma once

#include <array>

namespace msg
{
using id_type = unsigned short;

static constexpr id_type 
    PLAYER = 0x3f3f,
    REJECT = 0x8088,
    START_STREAM = 0xa000,
    END_STREAM = 0xa001;

enum class header : char
{
    reconnect               = 'e', /* uid original */
    join_as_player          = 'p', /* magic number */
    join_as_spectator       = 's', /* game id */
    draw_tile               = 'd', /* uid */
    discard_tile            = 't', /* 9-bit tile unique ID */
    call_pong               = '3', /* uid */
    call_chow               = 'c', /* uid */
    call_kong               = '4', /* uid */
    call_riichi             = 'r', /* uid */
    call_ron                = '*', /* uid */
    call_tsumo              = '+', /* uid */
    ask_for_draw            = 'd', /* uid */

    ping                    = ';', /* random number (16 bit) */
    
    reject                  = 'X', /* magic number */
    queue_size              = 'Q', /* number (1,2,3,4) */
    your_id                 = 'I', /* number 16-bit */
    your_position           = 'P', /* number (0,1,2,3) */
    this_player_drew        = 'D', /* player */
    tile                    = 'T', /* 9 bit tile unique ID */
    this_player_pong        = '#', /* player */
    this_player_chow        = 'C', /* player */
    this_player_kong        = '$', /* player */
    this_player_riichi      = 'R', /* player */
    this_player_ron         = '/', /* player */
    this_player_tsumo       = '-', /* player */
    this_player_won         = 'W', /* num points */
    this_many_points        = 'Z', /* num points */
    dora_indicator          = 'B', /* 9 bit tile unique ID */
    error                   = '!', /* player that caused it */
    this_player_hand        = 'H', /* player */
    closed_hand             = 'K', /* stream */
    yaku_list               = 'M', /* stream */
    winning_yaku            = 'Y', /* yaku_type */
    yaku_fan_count          = 'F', /* int */
    fu_count                = 'U', /* int */
};

static constexpr std::size_t BUFFER_SIZE = 3;

using buffer = std::array<char,BUFFER_SIZE>;

template<typename ObjType>
constexpr buffer buffer_data(header header, ObjType obj)
{
    return buffer({
        static_cast<char>(header),
        static_cast<char>(obj&0xff),
        static_cast<char>((obj>>8)&0xff)
    });
}

constexpr header type(buffer const &buf)
{
    return static_cast<header>(buf[0]);
}

template<typename ObjType>
constexpr ObjType data(buffer const &buf)
{
    return static_cast<ObjType>((buf[1]&0xff) | ((buf[2]&0xff)<<8));
}

}
