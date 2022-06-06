#ifndef MJ_SERVER_DECK_HPP
#define MJ_SERVER_DECK_HPP

#include "mahjong/mahjong.h"
#include <deque>
#include <random>

/**
 * @brief The deck class allows the shuffle and reset of any a deck of tiles.
 * This represent the walls of an actual game, and is used to draw tiles. It
 * includes a live and dead wall.
 */
class deck
{
public:
    using card_type         = mj_tile;
    using container_type    = std::deque<card_type>;
    using rng_type          = std::mt19937_64;

public:
    /**
     * Construct a new deck and shuffle it.
     */
    deck();

    deck(deck const &) = delete;
    deck &operator=(deck const &) = delete;

    /**
     * @brief Reset the wall and reshuffle it. Should be called at the start of
     * each round.
     */
    void reset();

    /**
     * @brief Return a random unsigned short from the RNG, sort of like a slot
     * or tiger machine.
     *
     * @return A random unsigned short from 0 to 0xffff.
     */
    unsigned short tiger() { return luck(rng); }

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
     * @return MJ_INVALID_TILE if the dead wall is depleated.
     */
    card_type draw_dora();

    /**
     * @return number of tiles in the live wall.
     */
    constexpr std::size_t size() const
    { return live_count; };

    /**
     * @return The rng engine for the wall. Could be used to shuffle another
     * array without needing to another rng.
     *
     * @warning This should not be used for a different game instance because
     * rng may not be thread safe.
     */
    rng_type &engine() { return rng; };

private:
    container_type tiles;
    std::size_t live_count { MJ_DECK_SIZE - MJ_DEAD_WALL_SIZE };
    std::size_t dora_count { 0 };
    rng_type rng{ std::random_device{}() };
    std::uniform_int_distribution<unsigned short> luck { 0, 0xffff };
};

#endif
