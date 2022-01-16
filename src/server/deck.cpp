#include "deck.hpp"
#include <algorithm>
deck::deck()
{
    reset();
}

void deck::reset()
{
    tiles.clear();

    /* Add the normal tiles to the deck */
    for (int suit = MJ_CHARACTER; suit <= MJ_BAMBOO; suit++)
        for (int number = 0; number < 9; number++)
            for (int sub = 0; sub < 4; sub++)
                tiles.push_back(MJ_TILE(suit, number, sub));

    /* Add the wind tiles to the deck */
    for (int number = MJ_EAST; number <= MJ_NORTH; number++)
        for (int sub = 0; sub < 4; sub++)
            tiles.push_back(MJ_TILE(MJ_WIND, number, sub));

    /* Add the dragon tiles to the deck */
    for (int number = MJ_GREEN; number <= MJ_WHITE; number++)
        for (int sub = 0; sub < 4; sub++)
            tiles.push_back(MJ_TILE(MJ_DRAGON, number, sub));
    
    /* Now shuffle the deck */
    std::shuffle(tiles.begin(), tiles.end(), rng);

    live_count = MJ_DECK_SIZE - MJ_DEAD_WALL_SIZE;
    dora_count = 0;
}

deck::card_type deck::operator()()
{
    if (live_count-- == 0)
        return MJ_INVALID_TILE;
        
    card_type tile = tiles.front();
    tiles.pop_front();
    return tile;
}

deck::card_type deck::draw_dora()
{
    if (++dora_count==5)
        return MJ_INVALID_TILE;
        
    card_type tile = tiles.back();
    tiles.pop_back();
    return tile;
}
