#include "mahjong.h"
#include <stdlib.h>
#include <string.h>

#define MJ_MAX_HAND_SIZE 14
#define MJ_MAX_TRIPLES_IN_HAND 4

static char const mj_suit_strings[5] = {'m','p','s','w','d'}; 

mj_size mj_parse(char const *str, mj_tile *tiles)
{
    mj_size size = 0;
    int cur_suit = 0, cur_sub;
    for (; *str && size < MJ_MAX_HAND_SIZE; str++)
    {
        if (*str < '1' || *str > '9')
        {
            if (mj_suit_strings[cur_suit++] != *str)
                return 0;
            else if (cur_suit == 5)
                return size;
        }
        else 
        {
            if (size && tiles[size-1] == MJ_TILE(cur_suit, *str - '1', cur_sub))
                cur_sub++;
            else
                cur_sub = 0;

            tiles[size++] = MJ_TILE(cur_suit, *str - '1', cur_sub);
        }
    }

    return size;
}

void mj_sort_hand(mj_tile *hand, mj_size size)
{
    mj_size i, j;
    mj_tile tmp;
    for (i = 1; i < size; ++i)
    {
        for (j = i; j > 0 && hand[j-1] > hand[j]; --j)
        {
            tmp = hand[j];
            hand[j] = hand[j-1];
            hand[j-1] = tmp;
        }
    }
}

mj_size mj_pairs(mj_tile *hand, mj_size size, mj_id *result)
{
    mj_size i, pairs = 0;
    
    for (i = 0; i < size-1; ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]))
        {
            result[pairs++] = MJ_ID_128(hand[i++]);
        }
    }
    LOG_DEBUG("pairs: %d\n", pairs);
    return pairs;
}

mj_bool mj_pong_available(mj_tile *hand, mj_size size, mj_tile const tile)
{
    for (mj_size i = 0; i < size-1 && MJ_ID_128(hand[i]) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_bool mj_kong_available(mj_tile *hand, mj_size size, mj_tile const tile)
{
    for (mj_size i = 0; i < size-2 && MJ_ID_128(hand[i]) <= MJ_ID_128(tile); ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+2]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(tile))
        {
            return MJ_TRUE;
        }
    }
    return MJ_FALSE;
}

mj_size mj_chow_available(mj_tile *hand, mj_size size, mj_tile const tile, mj_pair *chow_tiles)
{
    if (MJ_SUIT(tile) == MJ_WIND || MJ_SUIT(tile) == MJ_DRAGON)
    {
        return 0;
    }

    mj_size i, j, chows = 0;

    for (i = 0; i < size-1; ++i)
    {
        int difference; 
        if (MJ_SUIT(hand[i]) == MJ_SUIT(tile))
        {
            switch (MJ_NUMBER(hand[i]) - MJ_NUMBER(tile))
            {
                case -2: difference = -1; break;
                case -1: difference = 1; break;
                case 1: difference = 2; break;
                default: continue;
            }

            for (j = i+1; 
                 j < size && 
                 MJ_SUIT(hand[j]) == MJ_SUIT(tile) && 
                 MJ_NUMBER(hand[j])>MJ_NUMBER(tile)+difference;
                 ++j)
            {
                if (MJ_NUMBER(hand[j]) == MJ_NUMBER(tile) + difference)
                {
                    chow_tiles[chows++] = MJ_PAIR(hand[i], hand[j]);
                }
            }
        }
    }

    return chows;
}

mj_size mj_triples(mj_tile *hand, mj_size size, mj_triple *result, mj_size capacity)
{
    mj_size i, j, k, triples = 0;
    
    for (i = 0; i < size-2 && triples < capacity; ++i)
    {
        if (MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+1]) &&
            MJ_ID_128(hand[i]) == MJ_ID_128(hand[i+2]))
        {
            result[triples++] = MJ_TRIPLE(hand[i], hand[i+1], hand[i+2]);
            j = i + 3;
        }
        else 
        {
            j = i + 1;
        }

        if (MJ_SUIT(hand[i])==MJ_WIND || MJ_SUIT(hand[i])==MJ_DRAGON ||
            MJ_NUMBER1(hand[i]) > 7)
            continue;

        for (; j < size-1 && triples < capacity && 
               MJ_SUIT(hand[i]) == MJ_SUIT(hand[j]) &&
               MJ_NUMBER(hand[j]) - MJ_NUMBER(hand[i]) <= 1; ++j)
        {
            if (MJ_NUMBER(hand[j]) == MJ_NUMBER(hand[i]) + 1)
            {
                for (k = j + 1; k < size && triples < capacity && 
                       MJ_SUIT(hand[i]) == MJ_SUIT(hand[k]) &&
                       MJ_NUMBER(hand[k]) - MJ_NUMBER(hand[j]) <= 1; ++k)
                {
                    if (MJ_NUMBER(hand[k]) == MJ_NUMBER(hand[j]) + 1)
                    {
                        result[triples++] = MJ_TRIPLE(hand[i], hand[j], hand[k]);
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
static mj_hand_array *dfs(mj_tile *hand, mj_size size, mj_triple *triples, mj_size num_triples, mj_size n)
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
        mj_tile tmp_hand[MJ_MAX_HAND_SIZE];
        mj_size tmp_size = mj_clean_hand(hand, size, tmp_hand);
        mj_size next = mj_traverse_tree(tmp_hand, 0, tmp_size, triples[i]);
        if (next != 0)
        {
            mj_hand_array *found = dfs(tmp_hand, tmp_size, triples+i+1, num_triples-i-1, n-1);
            // if my thing returned 1, then there is 1 solution and i store it in result
            if (found)
            {
                if (!children)
                    children = mj_hand_array_init();
                mj_tree_node node = {triples[i], found};
                mj_hand_array_insert(&children, node);
#if _DEBUG_LEVEL > 3
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


mj_size mj_n_triples(mj_tile *hand, mj_size size, mj_triple *triples, mj_size num_triples, mj_triple *result, mj_size n)
{
    if (num_triples < n)
    {
        return 0;
    }

    mj_triple perm_triples[MJ_MAX_TRIPLES_IN_HAND];

    mj_tile clean_hand[MJ_MAX_HAND_SIZE]; // maybe not needed

    mj_size clean_hand_size = mj_clean_hand(hand, size, clean_hand);

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
    while(MJ_SUIT(hand[size - 1])==MJ_WIND || MJ_SUIT(hand[size - 1])==MJ_DRAGON)
    {
        --size;
    }

    /* We call the rest of the hand recursively */
    mj_hand_array *children = dfs(hand, size, triples, num_triples, n-perms);
    mj_add_perms(perm_triples, perms, result, children, n);
    mj_add_arrays(children, n, result);

    mj_size count = n * children->count;

    mj_hand_array_destroy(children);

    return count;
}


void mj_print_tile(mj_tile tile)
{
#if _DEBUG_LEVEL > 0
    char suit[5] = {'m', 'p', 's', 'w', 'd'};
    char delim[4] = {' ', '_', '-', '^'};
    printf("%d%c%c", MJ_NUMBER1(tile), suit[MJ_SUIT(tile)], delim[(tile&0b11)]);
#endif
}
void mj_print_hand(mj_tile *hand, mj_size size)
{
#if _DEBUG_LEVEL > 0
    for (mj_size i = 0; i < size; ++i)
    {
        mj_print_tile(hand[i]);
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
    printf("[");
    mj_print_tile(MJ_FIRST(triple));
    mj_print_tile(MJ_SECOND(triple));
    mj_print_tile(MJ_THIRD(triple));
    printf("]");
#endif
}
