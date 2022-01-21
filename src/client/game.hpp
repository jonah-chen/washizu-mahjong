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
    explicit server_exception(std::string const &msg) : msg(msg) {}
    const char *what() const noexcept override { return msg.c_str(); }
private:
    std::string msg;
};

class game
{
public:
    static constexpr int NUM_PLAYERS            = 4;
    static constexpr int MAX_DISCARD_PER_PLAYER = 24;
    static constexpr int MAX_CHOWS              = 16;
    static constexpr int MAX_DORAS              = 5;
    static constexpr int STARTING_PTS           = 30000;
    using card_type         = mj_tile;
    using score_type        = int;
    using discards_type     = std::vector<card_type,
        optim<MAX_DISCARD_PER_PLAYER>::allocator<card_type>>;

public:
    game();
    void start_round();
    void turn();

private:
    R interface {R::protocall::v4(), 10000};
    
    std::array<mj_hand, NUM_PLAYERS> hands {};
    std::array<mj_meld, NUM_PLAYERS> melds {};
    std::array<score_type, NUM_PLAYERS> scores {};
    std::array<discards_type, NUM_PLAYERS> discards {};
    std::vector<card_type, optim<MAX_DORAS*2>::allocator<card_type>> doras {};

    int my_pos;
    msg::id_type my_uid;
    msg::buffer buf;
    int prevailing_wind;
    int round_no;
    int seat_wind;
    int cur_player;
    bool in_riichi;
    mj_tile cur_tile;

    std::array<mj_pair, MAX_CHOWS> c_pairs;

    std::thread cmd_thread;
    std::mutex class_write_mutex;

private:
    void invalid_msg() const;

    void check_calls();

    void draw();

    void after_draw();

    void print_state() const;

    void payment();

    void command();
};

#endif
