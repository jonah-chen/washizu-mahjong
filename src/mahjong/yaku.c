#include "yaku.h"

static unsigned short *yakus;
static mj_bool closed;

/**
 * Closed Only.
 * Prereqs: Ryanpeikou 
 */
static inline void one_sequence(mj_meld const *melds)
{
    if (closed)
    {
        for (mj_size i = 0; i < melds->size - 1; ++i)
        {
            if (MJ_ID_128(melds->melds[i]) == MJ_ID_128(melds->melds[i + 1]))
            {
                yakus[MJ_YAKU_IPEIKOU] = 1;
                return;
            }
        }
    }
}

/**
 * Open or Closed.
 * Prereqs: None.
 */
static inline void tanyao(mj_meld const *melds, mj_pair pair)
{
    for (mj_size i = 0; i < melds->size; ++i)
    {
        if (MJ_IS_19_MELD(melds->melds[i]))
        {
            return;
        }
    }
    yakus[MJ_YAKU_TANYAO] = MJ_IS_19(MJ_FIRST(pair)) ? 1 : 0;
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void yakuhai(mj_meld const *melds, int prevailing_wind, int seat_wind)
{
    for (mj_size i = 0; i < melds->size; ++i)
    {
        if (MJ_SUIT(melds->melds[i]) == MJ_DRAGON)
        {
            yakus[MJ_YAKU_YAKUHAI]++;
        }
        if (MJ_SUIT(melds->melds[i]) == MJ_WIND)
        {
            if (MJ_NUMBER(melds->melds[i]) == prevailing_wind)
            {
                yakus[MJ_YAKU_YAKUHAI]++;
            }
            if (MJ_NUMBER(melds->melds[i]) == seat_wind)
            {
                yakus[MJ_YAKU_YAKUHAI]++;
            }
        }
    }
}

/**
 * Closed or Open.
 * Prereqs: Pure must be called first with MJ_TRUE before MJ_FALSE. 
 * all_terminals must be called before calling with MJ_FALSE.
 */
static inline void chk_19(mj_meld const *melds, mj_pair pair, mj_bool pure)
{
    if (MJ_IS_19(pair)&&!(pure&&MJ_IS_HONOR(pair)))
    {
        for (mj_size i = 0; i < melds->size; ++i)
        {
            if (!MJ_IS_19_MELD(melds->melds[i]) && !(pure&&MJ_IS_HONOR(melds->melds[i])))
            {
                return;
            }
        }
        if (pure)
            yakus[MJ_YAKU_JUNCHAN] = closed ? 3 : 2;
        else if (yakus[MJ_YAKU_JUNCHAN])
            yakus[MJ_YAKU_CHANTA] = closed ? 2 : 1;
    }
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void three_identical(mj_meld const *melds)
{
    unsigned char suits_set[9] = {0,0,0,0,0,0,0,0,0};
    unsigned char suits_seq[9] = {0,0,0,0,0,0,0,0,0};

    for (mj_size i = 0; i < melds->size; ++i)
    {
        mj_triple const cur = melds->melds[i];
        int number = MJ_NUMBER(cur);
        if (MJ_IS_SET(cur))
        {
            switch(MJ_SUIT(cur))
            {
                case MJ_CHARACTER:
                    suits_set[number] |= 0b001;
                    break;
                case MJ_BAMBOO:
                    suits_set[number] |= 0b010;
                    break;
                case MJ_CIRCLE:
                    suits_set[number] |= 0b100;
                    break;
            }
        }
        else
        {
            switch(MJ_SUIT(cur))
            {
                case MJ_CHARACTER:
                    suits_seq[number] |= 0b001;
                    break;
                case MJ_BAMBOO:
                    suits_seq[number] |= 0b010;
                    break;
                case MJ_CIRCLE:
                    suits_seq[number] |= 0b100;
                    break;
            }
        }
    }


    // the number has to be one of the first 2
    int first_number = MJ_NUMBER(melds->melds[0]);
    int second_number = MJ_NUMBER(melds->melds[1]);

    if (suits_seq[first_number]==0b111||suits_seq[second_number]==0b111)
        yakus[MJ_YAKU_SANANKOU] = closed ? 2 : 1;
    else if (suits_set[first_number]==0b111||suits_set[second_number]==0b111)
        yakus[MJ_YAKU_SANSHOKU0] = 2;
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void straight(mj_meld const *melds)
{
    unsigned char suits[3] = {0, 0, 0};
    for (mj_size i = 0; i < melds->size; ++i)
    {
        mj_triple const cur = melds->melds[i];
        if (!MJ_IS_SET(cur))
        {
            switch(MJ_NUMBER(cur))
            {
                case 0:
                    suits[MJ_SUIT(cur)] |= 0b001;
                    break;
                case 3:
                    suits[MJ_SUIT(cur)] |= 0b010;
                    break;
                case 6:
                    suits[MJ_SUIT(cur)] |= 0b100;
                    break;
                default:
                    break;
            }
        }
    }

    if (suits[0]==0b111||suits[1]==0b111||suits[2]==0b111)
        yakus[MJ_YAKU_ITTSU] = closed ? 2 : 1;
}

/**
 * Closed or Open.
 * Prereqs: None.
 */ 
static inline void all_sets(mj_meld const *melds)
{
    for (mj_size i = 0; i < melds->size; ++i)
    {
        if (!MJ_IS_SET(melds->melds[i]))
        {
            return;
        }
    }
    yakus[MJ_YAKU_TOITOI] = 2;
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void three_closed_sets(mj_meld const *melds)
{
    int sets = 0;
    for (mj_size i = 0; i < melds->size; ++i)
    {
        if (MJ_IS_SET(melds->melds[i]) && !MJ_OPEN_TRIPLE(melds->melds[i]))
        {
            ++sets;
        }
    }
    if (sets == 3)
    {
        yakus[MJ_YAKU_SANANKOU] = 2;
    }
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void three_kongs(mj_meld const *melds)
{
    int kongs = 0;
    for (mj_size i = 0; i < melds->size; ++i)
    {
        if (MJ_IS_KONG(melds->melds[i]))
        {
            ++kongs;
        }
    }
    if (kongs == 3)
    {
        yakus[MJ_YAKU_SANKANTSU] = 2;
    }
}

static inline void all_terminals(mj_meld const *melds, mj_pair pair)
{
    for (mj_size i = 0; i < melds->size; ++i)
    {
        if (!(MJ_IS_SET(melds->melds[i])||MJ_IS_19(melds->melds[i])))
        {
            return;
        }
    }
    if (MJ_IS_19(pair))
    {
        yakus[MJ_YAKU_HONROUTOU] = 2;
    }
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void three_dragon(mj_meld const *meld, mj_pair pair)
{
    // pair must be a dragon
    if (MJ_SUIT(pair)!=MJ_DRAGON)
        return;
    
    int dragons = 0;

    for (mj_size i = 0; i < meld->size; ++i)
        if (MJ_SUIT(meld->melds[i])==MJ_DRAGON)
            ++dragons;

    if (dragons == 2)
        yakus[MJ_YAKU_SHOUSANGEN] = 2;
}

/**
 * Closed or Open.
 * Prereqs: None.
 */
static inline void flush(mj_meld const *meld, mj_pair pair)
{
    int suit = -1;
    mj_bool full = MJ_TRUE;
    for (mj_size i = 0; i < meld->size; ++i)
    {
        // if i find a honor, it's not full
        if (MJ_IS_HONOR(meld->melds[i]))
        {
            full = MJ_FALSE;
        }
        // otherwise the suit
        else if (suit == -1)
        {
            suit = MJ_SUIT(meld->melds[i]);
        }
        else if  (suit != MJ_SUIT(meld->melds[i]))
        {
            return;
        }
    }

    if (full && MJ_SUIT(pair)==suit)
        yakus[MJ_YAKU_CHINITSU] = closed ? 6 : 5;
    else if (MJ_SUIT(pair)==suit||MJ_IS_HONOR(pair))
        yakus[MJ_YAKU_HONITSU] = closed ? 3 : 2;
}



/**
 * Closed Only.
 * Prereqs: None
 */
static void two_sequence(mj_meld const *melds)
{
    // check first 2
    yakus[MJ_YAKU_RYANPEIKOU] = closed &&
        MJ_ID_128(MJ_FIRST(melds->melds[0])) ==
        MJ_ID_128(MJ_FIRST(melds->melds[1])) &&
        MJ_ID_128(MJ_FIRST(melds->melds[2])) ==
        MJ_ID_128(MJ_FIRST(melds->melds[3])) ? 3 : 0;
}



int mj_fu(unsigned short *_yakus, mj_meld const *melds, mj_pair pair, mj_tile ron, mj_bool tsumo)
{
    unsigned short *_tmp = _yakus;
    yakus = _yakus;
    closed = MJ_TRUE;
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
                triple_points <<= 1;

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
        if (closed)
        {
            yakus[MJ_YAKU_PINFU] = 1;
            yakus = _tmp;
            return tsumo ? 20 : 30;
        }
        else return 30;
    }

    if (tsumo)
        fu += 2;
    else if (closed)
        fu += 10;

    yakus = _tmp;

    return 10 * ((fu + 9) / 10); // round up
}

int mj_fan(unsigned short *_yakus, mj_meld const *melds, mj_pair pair, int prevailing_wind, int seat_wind)
{
    unsigned short *_tmp = yakus; 
    yakus = _yakus;

    two_sequence(melds);
    one_sequence(melds);
    tanyao(melds, pair);
    yakuhai(melds, prevailing_wind, seat_wind);
    all_terminals(melds, pair);
    chk_19(melds, pair, MJ_TRUE);
    chk_19(melds, pair, MJ_FALSE);
    three_identical(melds);
    straight(melds);
    all_sets(melds);
    three_closed_sets(melds);
    three_kongs(melds);
    three_dragon(melds, pair);
    flush(melds, pair);

    int score = 0;
    for (int i = 0; i < MJ_YAKU_ARR_SIZE; ++i)
    {
        score += yakus[i];
    }
    yakus = _tmp;
    return score;
}

int mj_seven_pairs(unsigned short *_yakus, mj_hand const *hand)
{
    if (hand->size != MJ_MAX_HAND_SIZE)
        return 0;
    
    if (mj_pairs(*hand, NULL) != 7)
        return 0;
    
    unsigned short *_tmp = yakus;
    yakus = _yakus;
    yakus[MJ_YAKU_CHIITOITSU] = 2;

    // we need to deal with these yaku
    mj_bool full_flush = MJ_TRUE, half_flush = MJ_FALSE, term = MJ_TRUE, tanyao = MJ_TRUE;
    int suit = -1;
    for (int i = 0; i < MJ_MAX_HAND_SIZE; ++i)
    {
        mj_tile tile = hand->tiles[i];
        if (MJ_IS_HONOR(tile))
        {
            full_flush = MJ_FALSE;
        }
        else if (suit == -1)
        {
            suit = MJ_SUIT(tile);
        }
        else if (suit != MJ_SUIT(tile))
        {
            full_flush = MJ_FALSE;
            half_flush = MJ_FALSE;
        }

        if (MJ_IS_19(tile))
            tanyao = MJ_FALSE;
        else
            term = MJ_FALSE;
    }

    if (full_flush)
        yakus[MJ_YAKU_CHINITSU] = 6;
    else if (half_flush)
        yakus[MJ_YAKU_HONITSU] = 3;
    else if (term)
        yakus[MJ_YAKU_HONROUTOU] = 2;
    else if (tanyao)
        yakus[MJ_YAKU_TANYAO] = 1;
    
    int fan;
    for (int i = 0; i < MJ_YAKU_ARR_SIZE; ++i)
    {
        fan += yakus[i];
    }
    yakus = _tmp;
    return fan;
}

inline int mj_basic_score(int fu, int fan)
{
    if (fan < 5)
    {
        return fu << (2 + fan);
    }
    else if (fan < 6)
    {
        return MJ_MANGAN;
    }
    else if (fan < 8)
    {
        return MJ_HANEMAN;
    }
    else if (fan < 11)
    {
        return MJ_BAIMAN;
    }
    else
    {
        return MJ_SANBAIMAN;
    }
}

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
