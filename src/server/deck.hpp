#pragma once

#include "mahjong/mahjong.h"
#include <random>
#include <deque>

class deck
{
public:
    using card_type = mj_tile;
    using container_type = std::deque<card_type>;

public:
    deck();
    
    deck(deck const &) = delete;
    deck &operator=(deck const &) = delete;

    /**
     * @brief Draw a card from the deck.
     * 
     * @return A tile in the live wall.
     */
    card_type operator()();

    /**
     * @brief Draw a dora from the deck. This does not decrease the number of
     * tiles in the live wall.
     * 
     * @return A tile in the dead wall.
     */
    card_type draw_dora();

    /** 
     * @return number of tiles in the live wall. 
     */
    constexpr std::size_t size() const
    { return live_count; };

private:
    container_type tiles;
    std::size_t live_count { MJ_DECK_SIZE - MJ_DEAD_WALL_SIZE };
    std::size_t dora_count { 0 };
    std::mt19937 rng{ std::random_device{}() };
};
