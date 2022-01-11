#include "mahjong.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

static void test_some_hand()
{
    mj_hand my_hand;
    mj_parse("11123345678999mpswd", &my_hand);
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

    mj_meld empty = {0,0,0,0,0};// {MJ_OPEN_TRIPLE(MJ_TRIPLE(MJ_TILE(MJ_CHARACTER, 2, 0), MJ_TILE(MJ_CHARACTER, 2, 1), MJ_TILE(MJ_CHARACTER, 2, 2))), 0, 0, 0, 1};
    mj_meld _result[100];
    mj_pair _pairs[100];

    // get the runtime of this function
    mj_size _res_size = mj_n_agari(my_hand, empty, _result, _pairs);
    mj_size  rs[10000];
    clock_t start = clock();
    for (int i = 0; i < 10000; ++i)
        mj_n_agari(my_hand, empty, _result, _pairs);
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("time spent: %f\n", time_spent);

    printf("%d different winning combinations\n", _res_size);
    for (mj_size i = 0; i < _res_size; ++i)
    {
        mj_print_meld(_result[i]);
        printf("\n");
    }
}

// static void test_complex_hand()
// {
//     mj_tile my_hand[14] = {
//         MJ_TILE(MJ_DRAGON, MJ_RED, 0),
//         MJ_TILE(MJ_DRAGON, MJ_RED, 1),
//         MJ_TILE(MJ_DRAGON, MJ_RED, 2),
//         MJ_TILE(MJ_CHARACTER, 0, 3),
//         MJ_TILE(MJ_CHARACTER, 1, 3),
//         MJ_TILE(MJ_CHARACTER, 2, 0),
//         MJ_TILE(MJ_CHARACTER, 2, 1),
//         MJ_TILE(MJ_CHARACTER, 2, 2),
//         MJ_TILE(MJ_CHARACTER, 3, 1),
//         MJ_TILE(MJ_CHARACTER, 4, 3),
//         MJ_TILE(MJ_CHARACTER, 5, 0),
//         MJ_TILE(MJ_CHARACTER, 6, 1),
//         MJ_TILE(MJ_CHARACTER, 7, 2),
//         MJ_TILE(MJ_CHARACTER, 8, 1)
//     };
//     mj_size my_hand_size = 14;
//     mj_print_hand(my_hand, my_hand_size);

//     mj_sort_hand(my_hand, my_hand_size);
//     mj_print_hand(my_hand, my_hand_size);

//     mj_id pairs[100];
//     mj_triple triples[100];
//     mj_size pairs_size = mj_pairs(my_hand, my_hand_size, pairs);
//     mj_size triples_size = mj_triples(my_hand, my_hand_size, triples, 100);
//     for (mj_size i = 0; i < triples_size; ++i)
//     {
//         mj_print_triple(triples[i]);
//         printf("\n");
//     }

//     printf("\n\n");
//     mj_triple result[100];
//     mj_size res_size = mj_n_triples(my_hand, my_hand_size, triples, triples_size, result, 4);
    
//     printf("%d\n", res_size);
//     for (mj_size i = 0; i < res_size; ++i)
//     {
//         mj_print_triple(result[i]);
//         printf("\n");
//     }
// }
int main(int argc, char *argv[])
{
    test_some_hand();

    return 0;
}
