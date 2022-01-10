#pragma once

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define _DEBUG_LEVEL 10

#if _DEBUG_LEVEL > 0
#include <stdio.h>
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


typedef unsigned char mj_bool;
#define MJ_TRUE (mj_bool)2
#define MJ_MAYBE (mj_bool)1
#define MJ_FALSE (mj_bool)0

typedef unsigned short mj_tile;

#define MJ_TILE(suit,number,sub) \
((sub)|((number)<<2)|((suit)<<6))

#define MJ_128_TILE(suit,number) \
((number)|(suit<<4))

#define MJ_NUMBER(x) \
(((x)>>2) & 0b1111)

#define MJ_NUMBER1(x) \
(MJ_NUMBER(x)+1)

#define MJ_SUIT(x) \
(((x)>>6) & 0b111)

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

#define MJ_OPAQUE(x) \
(((x) & 0b11)==0)

#define MJ_INVALID_TILE (mj_tile)(-1)

typedef signed char mj_id;

#define MJ_ID_128(x) \
(mj_id)(((x)>>2) & 0xff)

typedef unsigned int mj_pair;

#define MJ_PAIR(x,y) \
(mj_pair)(((y)<<9)|(x))

typedef unsigned int mj_triple;

#define MJ_TRIPLE(x,y,z) \
(mj_triple)(((z)<<18)|((y)<<9)|(x))


#define MJ_FIRST(x) \
(mj_tile)((x) & 0b111111111)

#define MJ_SECOND(x) \
(mj_tile)(((x)>>9) & 0b111111111)

#define MJ_THIRD(x) \
(mj_tile)(((x)>>18)& 0b111111111)

typedef unsigned short mj_size;

/**
 * @brief Parse a string (of specified format) into a hand of tiles.
 * 
 * @note This method only parses up to the maximum length of a hand, which is 14.
 * Otherwise, the first 14 tiles will be parsed.
 * 
 * @param str The formatted string to parse.
 * @pre The string must be formatted according to the following format:
 * [numbers]m[numbers]s[numbers]p[numbers]w[numbers]d, where numbers
 * must be in ascending order (1-9 for non-honors, 1-4 for wind, 1-3 for dragon).
 * Note that all suits must be specified. If no tile of that suit, [numbers] 
 * should be empty.
 * 
 * @param tiles The array of tiles to fill. 
 * @return The number of tiles parsed, or 0 if the string is invalid.
 */
mj_size mj_parse(char const *str, mj_tile *tiles);

/**
 * @brief Sort the tiles in ascending order.
 * 
 * @param hand the hand to sort
 * @param size the size of the hand
 */
void mj_sort_hand(mj_tile *hand, mj_size size);

/**
 * @brief Finds all possible pairs in the hand.
 * 
 * @param hand The hand to search.
 * @param size The size of the hand.
 * @param result The array of pairs to fill.
 * @return The number of pairs found.
 */
mj_size mj_pairs(mj_tile *hand, mj_size size, mj_id *result);

/**
 * @brief Determine if a PONG call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling PONG.
 * @param size The size of the hand.
 * @param tile The tile that is called.
 * @return True if the PONG call is valid, false otherwise.
 */
mj_bool mj_pong_available(mj_tile *hand, mj_size size, mj_tile const tile);

/**
 * @brief Determine if a KONG call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling KONG.
 * @param size The size of the hand.
 * @param tile The tile that is called.
 * @return True if the KONG call is valid, false otherwise.
 */
mj_bool mj_kong_available(mj_tile *hand, mj_size size, mj_tile const tile);

/**
 * @brief Determine if a CHOW call is valid on a specific tile.
 * 
 * @param hand The hand of the player calling CHOW.
 * @param size The size of the hand.
 * @param tile The tile that is called.
 * @param chow_tiles The different sets of 2 tiles that is able to 
 * chow the called tile (since there are more than 1 way to chow).
 * @return The number of different ways to chow the tile. If 0, the
 * call is invalid.
 */
mj_size mj_chow_available(mj_tile *hand, mj_size size, mj_tile const tile, mj_pair *chow_tiles);

/**
 * @brief Find all possible triples that can be formed from the hand.
 * 
 * @note This includes repeat tiles. (i.e. if hand contains 3 of the same tile,
 * they will be treated seperately).
 * 
 * @param hand The hand to search.
 * @param size The size of the hand.
 * @param result The array to store the triples.
 * @param capacity The maximum number of triples to store.
 * @return The number of triples found.
 */
mj_size mj_triples(mj_tile *hand, mj_size size, mj_triple *result, mj_size capacity);

/**
 * @brief Find all possible combinations of n triples that can be formed from the hand.
 * 
 * @details This method uses depth first search over a tree. It is useful for finding
 * if a hand is winning or not, and calculating the score (especially the Fu).
 * 
 * @param hand The hand to search.
 * @param size The size of the hand.
 * @param triples The array of triples which the hand can form (or to search through).
 * @param num_triples The size of the triples array.
 * @param result The array to store the combinations.
 * @param n The number of triples to form the "winning hand".
 * @return The number of ways to form n triples.
 */
mj_size mj_n_triples(mj_tile *hand, mj_size size, mj_triple *triples, mj_size num_triples, mj_triple *result, mj_size n);

void mj_print_tile(mj_tile tile);
void mj_print_pair(mj_pair pair);
void mj_print_triple(mj_triple triple);
void mj_print_hand(mj_tile *hand, mj_size size);

