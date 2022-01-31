#ifndef MJ_YAKU_H
#define MJ_YAKU_H

#include "mahjong.h"

/* Scores */
#define MJ_BASE_FU 20
#define MJ_YAKUMAN 13
#define MJ_MANGAN 2000
#define MJ_HANEMAN 3000
#define MJ_BAIMAN 4000
#define MJ_SANBAIMAN 6000

/* Yaku */
#define MJ_YAKU_RICHII 0 // external
#define MJ_YAKU_IPPATSU 1 // external
#define MJ_YAKU_MEN 2 // fu
#define MJ_YAKU_PINFU 3 // fu
#define MJ_YAKU_IPEIKOU 4 // one_sequence
#define MJ_YAKU_HAITEI 5 // external
#define MJ_YAKU_HOUTEI 6 // external
#define MJ_YAKU_RINSHAN 7 // external
#define MJ_YAKU_CHANKAN 8 // external
#define MJ_YAKU_TANYAO 9 // tanyao
#define MJ_YAKU_YAKUHAI 10 // yakuhai
#define MJ_YAKU_CHANTA 11 // chk_19 (with MJ_FALSE)
#define MJ_YAKU_SANSHOKU 12 // three_identical (2 in 1)
#define MJ_YAKU_ITTSU 13 // straight
#define MJ_YAKU_TOITOI 14 // all_sets
#define MJ_YAKU_SANANKOU 15 // three_closed_sets
#define MJ_YAKU_SANSHOKU0 16 // three_identical (2 in 1)
#define MJ_YAKU_SANKANTSU 17 // three_kongs
#define MJ_YAKU_CHIITOITSU 18 // seven_pairs
#define MJ_YAKU_HONROUTOU 19 // all_terminals
#define MJ_YAKU_SHOUSANGEN 20 // three_dragon
#define MJ_YAKU_HONITSU 21 // flush (2 in 1)
#define MJ_YAKU_JUNCHAN 22 // chk_19 (with MJ_TRUE)
#define MJ_YAKU_RYANPEIKOU 23 // two_sequence
#define MJ_YAKU_CHINITSU 24 // flush (2 in 2)
#define MJ_YAKU_DORA 25 // external
#define MJ_YAKU_ARR_SIZE 26

#ifdef __cplusplus
extern "C" {
#endif

static char const *const MJ_YAKU_NAMES[MJ_YAKU_ARR_SIZE] = {
    "Richii", // 0
    "Ippatsu", // 1
    "Menzenchin Tsumohou", // 2
    "Pinfu", // 3
    "Ipeikou", // 4
    "Haitei", // 5
    "Houtei", // 6
    "Rinshan", // 7
    "Chankan", // 8
    "Tanyao", // 9
    "Yakuhai", // 10
    "Chanta", // 11
    "Sanshoku doujun", // 12
    "Ittsu", // 13
    "Toitoi", // 14
    "Sanankou", // 15
    "Sanshoku doukou", // 16
    "Sankantsu", // 17
    "Chiitoitsu", // 18
    "Honroutou", // 19
    "Shousangen", // 20
    "Honitsu", // 21
    "Junchantai", // 22
    "Ryanpeikou", // 23
    "Chinitsu" // 24
};

/**
 * @brief Count the fu of a hand.
 * @requires The hand is already known to not be yakuman or 7 pairs.
 *
 * @warning This
 *
 * @param yakus The array to store the yakus.
 * @param melds The melds the hand formed.
 * @param pair The pair of the hand.
 * @param ron The tile the hand won on.
 * @param tsumo Whether the hand is won by tsumo.
 * @return The number of fu. It is at least 20.
 */
int mj_fu(unsigned short *_yakus, mj_meld const *melds, mj_pair pair, mj_tile ron, mj_bool tsumo, int prevailing_wind, int seat_wind);

/**
 * @brief Count all the possible yakus in a hand.
 * @requires The hand is already known to not be yakuman and 7 pairs.
 * @requires The hand to be already counted for fu.
 *
 * @param _yakus The array to store the yakus.
 * @param melds The melds the hand formed.
 * @param pair The pair of the hand.
 * @param prevailing_wind The prevailing wind of the round.
 * @param seat_wind The player's seat wind.
 * @return The number of fan.
 */
int mj_fan(unsigned short *_yakus, mj_meld const *melds, mj_pair pair, int prevailing_wind, int seat_wind);

/**
 * @brief Check all the possible yakus in a hand, assuming that it is seven pairs.
 *
 * @param _yakus The array to store the yakus.
 * @param hand The hand to check.
 * @return The number of fan, or 0 if the hand is not seven pairs.
 */
int mj_seven_pairs(unsigned short *_yakus, mj_hand const *hand);

int mj_yakuman(unsigned short *yakus, mj_meld const *melds, mj_pair pair, mj_tile ron, mj_bool tsumo);

/**
 * @brief Calculate the basic score of a hand.
 *
 * @details Payments in the game is based on the basic score. Usually,
 * a player gets 4 times the basic score and the dealer gets 6 times
 * the basic score.
 *
 * @param fu The number of fu.
 * @param fan The number of fan.
 * @return The basic score of the hand.
 */
int mj_basic_score(int fu, int fan);

int mj_score(int *fu, int *fan, unsigned short *yakus,
    mj_hand const *hand, mj_meld const *melds, mj_tile ron, mj_bool tsumo,
    int prevailing_wind, int seat_wind);

void mj_print_yaku(unsigned short const *yakus);

#ifdef __cplusplus
}
#endif

#endif
