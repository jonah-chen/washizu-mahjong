
#include "mahjong.hpp"

namespace mj {

Hand::Hand(const char *str)
{
    const char *suits = "mpswd";
    Fast8 cur_suit = 0;
    Fast8 cur_sub;
    for (; *str && size_ < k_MaxHandSize; ++str)
    {
        if (*str < '1' || *str > '9')
        {
            while (suits[cur_suit++] != *str)
                if (cur_suit == 5)
                    return;
        }
        else
        {
            if (size_ && tiles_[size_ - 1] == Tile(Suit(cur_suit), *str - '1', cur_sub))
                ++cur_sub;
            else
                cur_sub = 0;
            tiles_[size_++] = Tile(Suit(cur_suit), *str - '1', cur_sub);
        }
    }
}

void Hand::sort() const
{
    if (sorted_)
        return;
    Fast8 i, j;
    for (i = 1; i < size_; ++i)
        for (j = i; j > 0 && tiles_[j - 1] > tiles_[j]; --j)
            std::swap(tiles_[j - 1], tiles_[j]);
    sorted_ = true;
}

std::vector<Meld> Hand::pairs() const
{
    if (!sorted_)
        sort();
    std::vector<Meld> pairs;
    for (Fast8 i = 0; i < size_ - 1; ++i)
    {
        if (tiles_[i] == tiles_[i + 1])
        {
            pairs.emplace_back(tiles_[i], tiles_[i + 1]);
            ++i;
        }
    }
    return pairs;
}

std::vector<Meld> Hand::triples() const
{
    Fast8 j;
    Fast8 k;
    std::vector<Meld> triples;
    for (Fast8 i = 0; i < size_-2; ++i)
    {
        if (tiles_[i].id7() == tiles_[i+1].id7() &&
            tiles_[i].id7() == tiles_[i+2].id7())
        {
            triples.emplace_back(tiles_[i], tiles_[i+1], tiles_[i+2]);
            j = i + 3;
        }
        else
            j = i + 1;
        
        if (tiles_[i].suit()==Suit::Wind || tiles_[i].suit()==Suit::Dragon ||
            tiles_[i].num1() > 7) continue;
        
        for (; j < size_-1 && tiles_[i].suit()==tiles_[j].suit() && tiles_[j].num() - tiles_[i].num() <= 1; ++j)
        {
            if (tiles_[j].num() == 1 + tiles_[i].num())
            {
                for (k = j + 1; k < size_ && tiles_[i].suit()==tiles_[k].suit() && tiles_[k].num() - tiles_[j].num() <= 1; ++k)
                {
                    if (tiles_[k].num() == 1 + tiles_[j].num())
                    {
                        triples.emplace_back(tiles_[i], tiles_[j], tiles_[k]);
                    }
                }
            }
        }
    }
    return triples;
}

}