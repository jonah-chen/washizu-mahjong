#pragma once

#include "mahjong.h"

#define MJ_BASE_FU 20
#define MJ_YAKUMAN 13

#define MJ_YAKU_RICHII 0
#define MJ_YAKU_IPPATSU 1
#define MJ_YAKU_MEN 2
#define MJ_YAKU_PINFU 3
#define MJ_YAKU_IPEIKOU 4
#define MJ_YAKU_HAITEI 5
#define MJ_YAKU_HOUTEI 6
#define MJ_YAKU_RINSHAN 7
#define MJ_YAKU_CHANKAN 8
#define MJ_YAKU_TANYAO 9
#define MJ_YAKU_YAKUHAI 10
#define MJ_YAKU_CHANTA 11
#define MJ_YAKU_SANSHOKU 12
#define MJ_YAKU_ITTSU 13
#define MJ_YAKU_TOITOI 14
#define MJ_YAKU_SANANKOU 15
#define MJ_YAKU_SANSHOKU0 16
#define MJ_YAKU_SANKANTSU 17
#define MJ_YAKU_CHIITOITSU 18
#define MJ_YAKU_HONROUTOU 19
#define MJ_YAKU_SHOUSANGEN 20
#define MJ_YAKU_HONITSU 21
#define MJ_YAKU_JUNCHAN 22
#define MJ_YAKU_RYANPEIKOU 23
#define MJ_YAKU_CHINITSU 24
#define MJ_YAKU_ARR_SIZE 25

int mj_fu(unsigned short *yakus, mj_meld const *melds, mj_pair pair, mj_tile ron, mj_bool tsumo);

typedef mj_size(*mj_yaku_check_local)(mj_hand const *, mj_meld const *, mj_pair const *, void *);
typedef mj_bool(*mj_yaku_check_global)(mj_hand const *, mj_meld const *, mj_pair const *, void *);
typedef mj_bool(*mj_yaku_check_special)(mj_hand const *);

/**
 * @warning The following yakus are not implemented here because they are not
 * dependent on the hand, but rather the other conditions in the game.
 * <ul>
 * <li>Menzenchin tsumohou</li>
 * <li>Riichi</li>
 * <li>Chankan</li>
 * <li>Dora</li>
 * <li>Uradora</li>
 * <li>Ippatsu</li>
 * <li>Haitei</li>
 * <li>Houtei</li>
 * <li>Rinshan</li>
 * <li>Pinfu (Additional Fan when fu is counted should be added to bonus)</li>
 * </li> 
 * </ul>
 * 
 */

// /* this one should be called with meld and pair being NULL */
// mj_bool mj_kokushi (mj_hand const *hand);

// mj_bool mj_chuuren (mj_hand hand, mj_triple *melds, mj_pair *pair);

// mj_bool mj_tsuuiisou (mj_hand hand, mj_triple *melds, mj_pair *pair);

// mj_bool mj_ryuuiisou (mj_hand hand, mj_triple *melds, mj_pair *pair);

// /* 6 fan */
// mj_bool mj_chinitsu (mj_hand hand, mj_triple *melds, mj_pair *pair);

// /* 3 fan */
// mj_bool mj_junchan (mj_hand hand, mj_triple *melds, mj_pair *pair);

// /* 2 fan */
// mj_bool mj_chitoitsu (mj_hand hand, mj_triple *melds, mj_pair *pair);