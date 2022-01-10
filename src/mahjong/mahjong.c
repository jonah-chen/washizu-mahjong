#include "mahjong.h"
#include <stdlib.h>
#include <string.h>

#define MAX_CAPACITY 1000

void mj_parse(char const *str, mj_hand *hand)
{
    static char const mj_suit_strings[5] = {'m','p','s','w','d'}; 

    hand->size = 0;
    int cur_suit = 0, cur_sub;
    for (; *str && hand->size < MJ_MAX_HAND_SIZE; str++)
    {
        if (*str < '1' || *str > '9')
        {
            if (mj_suit_strings[cur_suit++] != *str)
            {
                hand->size = 0;
                return;
            }
            else if (cur_suit == 5)
                return;
        }
        else 
        {
            if (hand->size && hand->tiles[hand->size-1] == MJ_TILE(cur_suit, *str - '1', cur_sub))
                cur_sub++;
            else
                cur_sub = 0;

            hand->tiles[hand->size++] = MJ_TILE(cur_suit, *str - '1', cur_sub);
        }
    }
}

void mj_sort_hand(mj_hand *hand)
{
    mj_size i, j;
    mj_tile tmp;
    for (i = 1; i < hand->size; ++i)
    {
        for (j = i; j > 0 && hand->tiles[j-1] > hand->tiles[j]; --j)
        {
            tmp = hand->tiles[j];
            hand->tiles[j] = hand->tiles[j-1];
            hand->tiles[j-1] = tmp;
        }
    }
}

mj_size mj_pairs(mj_hand hand, mj_id *result)
{
    mj_size i, pairs = 0;
    
    for (i = 0; i < hand.size-1; ++i)
    {
        if (MJ_ID_128(hand.tiles[i]) == MJ_ID_128(hand.tiles[i+1]))
        {
            result[pairs++] = MJ_ID_128(hand.tiles[i++]);
        }
    }
    LOG_DEBUG("pairs: %d\n", pairs);
    return pairs;
}

mj_bool mj_pong_available(mj_hand hand, mj_tile const tile)
{
    for (mj_size i = 0; i < hand.size-1 && MJ_ID_128(hand.tiles[i]) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(hand.tiles[i]) == MJ_ID_128(hand.tiles[i+1]) &&
            MJ_ID_128(hand.tiles[i]) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_bool mj_kong_available(mj_hand hand, mj_tile const tile)
{
    for (mj_size i = 0; i < hand.size-2 && MJ_ID_128(hand.tiles[i]) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(hand.tiles[i]) == MJ_ID_128(hand.tiles[i+1]) &&
            MJ_ID_128(hand.tiles[i]) == MJ_ID_128(hand.tiles[i+2]) &&
            MJ_ID_128(hand.tiles[i]) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_size mj_chow_available(mj_hand hand, mj_tile const tile, mj_pair *chow_tiles)
{
    if (MJ_SUIT(tile) == MJ_WIND || MJ_SUIT(tile) == MJ_DRAGON)
    {
        return 0;
    }

    mj_size i, j, chows = 0;

    for (i = 0; i < hand.size-1; ++i)
    {
        int difference; 
        if (MJ_SUIT(hand.tiles[i]) == MJ_SUIT(tile))
        {
            switch (MJ_NUMBER(hand.tiles[i]) - MJ_NUMBER(tile))
            {
                case -2: difference = -1; break;
                case -1: difference = 1; break;
                case 1: difference = 2; break;
                default: continue;
            }

            for (j = i+1; 
                 j < hand.size && 
                 MJ_SUIT(hand.tiles[j]) == MJ_SUIT(tile) && 
                 MJ_NUMBER(hand.tiles[j])>MJ_NUMBER(tile)+difference;
                 ++j)
            {
                if (MJ_NUMBER(hand.tiles[j]) == MJ_NUMBER(tile) + difference)
                {
                    chow_tiles[chows++] = MJ_PAIR(hand.tiles[i], hand.tiles[j]);
                }
            }
        }
    }

    return chows;
}

mj_size mj_triples(mj_hand hand, mj_triple *result, mj_size capacity)
{
    mj_size i, j, k, triples = 0;
    
    for (i = 0; i < hand.size-2 && triples < capacity; ++i)
    {
        if (MJ_ID_128(hand.tiles[i]) == MJ_ID_128(hand.tiles[i+1]) &&
            MJ_ID_128(hand.tiles[i]) == MJ_ID_128(hand.tiles[i+2]))
        {
            result[triples++] = MJ_TRIPLE(hand.tiles[i], hand.tiles[i+1], hand.tiles[i+2]);
            j = i + 3;
        }
        else 
        {
            j = i + 1;
        }

        if (MJ_SUIT(hand.tiles[i])==MJ_WIND || MJ_SUIT(hand.tiles[i])==MJ_DRAGON ||
            MJ_NUMBER1(hand.tiles[i]) > 7)
            continue;

        for (; j < hand.size-1 && triples < capacity && 
               MJ_SUIT(hand.tiles[i]) == MJ_SUIT(hand.tiles[j]) &&
               MJ_NUMBER(hand.tiles[j]) - MJ_NUMBER(hand.tiles[i]) <= 1; ++j)
        {
            if (MJ_NUMBER(hand.tiles[j]) == MJ_NUMBER(hand.tiles[i]) + 1)
            {
                for (k = j + 1; k < hand.size && triples < capacity && 
                       MJ_SUIT(hand.tiles[i]) == MJ_SUIT(hand.tiles[k]) &&
                       MJ_NUMBER(hand.tiles[k]) - MJ_NUMBER(hand.tiles[j]) <= 1; ++k)
                {
                    if (MJ_NUMBER(hand.tiles[k]) == MJ_NUMBER(hand.tiles[j]) + 1)
                    {
                        result[triples++] = MJ_TRIPLE(hand.tiles[i], hand.tiles[j], hand.tiles[k]);
                    }
                }
            }
        }
    }

    LOG_DEBUG("triples: %d\n", triples);
#if _DEBUG_LEVEL > 1
    if (triples==capacity)
        LOG_WARN("The capacity was reached. Not all triples are included.\n");
#endif


    return triples;
}

static mj_size mj_clean_hand(mj_tile *hand, mj_size size, mj_tile *output)
{
    mj_size i = 0, j = 0;

    if (!output)
    {
        output = hand;
    }

    for (; i < size; ++i)
    {
        if (hand[i] != MJ_INVALID_TILE)
        {
            output[j++] = hand[i];
        }
    }

    return j;
}

static mj_size mj_traverse_tree(mj_tile *hand, mj_size begin, mj_size size, mj_triple branch)
{
    if (size - begin < 3)
        return 0;

    for (; hand[begin] != MJ_FIRST(branch) ; ++begin)
        if (begin == size - 2)
            return 0;

    hand[begin++] = MJ_INVALID_TILE;

    mj_size i = begin;
    for (; hand[i] != MJ_SECOND(branch); ++i)
        if (i == size - 1)
            return 0;
    
    hand[i++] = MJ_INVALID_TILE;
    for (; hand[i] != MJ_THIRD(branch); ++i)
        if (i == size)
            return 0;
    
    hand[i++] = MJ_INVALID_TILE;

    return begin; // next time, only traverse from here (due to the order)
}

typedef struct mj_tree_node
{
    mj_triple meld;
    struct mj_hand_array *child; // NULL if it is a leaf
} mj_tree_node;

typedef struct mj_hand_array
{
    mj_tree_node *array;
    mj_size size;       // number of elements in the array
    mj_size count;      // number of elements that this branch can reach. Count should always be bigger than size.
} mj_hand_array;

static mj_hand_array *mj_hand_array_init()
{
    mj_hand_array *tree = malloc(sizeof(mj_hand_array));
    tree->array = NULL;
    tree->size = 0;
    tree->count = 0;
    return tree;
}

static void mj_hand_array_insert(mj_hand_array **arr, mj_tree_node node)
{
    if ((*arr)->size)
        (*arr)->array = realloc((*arr)->array, sizeof(mj_tree_node) * ((*arr)->size + 1));
    else
        (*arr)->array = malloc(sizeof(mj_tree_node));
    
    (*arr)->array[((*arr)->size)++] = node;
    (*arr)->count += node.child->count;
}

static void mj_hand_array_destroy(mj_hand_array *arr)
{
    if (arr->array) // if this is not NULL, size should never be 0
    {
        for (mj_size i = 0; i < arr->size; ++i)
        {
            if (arr->array[i].child)
                mj_hand_array_destroy(arr->array[i].child);
        }
        free(arr->array);
    }
    free(arr); // always free this
}

/* return how many */
static mj_hand_array *dfs(mj_tile *tiles, mj_size size, mj_triple *triples, mj_size num_triples, mj_size n)
{
    if (n == 0)
    {
        mj_hand_array *res = mj_hand_array_init();
        res->count = 1;
        return res;
    }

    mj_hand_array *children = NULL;

    for (mj_size i = 0; i <= num_triples - n; ++i)
    {
        mj_tile tmp_tiles[MJ_MAX_HAND_SIZE];
        mj_size tmp_size = mj_clean_hand(tiles, size, tmp_tiles);
        mj_size next = mj_traverse_tree(tmp_tiles, 0, tmp_size, triples[i]);
        if (next != 0)
        {
            mj_hand_array *found = dfs(tmp_tiles, tmp_size, triples+i+1, num_triples-i-1, n-1);
            // if my thing returned 1, then there is 1 solution and i store it in result
            if (found)
            {
                if (!children)
                    children = mj_hand_array_init();
                mj_tree_node node = {triples[i], found};
                mj_hand_array_insert(&children, node);
#if _DEBUG_LEVEL > 39
                mj_print_triple(triples[i]);
                LOG_DEBUG(" has %d children with depth %d\n", found->count, n);
#endif
            }
        }
    }

    return children;
}

static void mj_add_perms(mj_triple *perm_triples, mj_size perms, mj_triple *result, mj_hand_array *root, mj_size n)
{
    for (mj_size i = 1; i <= root->count; ++i)
    {
        for (mj_size j = 0; j < perms; ++j)
            result[i*n-j-1] = perm_triples[j];
    }
}

static void mj_add_arrays(mj_hand_array *root, mj_size n, mj_triple *result)
{
    if (!root)
        return;
    mj_size offset = 0;
    for (mj_size i = 0; i < root->size; ++i)
    {
        mj_size child_count = root->array[i].child->count;
        for (mj_size j = 0; j < child_count; ++j)
        {
            result[(offset+j)*n] = root->array[i].meld;
        }
        mj_add_arrays(root->array[i].child, n, result+1+offset*n);
        offset += child_count;
    }
}


mj_size mj_n_triples(mj_hand hand, mj_triple *triples, mj_size num_triples, mj_triple *result, mj_size n)
{
    if (num_triples < n)
    {
        return 0;
    }

    mj_triple perm_triples[MJ_MAX_TRIPLES_IN_HAND];

#if _DEBUG_LEVEL > 0
    if (n > MJ_MAX_TRIPLES_IN_HAND) 
        LOG_CRIT("n=%d is greater than the maximum triples in hand of %d\n", n, MJ_MAX_TRIPLES_IN_HAND);
#endif

    mj_size perms = 0; /* number of permenent triples */

    /* Temporary id to ensure no 4-of-a-kind is counted as 2 triples */
    mj_id _tmp_id = MJ_ID_128(MJ_INVALID_TILE); 

    /* We start from the end of the list because the triples are weakly ordered 
     * detect the permenent triples (those for winds and dragon tiles) 
     *
     * We want to remove them to optimize the performance, as they cannot form runs.
     */
    for (;
        num_triples > 0 && (
        MJ_SUIT(MJ_FIRST(triples[num_triples - 1])) == MJ_WIND ||
        MJ_SUIT(MJ_FIRST(triples[num_triples - 1])) == MJ_DRAGON
        );
        --num_triples)
    {
        mj_tile a_tile = MJ_FIRST(triples[num_triples - 1]);
        if (MJ_ID_128(a_tile) != _tmp_id)
        {
            _tmp_id = MJ_ID_128(a_tile);
            perm_triples[perms++] = triples[num_triples - 1];
        }
    }

    /* Now we remove all winds and dragons from the hand, start at the end of the array */
    while(MJ_SUIT(hand.tiles[hand.size - 1])==MJ_WIND || MJ_SUIT(hand.tiles[hand.size - 1])==MJ_DRAGON)
    {
        --hand.size;
    }

    /* We call the rest of the hand recursively */
    mj_hand_array *children = dfs(hand.tiles, hand.size, triples, num_triples, n-perms);
    
    if (!children)
        return 0;
    
    if (perms > 0)
        mj_add_perms(perm_triples, perms, result, children, n);
    if (perms < n)
        mj_add_arrays(children, n, result);

    mj_size count = n * children->count;

    mj_hand_array_destroy(children);

    return count;
}

mj_size mj_n_agari(mj_hand hand, mj_meld open, mj_meld *result)
{
    mj_id pairs[8];
    mj_size num_pairs = mj_pairs(hand, pairs);

    mj_size const num_closed = MJ_MAX_TRIPLES_IN_HAND - open.size;
    mj_triple triple_buffer[MAX_CAPACITY], combo_buffer[MAX_CAPACITY];
    mj_hand tmp_hand;

    mj_size num_wins = 0;

    for (mj_size i = 0; i < num_pairs; ++i)
    {
        mj_size pair_loc;
        memcpy(tmp_hand.tiles, hand.tiles, sizeof(mj_tile) * hand.size);
        for (pair_loc = 0; MJ_ID_128(tmp_hand.tiles[pair_loc]) != pairs[i]; ++pair_loc)
        {;}
        
        tmp_hand.tiles[pair_loc] = MJ_INVALID_TILE;
        tmp_hand.tiles[pair_loc+1] = MJ_INVALID_TILE;
        
        tmp_hand.size = mj_clean_hand(tmp_hand.tiles, hand.size, NULL);

        mj_size num_triples = mj_triples(tmp_hand, triple_buffer, MAX_CAPACITY);
        mj_size num_combos = mj_n_triples(tmp_hand, triple_buffer, num_triples, combo_buffer, num_closed);

#if _DEBUG_LEVEL > 0
        assert(num_combos <= MAX_CAPACITY);
#endif

#if _DEBUG_LEVEL > 2
        LOG_DEBUG("%d combos for pair %d\n", num_combos, pairs[i]);
        for (mj_size j = 0; j < num_combos; ++j)
        {
            mj_print_triple(combo_buffer[j]);
            LOG_DEBUG("\n");
        }
#endif
        if (num_combos == 0)
            continue;

        for (mj_size j = 0; j < num_combos / num_closed; ++j)
        {
            mj_meld *prev = result + num_wins - 1;
            mj_meld *cur = result + num_wins;
            
            memcpy(cur->melds, combo_buffer + j*num_closed, sizeof(mj_triple) * num_closed);
            
            if (j == 0)
            {
                for (mj_size l = 0; l < open.size; ++l)
                {
                    cur->melds[num_closed+l] = open.melds[l];
                }
                cur->size = num_closed + open.size;
                ++num_wins;
                LOG_DEBUG("size: %d\n", cur->size);
            }
            else
            {
                for (mj_size k = 0; ; ++k)
                {
                    if (k == num_closed)
                    {
                        break;
                    }
                    if (MJ_TRIPLE_WEAK_EQ(cur->melds[k], prev->melds[k]) == MJ_FALSE)
                    {
                        for (mj_size l = 0; l < open.size; ++l)
                        {
                            cur->melds[num_closed+l] = open.melds[l];
                        }
                        cur->size = num_closed + open.size;
                        ++num_wins;
                        LOG_DEBUG("size: %d\n", cur->size);
                        break;
                    }
                }
            }
        }
    }

    return num_wins;
}


void mj_print_tile(mj_tile tile)
{
#if _DEBUG_LEVEL > 0
    char suit[5] = {'m', 'p', 's', 'w', 'd'};
    char delim[4] = {' ', '_', '-', '^'};
    printf("%d%c%c", MJ_NUMBER1(tile), suit[MJ_SUIT(tile)], delim[(tile&0b11)]);
#endif
}
void mj_print_hand(mj_hand hand)
{
#if _DEBUG_LEVEL > 0
    for (mj_size i = 0; i < hand.size; ++i)
    {
        mj_print_tile(hand.tiles[i]);
    }
    printf("\n");
#endif
}
void mj_print_pair(mj_pair pair)
{
#if _DEBUG_LEVEL > 0
    printf("[");
    mj_print_tile(MJ_FIRST(pair));
    mj_print_tile(MJ_SECOND(pair));
    printf("]");
#endif
}

void mj_print_triple(mj_triple triple)
{
#if _DEBUG_LEVEL > 0
    switch(MJ_IS_OPEN(triple))
    {
        case MJ_TRUE:
            printf("{");
            break;
        case MJ_FALSE:
            printf("[");
            break;
        case MJ_MAYBE:
            printf("<");
            break;
    }
    mj_print_tile(MJ_FIRST(triple));
    mj_print_tile(MJ_SECOND(triple));
    mj_print_tile(MJ_THIRD(triple));
    switch(MJ_IS_OPEN(triple))
    {
        case MJ_TRUE:
            printf("}");
            break;
        case MJ_FALSE:
            printf("]");
            break;
        case MJ_MAYBE:
            printf(">");
            break;
    }
#endif
}

void mj_print_meld(mj_meld meld)
{
#if _DEBUG_LEVEL > 0
    printf("("); 
    for (mj_size i = 0; i < meld.size; ++i)
        mj_print_triple(meld.melds[i]);
    printf(")");
#endif
}