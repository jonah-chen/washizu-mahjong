#include "mahjong.h"

int main(int argc, char *argv[])
{
    mj_tile my_hand[14] = {
        MJ_TILE(MJ_DRAGON, MJ_RED, 0),
        MJ_TILE(MJ_DRAGON, MJ_RED, 1),
        MJ_TILE(MJ_DRAGON, MJ_RED, 2),
        MJ_TILE(MJ_CHARACTER, 1, 3),
        MJ_TILE(MJ_CHARACTER, 2, 0),
        MJ_TILE(MJ_CHARACTER, 2, 1),
        MJ_TILE(MJ_CHARACTER, 2, 2),
        MJ_TILE(MJ_CHARACTER, 3, 1),
        MJ_TILE(MJ_CHARACTER, 4, 3),
        MJ_TILE(MJ_CHARACTER, 5, 0),
        MJ_TILE(MJ_CHARACTER, 6, 1),
        MJ_TILE(MJ_CHARACTER, 7, 2),
        MJ_TILE(MJ_CHARACTER, 8, 1),
        MJ_TILE(MJ_CHARACTER, 0, 3)
    };
    mj_size my_hand_size = 14;
    mj_print_hand(my_hand, my_hand_size);

    mj_sort_hand(my_hand, my_hand_size);
    mj_print_hand(my_hand, my_hand_size);

    mj_id pairs[100];
    mj_triple triples[100];
    mj_size pairs_size = mj_pairs(my_hand, my_hand_size, pairs);
    mj_size triples_size = mj_triples(my_hand, my_hand_size, triples, 100);
    for (mj_size i = 0; i < triples_size; ++i)
    {
        mj_print_triple(triples[i]);
        printf("\n");
    }
    return 0;
}