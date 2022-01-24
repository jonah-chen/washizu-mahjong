#ifndef MJ_MAHJONG_H
#define MJ_MAHJONG_H

#ifndef _DEBUG_LEVEL
#define _DEBUG_LEVEL 1
#endif

#if _DEBUG_LEVEL > 0
#include <stdio.h>
#include <assert.h>
#define LOG_CRIT(...) printf(__VA_ARGS__)
#else
#define LOG_CRIT(...)
#endif

#if _DEBUG_LEVEL > 1
#define LOG_WARN(...) printf(__VA_ARGS__)
#else
#define LOG_WARN(...)
#endif

#if _DEBUG_LEVEL > 2
#define LOG_INFO(...) printf(__VA_ARGS__)
#else
#define LOG_INFO(...)
#endif

#if _DEBUG_LEVEL > 3
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

/* "Boolean" value with a maybe */
typedef unsigned char mj_bool;
/* Tile bit structure */
typedef unsigned short mj_tile;
/* Tile identifier */
typedef signed char mj_id;
/* Pair of tiles (bits again) */
typedef unsigned int mj_pair;
/* Triplet of tiles (bits again) */
typedef unsigned int mj_triple;
/* Size (size_t is too wide) */
typedef unsigned short mj_size;
/* Mahjong hand */
#define MJ_MAX_HAND_SIZE 14
typedef struct mj_hand {
    mj_tile tiles[MJ_MAX_HAND_SIZE];
    mj_size size;
} mj_hand;
/* Melds */
#define MJ_MAX_TRIPLES_IN_HAND 4
typedef struct mj_meld {
    mj_triple melds[MJ_MAX_TRIPLES_IN_HAND];
    mj_size size;
} mj_meld;

/* Constants */
#define MJ_TRUE (mj_bool)2
#define MJ_MAYBE (mj_bool)1
#define MJ_FALSE (mj_bool)0

#define MJ_CHARACTER 0
#define MJ_CIRCLE 1
#define MJ_BAMBOO 2
#define MJ_WIND 3
#define MJ_DRAGON 4

#define MJ_GREEN 0
#define MJ_RED 1
#define MJ_WHITE 2
#define MJ_EAST 0
#define MJ_SOUTH 1
#define MJ_WEST 2
#define MJ_NORTH 3

#define MJ_UNIQUE_TILES 34
#define MJ_DECK_SIZE 136
#define MJ_DEAD_WALL_SIZE 14

#define MJ_BONUS_SCORE 100
#define MJ_RIICHI_DEPOSIT 1000

#define MJ_INVALID_TILE (mj_tile)(-1)

/* Construction Macros */
#define MJ_TILE(suit,number,sub) \
((sub)|((number)<<2)|((suit)<<6))

#define MJ_128_TILE(suit,number) \
((number)|(suit<<4))

#define MJ_PAIR(x,y) \
(mj_pair)(((y)<<9)|(x))

#define MJ_TRIPLE(x,y,z) \
(mj_triple)(((z)<<18)|((y)<<9)|(x))

#define MJ_OPEN_TRIPLE(x) \
((x)|(1u<<31))

#define MJ_KONG_TRIPLE(x) \
((x)|(1u<<30))


/* Field Access Macros */
#define MJ_ID_128(x) \
(mj_id)(((x)>>2) & 0x7f)

/* Number gets the number of the first tile in the meld */
#define MJ_NUMBER(x) \
(((x)>>2) & 0b1111)

#define MJ_NUMBER1(x) \
(MJ_NUMBER(x)+1)

 /* suit can be used on both tiles and melds */
#define MJ_SUIT(x) \
(((x)>>6) & 0b111)

#define MJ_FIRST(x) \
(mj_tile)((x) & 0b111111111)

#define MJ_SECOND(x) \
(mj_tile)(((x)>>9) & 0b111111111)

#define MJ_THIRD(x) \
(mj_tile)(((x)>>18)& 0b111111111)

/* Checks */
#define MJ_IS_OPAQUE(x) \
(mj_bool)((((x) & 0b11)==0)?MJ_TRUE:MJ_FALSE)

#define MJ_IS_HONOR(x) \
(mj_bool)((MJ_SUIT(x)==MJ_WIND||MJ_SUIT(x)==MJ_DRAGON)?MJ_TRUE:MJ_FALSE)

#define MJ_IS_19(x) \
(mj_bool)((MJ_IS_HONOR(x) || MJ_NUMBER(x)==0 || MJ_NUMBER(x)==8)?MJ_TRUE:MJ_FALSE)

#define MJ_IS_19_MELD(x) \
(mj_bool)((MJ_IS_HONOR(MJ_FIRST(x)) || MJ_NUMBER(MJ_FIRST(x))==0 || MJ_NUMBER(MJ_THIRD(x))==8)?MJ_TRUE:MJ_FALSE)

#define MJ_IS_SET(x) \
(mj_bool)((MJ_ID_128(x)==MJ_ID_128(MJ_SECOND(x)))?MJ_TRUE:MJ_FALSE)

#define MJ_IS_OPEN(x) \
(mj_bool)(((x)>>30)&2)

#define MJ_IS_KONG(x) \
(mj_bool)(((x)>>29)&2)

#define MJ_TRIPLE_WEAK_EQ(x,y) \
(mj_bool)((((x)|3|3<<9|3<<18)==((y)|3|3<<9|3<<18))?MJ_TRUE:MJ_FALSE)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a string (of specified format) into a hand of tiles.
 * 
 * @note This method only parses up to the maximum length of a hand, which is 14.
 * Otherwise, the first 14 tiles will be parsed.
 * 
 * @param str The formatted string to parse.
 * @pre The string must be formatted according to the following format:
 * [numbers]m[numbers]s[numbers]p[numbers]w[numbers]d, where numbers
 * must be (1-9 for non-honors, 1-4 for wind, 1-3 for dragon).
 * Note that all suits must be specified. If no tile of that suit, [numbers] 
 * should be empty.
 * 
 * @param hand The location to store the parsed hand.
 * 
 */
void mj_parse(char const *str, mj_hand *hand);

/**
 * @brief Sort the tiles in ascending order.
 * 
 * @param hand Pointer to the hand to be sorted.
 */
void mj_sort_hand(mj_hand *hand);

/**
 * @brief Finds all possible pairs in the hand.
 * 
 * @param hand The hand to search. @pre must be sorted.
 * @param result The array of pairs to fill.
 * @return The number of pairs found.
 */
mj_size mj_pairs(mj_hand hand, mj_id *result);

/**
 * @brief Find all possible triples that can be formed from the hand.
 * 
 * @note This includes repeat tiles. (i.e. if hand contains 3 of the same tile,
 * they will be treated seperately).
 * 
 * @param hand The hand to search. @pre must be sorted.
 * @param result The array to store the triples.
 * @param capacity The maximum number of triples to store.
 * @return The number of triples found.
 */
mj_size mj_triples(mj_hand hand, mj_triple *result, mj_size capacity);

/**
 * @brief Find all possible combinations of n triples that can be formed from the hand.
 * 
 * @details This method uses depth first search over a tree. It is useful for finding
 * if a hand is winning or not, and calculating the score (especially the Fu).
 * 
 * @param hand The hand to search. @pre must be sorted.
 * @param triples The array of triples which the hand can form (or to search through).
 * @param num_triples The size of the triples array.
 * @param result The array to store the combinations.
 * @param n The number of triples to form the "winning hand".
 * @return The number of ways to form n triples.
 */
mj_size mj_n_triples(mj_hand hand, mj_triple *triples, mj_size num_triples, mj_triple *result, mj_size n);

/**
 * @brief Check the winning combinations that a hand can form.
 * 
 * @param hand The hand to check. @pre must be sorted.
 * @param o_melds The open melds the player has called.
 * @param m_result The sets of 4 melds that the is formed.
 * @param p_result The pair that the hand can form.
 * @return The number of winning combinations found.
 */
mj_size mj_n_agari(mj_hand hand, mj_meld o_melds, mj_meld *m_result, mj_pair *p_result);

/**
 * @brief Check if a hand is tenpai.
 * 
 * @details A hand is tenpai if it is one tile away from a winning hand,
 * which may allow the hand to call RON or TSUMO if the correct tile is
 * dealt.
 * 
 * @param hand The hand to check. @pre must be sorted.
 * @param o_melds The open melds the player has called.
 * @param result The IDs of different tiles the hand can win with.
 * @return The number of distinct tiles that the hand can win from.
 */
mj_size mj_tenpai(mj_hand hand, mj_meld o_melds, mj_id *result);

void mj_print_tile(mj_tile tile);
void mj_print_pair(mj_pair pair);
void mj_print_triple(mj_triple triple);
void mj_print_hand(mj_hand hand);
void mj_print_meld(mj_meld meld);

#ifdef __cplusplus
}
#endif

#endif
