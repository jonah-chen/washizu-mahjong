#include "mahjong.h"
#include <stdlib.h>
#include <string.h>


void mj_sort_hand(mj_tile *hand, mj_size size)
{
    mj_size i, j;
    mj_tile tmp;
    for (i = 1; i < size; ++i)
    {
        for (j = i; j > 0 && hand[j-1] > hand[j]; --j)
        {
            tmp = hand[j];
            hand[j] = hand[j-1];
            hand[j-1] = tmp;
        }
    }
}

mj_size mj_pairs(mj_tile *hand, mj_size size, mj_id *result)
{
    mj_size i, pairs = 0;
    
    for (i = 0; i < size-1; ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]))
        {
            result[pairs++] = MJ_ID_128(hand[i++]);
        }
    }
    LOG_DEBUG("pairs: %d\n", pairs);
    return pairs;
}

mj_bool mj_pong_available(mj_tile *hand, mj_size size, mj_tile const tile)
{
    for (mj_size i = 0; i < size-1 && MJ_ID_128(hand[i]) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_bool mj_kong_available(mj_tile *hand, mj_size size, mj_tile const tile)
{
    for (mj_size i = 0; i < size-2 && MJ_ID_128(hand[i]) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+2]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_size mj_chow_available(mj_tile *hand, mj_size size, mj_tile const tile, mj_pair *chow_tiles)
{
    if (MJ_SUIT(tile) == MJ_WIND || MJ_SUIT(tile) == MJ_DRAGON)
    {
        return 0;
    }

    mj_size i, j, chows = 0;

    for (i = 0; i < size-1; ++i)
    {
        int difference; 
        if (MJ_SUIT(hand[i]) == MJ_SUIT(tile))
        {
            switch (MJ_NUMBER(hand[i]) - MJ_NUMBER(tile))
            {
                case -2: difference = -1; break;
                case -1: difference = 1; break;
                case 1: difference = 2; break;
                default: continue;
            }

            for (j = i+1; 
                 j < size && 
                 MJ_SUIT(hand[j]) == MJ_SUIT(tile) && 
                 MJ_NUMBER(hand[j])>MJ_NUMBER(tile)+difference;
                 ++j)
            {
                if (MJ_NUMBER(hand[j]) == MJ_NUMBER(tile) + difference)
                {
                    chow_tiles[chows++] = MJ_PAIR(hand[i], hand[j]);
                }
            }
        }
    }

    return chows;
}

mj_size mj_triples(mj_tile *hand, mj_size size, mj_triple *result, mj_size capacity)
{
    mj_size i, j, k, triples = 0;
    
    for (i = 0; i < size-2 && triples < capacity; ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+2]))
        {
            result[triples++] = MJ_TRIPLE(hand[i], hand[i+1], hand[i+2]);
            j = i + 3;
        }
        else if (MJ_SUIT(hand[i])==MJ_WIND || MJ_SUIT(hand[i])==MJ_DRAGON ||
                 MJ_NUMBER1(hand[i]) > 7)
        {
            continue;
        }
        else 
        {
            j = i + 1;
        }

        for (; j < size-1 && triples < capacity && 
               MJ_SUIT(hand[i]) == MJ_SUIT(hand[j]) &&
               MJ_NUMBER(hand[j]) - MJ_NUMBER(hand[i]) <= 1; ++j)
        {
            if (MJ_NUMBER(hand[j]) == MJ_NUMBER(hand[i]) + 1)
            {
                for (k = j + 1; k < size && triples < capacity && 
                       MJ_SUIT(hand[i]) == MJ_SUIT(hand[k]) &&
                       MJ_NUMBER(hand[k]) - MJ_NUMBER(hand[j]) <= 1; ++k)
                {
                    if (MJ_NUMBER(hand[k]) == MJ_NUMBER(hand[j]) + 1)
                    {
                        result[triples++] = MJ_TRIPLE(hand[i], hand[j], hand[k]);
                        break;
                    }
                }
            }
        }

    }

    LOG_DEBUG("triples: %d\n", triples);
#if _DEBUG_LEVEL > 1
    if (triples==capacity)
        LOG_WARN("The capacity was reached. Not all triples are included.\n");
#endif


    return triples;
}