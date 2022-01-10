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
 * Sort
 * @param hand
 * @param size
 */
void mj_sort_hand(mj_tile *hand, mj_size size);

mj_size mj_pairs(mj_tile *hand, mj_size size, mj_id *result);

mj_bool mj_pong_available(mj_tile *hand, mj_size size, mj_tile const tile);

mj_bool mj_kong_available(mj_tile *hand, mj_size size, mj_tile const tile);

mj_size mj_chow_available(mj_tile *hand, mj_size size, mj_tile const tile, mj_pair *chow_tiles);

mj_size mj_triples(mj_tile *hand, mj_size size, mj_triple *result, mj_size capacity);

mj_size mj_n_triples(mj_tile *hand, mj_size size, mj_triple *triples, mj_size num_triples, mj_triple *result, mj_size n);


void mj_print_tile(mj_tile tile);
void mj_print_pair(mj_pair pair);
void mj_print_triple(mj_triple triple);
void mj_print_hand(mj_tile *hand, mj_size size);

