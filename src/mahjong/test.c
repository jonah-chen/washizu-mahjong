#include "mahjong.h"
#include "yaku.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void pinfu_only()
{
    mj_hand hand;
    mj_parse("345m234p12355678swd", &hand);
    unsigned short yakus[MJ_YAKU_ARR_SIZE];
    mj_meld empty = {0,0,0,0,0};
    mj_meld result[64];
    mj_pair pairs[16];
    mj_size n = mj_n_agari(hand, empty, result, pairs);
    assert(n == 1);
    
    int fu, fan;

    memset(yakus, 0, sizeof(yakus));
    fu = mj_fu(yakus, result, pairs[0], hand.tiles[0], MJ_FALSE, MJ_EAST, MJ_EAST);
    fan = mj_fan(yakus, result, pairs[0], MJ_EAST, MJ_EAST);
    assert(fu == 30);
    assert(fan == 1);

    memset(yakus, 0, sizeof(yakus));
    fu = mj_fu(yakus, result, pairs[0], hand.tiles[0], MJ_TRUE, MJ_EAST, MJ_EAST);
    fan = mj_fan(yakus, result, pairs[0], MJ_EAST, MJ_EAST);
    assert(fu == 20);
    assert(fan == 2);

    memset(yakus, 0, sizeof(yakus));
    fu = mj_fu(yakus, result, pairs[0], hand.tiles[0], MJ_FALSE, MJ_EAST, MJ_EAST);
    fan = mj_fan(yakus, result, pairs[0], MJ_EAST, MJ_EAST);
    assert(fu == 30);
    assert(fan == 1);
}

static void open_pinfu()
{
    mj_hand hand;
    mj_parse("345m234p12355swd", &hand);
    unsigned short yakus[MJ_YAKU_ARR_SIZE];
    mj_meld sixman = {MJ_OPEN_TRIPLE(MJ_TRIPLE(MJ_TILE(MJ_CHARACTER,6,0),MJ_TILE(MJ_CHARACTER,6,1),MJ_TILE(MJ_CHARACTER,6,2))),0,0,0,1};
    mj_meld result[64];
    mj_pair pairs[16];
    mj_size n = mj_n_agari(hand, sixman, result, pairs);
    assert(n == 1);
    
    int fu, fan;

    memset(yakus, 0, sizeof(yakus));
    fu = mj_fu(yakus, result, pairs[0], hand.tiles[0], MJ_FALSE, MJ_EAST, MJ_EAST);
    fan = mj_fan(yakus, result, pairs[0], MJ_EAST, MJ_EAST);
    assert(fu == 30);
    assert(fan == 0);
}

static void test_some_hand()
{
    mj_hand my_hand;
    mj_parse("123345567m333ps22wd", &my_hand);
    mj_print_hand(my_hand);
    // mj_id pairs[100];
    // mj_triple triples[100], result[40];
    // mj_size pairs_size = mj_pairs(my_hand, pairs);
    // printf("pairs found %d\n", pairs_size);
    // mj_size triples_size = mj_triples(my_hand, triples, 100);
    // mj_size res_size = mj_n_triples(my_hand, triples, triples_size, result, 4);
    // printf("%d\n", res_size);
    // for (mj_size i = 0; i < res_size; ++i)
    // {
    //     mj_print_triple(result[i]);
    //     printf("\n");
    // }
    unsigned short yakus[MJ_YAKU_ARR_SIZE];
    memset(yakus, 0, sizeof(yakus));

    mj_meld empty = {0,0,0,0,0};// {MJ_OPEN_TRIPLE(MJ_TRIPLE(MJ_TILE(MJ_CHARACTER, 2, 0), MJ_TILE(MJ_CHARACTER, 2, 1), MJ_TILE(MJ_CHARACTER, 2, 2))), 0, 0, 0, 1};
    mj_meld _result[100];
    mj_pair _pairs[100];

    // get the runtime of this function
    mj_size _res_size = mj_n_agari(my_hand, empty, _result, _pairs);
    yakus[MJ_YAKU_RICHII] = 1;
    int fu = mj_fu(yakus, _result, _pairs[0], MJ_TILE(MJ_CHARACTER, 3, 0), MJ_FALSE, MJ_EAST, MJ_EAST);
    int fan = mj_fan(yakus, _result, _pairs[0], MJ_EAST, MJ_EAST);
    printf("Fu: %d\n", fu);
    printf("Fan: %d\n", fan);
    printf("Basic score: %d\n", mj_basic_score(fu, fan));
    mj_print_yaku(yakus);
}

int main(int argc, char *argv[])
{
    pinfu_only();
    open_pinfu();
    test_some_hand();

    return 0;
}
