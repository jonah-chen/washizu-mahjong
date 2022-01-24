#define _DEBUG_LEVEL 3

#include "mahjong.h"
#include "yaku.h"
#include "interaction.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void test_hand(int expected_fu, int expected_fan, int expected_combo,
    mj_tile ron, mj_bool tsumo, int prevailing_wind, int seat_wind,
    char const *hand_str, char const *meld1, char const *meld2, char const *meld3, char const *meld4)
{
    mj_hand hand;
    unsigned short yakus[MJ_YAKU_ARR_SIZE];
    mj_meld melds = {0,0,0,0,0};
    char const *melds_str[] = {meld1, meld2, meld3, meld4};
    for (int i = 0; i < 4; ++i)
    {
        if (melds_str[i] == NULL)
            continue;

        mj_parse(melds_str[i], &hand);
        if (hand.size == 4)
        {
            melds.melds[i] = MJ_KONG_TRIPLE(MJ_OPEN_TRIPLE(MJ_TRIPLE(
                hand.tiles[0], hand.tiles[1], hand.tiles[2])));
            ++melds.size;
        }
        else if (hand.size == 3)
        {
            melds.melds[i] = MJ_OPEN_TRIPLE(MJ_TRIPLE(
                hand.tiles[0], hand.tiles[1], hand.tiles[2]));
            ++melds.size;
        }
        else assert(0);
    }

    mj_parse(hand_str, &hand);

    mj_meld result[64];
    mj_pair pairs[16];
    mj_size n = mj_n_agari(hand, melds, result, pairs);
    assert(n == expected_combo);

    int fu, fan;
    memset(yakus, 0, sizeof(yakus));
    fu = mj_fu(yakus, result, pairs[0], ron, tsumo, prevailing_wind, seat_wind);
    fan = mj_fan(yakus, result, pairs[0], prevailing_wind, seat_wind);
#if _DEBUG_LEVEL > 2
    mj_print_hand(hand);
    LOG_INFO("Fu: %d, Fan: %d\n", fu, fan);
    mj_print_yaku(yakus);
    LOG_INFO("\n");
#endif
    assert(expected_fu == fu);
    assert(expected_fan == fan);
}

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

static void open_ipeikou()
{

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

static void test_chow(char const *hand_str, mj_tile chow_tile, mj_size chows)
{
    mj_hand hand;
    mj_parse(hand_str, &hand);
    assert(mj_chow_available(hand, chow_tile, NULL) == chows);
}

int main(int argc, char *argv[])
{
    mj_hand hand;
    mj_meld empty = {0,0,0,0,0};
    mj_parse("12345678m333ps22wd", &hand);
    assert(mj_tenpai(hand,empty,NULL)==3);


    test_hand(30, 1, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "123345567m123ps22wd", NULL, NULL, NULL, NULL); // pinfu only

    test_hand(30, 1, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_TRUE, MJ_EAST, MJ_EAST,
    "123345567m333ps22wd", NULL, NULL, NULL, NULL); // tsumo only

    test_hand(20, 2, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_TRUE, MJ_EAST, MJ_EAST,
    "123345567m567ps22wd", NULL, NULL, NULL, NULL); // pinfu tsumo

    test_hand(30, 1, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_TRUE, MJ_EAST, MJ_EAST,
    "123345567m123ps11wd", NULL, NULL, NULL, NULL); // east wind pair

    test_hand(40, 0, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "123345567m123psw22d", NULL, NULL, NULL, NULL); // dragon pair, 0 fan

    test_hand(30, 2, 1, MJ_TILE(MJ_CHARACTER, 1, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "22334499m123p678swd", NULL, NULL, NULL, NULL); // ipeikou

    test_hand(40, 7, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "23444456677888mpswd", NULL, NULL, NULL, NULL); // chitoitsu, tanyao

    test_hand(40, 2, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "123789m123p789sw22d", NULL, NULL, NULL, NULL); // chanta only

    test_hand(30, 1, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "123789m123psw22d", "mp789swd", NULL, NULL, NULL); // chanta open

    test_hand(50, 3, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "123345m55ps111w222d", NULL, NULL, NULL, NULL); // 3xYakuhai

    test_hand(50, 2, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_SOUTH,
    "123345m55ps111w222d", NULL, NULL, NULL, NULL); // 2xYakuhai

    test_hand(50, 1, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_SOUTH, MJ_WEST,
    "123345m55ps111w222d", NULL, NULL, NULL, NULL); // 1xYakuhai

    test_hand(40, 2, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "123456789m55ps222wd", NULL, NULL, NULL, NULL); // ittsu

    test_hand(40, 2, 1, MJ_TILE(MJ_CHARACTER, 2, 0), MJ_FALSE, MJ_EAST, MJ_EAST,
    "33mpswd", "555mpswd", "mps333wd", "m444pswd", "mp888swd"); // toitoi, 4 open


    pinfu_only();
    open_pinfu();

    test_chow("12567m123456ps22wd", MJ_TILE(MJ_CHARACTER, 2, 0), 1);

    test_chow("2467m1267p6s22w33d", MJ_TILE(MJ_CHARACTER, 4, 0), 2);

    test_chow("5m4678p1s112234w33d", MJ_TILE(MJ_CIRCLE, 6, 0), 1);

    test_chow("5m4678p1s112234w33d", MJ_TILE(MJ_CIRCLE, 4, 0), 2);

    return 0;
}
