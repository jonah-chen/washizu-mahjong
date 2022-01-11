#include "yaku.h"

int mj_fu(unsigned short *yakus, mj_meld const *melds, mj_pair pair, mj_tile ron, mj_bool tsumo)
{
    mj_bool closed = MJ_TRUE;
    int fu = MJ_BASE_FU + 2/* Most likely it is not 2 sided wait */;
    for (mj_size i = 0; i < MJ_MAX_TRIPLES_IN_HAND; ++i) 
    {
        mj_triple triple = melds->melds[i];
        if (MJ_FIRST(triple) == MJ_SECOND(triple)) // it is not a run
        {
            int triple_points = 2;
            mj_tile first_tile = MJ_FIRST(triple);

            /* Kong */
            if (MJ_IS_KONG(triple)==MJ_TRUE)
                triple_points <<= 2;

            /* Closed */
            if (MJ_IS_OPEN(triple)==MJ_TRUE)
                closed = MJ_FALSE;
            else if (tsumo==MJ_TRUE || MJ_ID_128(ron)!= MJ_ID_128(first_tile))
                triple_points <<= 1;

            /* Terminal or Honor */
            if (MJ_IS_HONOR(first_tile)||MJ_NUMBER(first_tile)==0||MJ_NUMBER(first_tile)==8)
                triple_points <= 1;

            fu += triple_points;
        }
        else if ((ron == MJ_FIRST(triple) && MJ_NUMBER(ron) != 6) || 
                (ron == MJ_THIRD(triple) && MJ_NUMBER(ron) != 2))
            fu -= 2 /* 2 sided wait */;
    }

    if (MJ_IS_HONOR(MJ_FIRST(pair)))
        fu += 2;

    if (closed && tsumo)
        yakus[MJ_YAKU_MEN] = 1;

    if (fu==MJ_BASE_FU)
    {
        yakus[MJ_YAKU_PINFU] = 1;
        return tsumo ? 20 : 30;
    }

    if (tsumo)
        fu += 2;
    else if (closed)
        fu += 10;

    return 10 * ((fu + 9) / 10);
}




// static mj_id const mj_guoshiwushuang[13] = {
//     MJ_128_TILE(0, 0),
//     MJ_128_TILE(0, 8),
//     MJ_128_TILE(1, 0),
//     MJ_128_TILE(1, 8),
//     MJ_128_TILE(2, 0),
//     MJ_128_TILE(2, 8),
//     MJ_128_TILE(MJ_WIND, 0),
//     MJ_128_TILE(MJ_WIND, 1),
//     MJ_128_TILE(MJ_WIND, 2),
//     MJ_128_TILE(MJ_WIND, 3),
//     MJ_128_TILE(MJ_DRAGON, 0),
//     MJ_128_TILE(MJ_DRAGON, 1),
//     MJ_128_TILE(MJ_DRAGON, 2)
// };

// static mj_id const mj_9things[13] = {
//     1,1,1,2,3,4,5,6,7,8,9,9,9
// };

// static mj_id const mj_greens[6] = {
//     MJ_128_TILE(MJ_BAMBOO, 1),
//     MJ_128_TILE(MJ_BAMBOO, 2),
//     MJ_128_TILE(MJ_BAMBOO, 3),
//     MJ_128_TILE(MJ_BAMBOO, 5),
//     MJ_128_TILE(MJ_BAMBOO, 7),
//     MJ_128_TILE(MJ_DRAGON, MJ_GREEN)
// };
















// mj_bool mj_kokushi(mj_hand const *hand)
// {
//     mj_size n_pair = 0;
//     for (int i = 0; i < 13; i++) 
//     {
//         if (hand->tiles[i] == hand->tiles[i+1])
//             n_pair = 1;

//         if (hand->tiles[i+n_pair] != mj_guoshiwushuang[i]) 
//             return MJ_FALSE;
//     }
//     return MJ_TRUE;
// }

// mj_bool mj_chuuren (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     if (MJ_SUIT(hand.tiles[0]) != MJ_SUIT(hand.tiles[13]))
//         return MJ_FALSE;

//     mj_size grace = 0;
//     for (int i = 0; i < 13; i++) 
//     {
//         if (MJ_NUMBER(hand.tiles[i+grace]) != mj_9things[i])
//             ++grace;
//         if (grace > 1)
//             return MJ_FALSE;
//     }
//     return MJ_TRUE;
// }

// mj_bool mj_tsuuiisou (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     return MJ_SUIT(hand.tiles[0]) == MJ_WIND ? MJ_TRUE : MJ_FALSE;
// }

// mj_bool mj_ryuuiisou (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     mj_size green_index = 0;
//     for (mj_size i = 0; i < 14; i++) 
//     {
//         while (hand.tiles[i] != mj_greens[green_index])
//         {
//             if(++green_index >= 6)
//                 return MJ_FALSE;
//         }
//     }
//     return MJ_TRUE;
// }

// mj_bool mj_chinitsu (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     return MJ_SUIT(hand.tiles[0]) == MJ_SUIT(hand.tiles[13]) ? MJ_TRUE : MJ_FALSE;
// }

// mj_bool mj_junchan (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     if (MJ_NUMBER(pair[0]) != 0 && MJ_NUMBER(pair[0]) != 8)
//         return MJ_FALSE;
    
//     for (int i = 0; i < 3; ++i)
//     {
//         if (MJ_NUMBER(MJ_FIRST(melds[i])) != 0 && MJ_NUMBER(MJ_THIRD(melds[i])) != 8)
//             return MJ_FALSE;
//     }
//     return MJ_TRUE;
// }

// mj_bool mj_chitoitsu (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     mj_id tmp[8];
//     return (mj_pairs(hand, tmp) == 7) ? MJ_TRUE : MJ_FALSE;
// }

// mj_bool mj_ittsu (mj_hand hand, mj_triple *melds, mj_pair *pair)
// {
//     int suit;
//     mj_size grace = 0;
//     for (int i = 0; i < 3; ++i)
//     {
//         suit = MJ_SUIT(MJ_FIRST(melds[i]));
//         if ((MJ_NUMBER(MJ_FIRST(melds[i+grace])) != 3*i ||
//             MJ_NUMBER(MJ_THIRD(melds[i+grace])) != 3*i+2 ||
//             suit != MJ_SUIT(MJ_THIRD(melds[i+grace]))))
//         {
//             if (++grace > 1)
//                 return MJ_FALSE;
//             suit = MJ_SUIT(MJ_THIRD(melds[i+grace]));
//         }
//     }
//     return MJ_TRUE;
// }

