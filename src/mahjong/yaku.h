#pragma once

#include "mahjong.h"

#define MJ_BASE_FU 20

int mj_fu(mj_meld const *melds, mj_pair pair, mj_tile ron);

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
 */



/* this one should be called with meld and pair being NULL */
mj_bool mj_kokushi (mj_hand const *hand);

mj_bool mj_chuuren (mj_hand hand, mj_triple *melds, mj_pair *pair);

mj_bool mj_tsuuiisou (mj_hand hand, mj_triple *melds, mj_pair *pair);

mj_bool mj_ryuuiisou (mj_hand hand, mj_triple *melds, mj_pair *pair);

/* 6 fan */
mj_bool mj_chinitsu (mj_hand hand, mj_triple *melds, mj_pair *pair);

/* 3 fan */
mj_bool mj_junchan (mj_hand hand, mj_triple *melds, mj_pair *pair);

/* 2 fan */
mj_bool mj_chitoitsu (mj_hand hand, mj_triple *melds, mj_pair *pair);