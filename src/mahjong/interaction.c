#include "interaction.h"
#include <string.h>

void mj_empty_hand(mj_hand *hand) 
{
    hand->size = 0;
    for (mj_tile *i = hand->tiles; i < hand->tiles + MJ_MAX_HAND_SIZE; ++i)
        *i = MJ_INVALID_TILE;
}

void mj_add_tile(mj_hand *hand, mj_tile tile)
{
    if (hand->size == MJ_MAX_HAND_SIZE)
        return;
    hand->tiles[hand->size++] = tile;
    mj_sort_hand(hand);
}

mj_bool mj_discard_tile(mj_hand *hand, mj_tile tile)
{
    for (mj_tile *i = hand->tiles; i < hand->tiles + hand->size; ++i)
    {
        if (*i == tile) 
        {
            *i = MJ_INVALID_TILE;
            mj_sort_hand(hand);
            --hand->size;
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;    
}

inline void mj_empty_melds(mj_meld *melds)
{
    memset(melds, 0, sizeof(mj_meld));
}

inline void mj_add_meld(mj_meld *melds, mj_triple triple)
{
    melds->melds[melds->size++] = triple;
}

mj_bool mj_pong_available(mj_hand hand, mj_tile const tile, mj_pair *pong_tiles)
{
    if (tile == MJ_INVALID_TILE)
        return MJ_FALSE;
    
    for (mj_tile *i = hand.tiles; i < hand.tiles + hand.size-1 && MJ_ID_128(*i) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(*i) == MJ_ID_128(i[1]) &&
            MJ_ID_128(*i) == MJ_ID_128(tile))
        {
            if (pong_tiles)
                *pong_tiles = MJ_PAIR(*i, i[1]);
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_bool mj_kong_available(mj_hand hand, mj_tile const tile)
{
    if (tile == MJ_INVALID_TILE)
        return MJ_FALSE;

    for (mj_tile *i = hand.tiles; i < hand.tiles+hand.size-2 && MJ_ID_128(*i) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(*i) == MJ_ID_128(i[1]) &&
            MJ_ID_128(*i) == MJ_ID_128(i[2]) &&
            MJ_ID_128(*i) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_bool mj_closed_kong_available(mj_hand hand, mj_tile const tile)
{
    if (tile == MJ_INVALID_TILE)
        return MJ_FALSE;

    for (mj_tile *i = hand.tiles; i < hand.tiles+hand.size-3 && MJ_ID_128(*i) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(*i) == MJ_ID_128(i[1]) &&
            MJ_ID_128(*i) == MJ_ID_128(i[2]) &&
            MJ_ID_128(*i) == MJ_ID_128(i[3]) &&
            MJ_ID_128(*i) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_triple *mj_open_kong_available(mj_meld melds, mj_tile const tile)
{
    if (tile == MJ_INVALID_TILE)
        return NULL;

    for (mj_triple *i = melds.melds; i < melds.melds + melds.size; ++i)
    {
        if (MJ_IS_SET(*i) && MJ_ID_128(*i) == MJ_ID_128(tile))
        {
            return i;
        }
    }
    return NULL;
}

mj_size mj_chow_available(mj_hand hand, mj_tile const tile, mj_pair *chow_tiles)
{
    if (tile == MJ_INVALID_TILE)
        return 0;
    
    if (MJ_SUIT(tile) == MJ_WIND || MJ_SUIT(tile) == MJ_DRAGON)
    {
        return 0;
    }

    mj_tile *i, *j;
    mj_size chows = 0;

    for (i = hand.tiles; i < hand.tiles+hand.size-1; ++i)
    {
        if (MJ_SUIT(*i) == MJ_SUIT(tile))
        {
            int difference; 
            switch (MJ_NUMBER(*i) - MJ_NUMBER(tile))
            {
                case -2: difference = -1; break;
                case -1: difference = 1; break;
                case 1: difference = 2; break;
                default: continue;
            }

            for (j = i+1; 
                 j < hand.tiles+hand.size && 
                 MJ_SUIT(*j) == MJ_SUIT(tile) && 
                 MJ_NUMBER(*j)<=MJ_NUMBER(tile)+difference;
                 ++j)
            {
                if (MJ_NUMBER(*j) == MJ_NUMBER(tile) + difference)
                {
                    if (chow_tiles)
                        chow_tiles[chows] = MJ_PAIR(*i, *j);
                    ++chows;
                }
            }
        }
    }

    return chows;
}
