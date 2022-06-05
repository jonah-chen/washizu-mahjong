
#pragma once
#include <array>
#include <vector>
#include <cstdint>

namespace mj {
using Fast8 = uint_fast8_t;
using Fast16 = uint_fast16_t;
using Fast32 = uint_fast32_t;
using Fast64 = uint_fast64_t;
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;
using S8 = int8_t;
using S16 = int16_t;
using S32 = int32_t;
using S64 = int64_t;
using F32 = float;
using F64 = double;
using F64 = double;
constexpr Fast8 k_East = 0;
constexpr Fast8 k_South = 1;
constexpr Fast8 k_West = 2;
constexpr Fast8 k_North = 3;

constexpr Fast8 k_Green = 0;
constexpr Fast8 k_Red = 1;
constexpr Fast8 k_White = 2;

constexpr Fast16 k_UniqueTiles = 34;
constexpr Fast16 k_DeckSize = 136;
constexpr Fast16 k_DeadWallSize = 14;
constexpr Fast16 k_MaxHandSize = 14;

namespace tilelayout
{
    constexpr Fast8 k_PlayerPos = 0;
    constexpr Fast8 k_IdxPos = 2;
    constexpr Fast8 k_NumPos = 4;
    constexpr Fast8 k_SuitPos = 8;
    constexpr U16 k_OpenFlag = 0x8000;
}

enum class Suit : U16
{
    Man = 0 << tilelayout::k_SuitPos,
    Pin = 1 << tilelayout::k_SuitPos,
    Sou = 2 << tilelayout::k_SuitPos,
    Wind = 3 << tilelayout::k_SuitPos,
    Dragon = 4 << tilelayout::k_SuitPos
};

enum class Dir : Fast8
{
    East = k_East,
    South = k_South,
    West = k_West,
    North = k_North
};

class Tile
{
public:
    constexpr Tile() noexcept : id_(-1) {}
    constexpr Tile(U16 id) noexcept : id_(id) {}
    constexpr Tile(Suit suit, Fast8 num, Fast8 idx, Fast8 player=k_East) 
    noexcept : id_((U16)suit | 
                num << tilelayout::k_NumPos | 
                idx << tilelayout::k_IdxPos | 
                player << tilelayout::k_PlayerPos) {}

private:
    U16 id_;

public:
    constexpr Suit suit() const noexcept
    { return static_cast<Suit>(id_ & (7 << tilelayout::k_SuitPos)); }

    constexpr Fast8 num() const noexcept
    { return (id_ >> tilelayout::k_NumPos) & 15; }
    constexpr Fast8 num1() const noexcept
    { return num() + 1; }

    constexpr Fast8 idx() const noexcept
    { return (id_ >> tilelayout::k_IdxPos) & 3; }
    constexpr operator bool() const noexcept
    { return id_ != (U16)-1; }
    constexpr U16 id() const noexcept
    { return id_; }
    constexpr Fast8 id7() const noexcept
    { return (id_ >> tilelayout::k_NumPos) & 127; }
    constexpr Fast16 id9() const noexcept
    { return (id_ >> tilelayout::k_IdxPos) & 511; }
    inline bool operator==(const Tile &rhs) const
    { return id_ == rhs.id_; }
    inline bool operator!=(const Tile &rhs) const
    { return id_ != rhs.id_; }
    inline bool operator<(const Tile &rhs) const
    { return id_ < rhs.id_; }
    inline bool operator>(const Tile &rhs) const
    { return id_ > rhs.id_; }

    inline bool operator<=(const Tile &rhs) const
    { return id_ <= rhs.id_; }
    inline bool operator>=(const Tile &rhs) const
    { return id_ >= rhs.id_; }
};
class Meld
{
public:
    constexpr Meld() noexcept : id_(-1) {}
    constexpr Meld(Tile called, Tile t1, Tile t2 = {}, Tile t3 = {})
        : id_(((U64)called << 48) | ((U64)t1 << 32) | ((U64)t2 << 16) | t3) {}

    constexpr Tile first() const noexcept
    { return Tile(id_ >> 48); }
    constexpr Tile second() const noexcept
    { return Tile((id_ >> 32) & 0xffff); }
    constexpr Tile third() const noexcept
    { return Tile((id_ >> 16) & 0xffff); }
    constexpr Tile fourth() const noexcept
    { return Tile(id_ & 0xffff); }
    constexpr operator bool() const noexcept
    { return id_ != -1; }
private:
    U64 id_;
};

class Hand
{
public:
    Hand() = default;
    Hand(const char *);
    void sort() const;

    constexpr bool check(U64 mask) const noexcept
    { return flags_ & mask; }
    constexpr void set(U64 mask) noexcept
    { flags_ |= mask; }
    constexpr Fast8 size() const noexcept
    { return size_; }

    std::vector<Meld> pairs() const;
    std::vector<Meld> triples() const;

    Tile &operator[](Fast8 idx) noexcept
    { return tiles_[idx]; }
    const Tile &operator[](Fast8 idx) const noexcept
    { return tiles_[idx]; }

private:
    mutable std::array<Tile, 14> tiles_{};
    U64 flags_{};
    Fast8 size_{};
    mutable bool sorted_{};
};

namespace score
{
    constexpr Fast16 k_BonusScore = 100;
    constexpr Fast16 k_RiichiDeposit = 1000;
    constexpr Fast16 k_BaseFu = 20;
    constexpr Fast16 k_Yakuman = 13;
    constexpr Fast16 k_Mangan = 2000;
    constexpr Fast16 k_Haneman = 3000;
    constexpr Fast16 k_Baiman = 4000;
    constexpr Fast16 k_Sanbaiman = 6000;
}

}