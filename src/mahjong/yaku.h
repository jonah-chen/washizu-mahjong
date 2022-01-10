#pragma once

#include "mahjong.h"

/* this one should be called with meld and pair being NULL */
mj_bool mj_kokushi (mj_tile *hand, mj_triple *melds, mj_pair *pair);

mj_bool mj_chuuren (mj_tile *hand, mj_triple *melds, mj_pair *pair);

mj_bool mj_tsuuiisou (mj_tile *hand, mj_triple *melds, mj_pair *pair);

mj_bool mj_ryuuiisou (mj_tile *hand, mj_triple *melds, mj_pair *pair);

/* 6 fan */
mj_bool mj_chinitsu (mj_tile *hand, mj_triple *melds, mj_pair *pair);

/* 3 fan */
mj_bool mj_junchan (mj_tile *hand, mj_triple *melds, mj_pair *pair);

/* 2 fan */
mj_bool mj_chitoitsu (mj_tile *hand, mj_triple *melds, mj_pair *pair);