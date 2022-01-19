#ifndef MJ_CLIENT_GAME_HPP
#define MJ_CLIENT_GAME_HPP

#include "mahjong/mahjong.h"
#include "mahjong/interaction.h"
#include "utils/optim.hpp"
#include "receiver.hpp"
#include <vector>

class server_exception : public std::exception
{
public:
    explicit server_exception(const std::string &msg) : msg(msg) {}
    const char *what() const noexcept override { return msg.c_str(); }
private:
    std::string msg;
};

class game
{
public:
    game();
    void start_round();
    void turn();
    void draw(int player);

private:
    R interface {R::protocall::v4(), 10000};
    std::array<mj_hand,4> hands;
    std::array<mj_meld,4> melds;
    std::vector<mj_tile, optim<24>::allocator<mj_tile>> discard_pile;
    int my_pos;
    msg::id_type my_uid;
    msg::buffer buf;
    int prevailing_wind;
    int round_no;
    int seat_wind;
    int cur_player;
private:
    void invalid_msg() const;
};

#endif