/**
 * This file describes the message structure of all communications between
 * the server and the client within the game. 
 */

#pragma once

#include <string>

namespace msg
{

enum class type : char
{
    join_as_player          = 'p',
    join_as_spectator       = 's',
    draw_tile               = 'd',
    call_pong               = '3',
    call_chow               = 'c',
    call_kong               = '4',
    call_richii             = 'r',
    call_ron                = '*',
    call_tsumo              = '+',
    ask_for_draw            = 'd',

    ping                    = ';',
    
    reject                  = 'X',
    queue_size              = 'Q',
    your_position           = 'P',
    this_player_drew        = 'D',
    tile                    = 'T',
    this_player_pong        = '#',
    this_player_chow        = 'C',
    this_player_kong        = '$',
    this_player_richii      = 'R',
    this_player_ron         = '/',
    this_player_tsumo       = '-',
    you_won                 = 'W',

    
};

}