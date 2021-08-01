#ifndef BITBOARD_H_
#define BITBOARD_H_

#include <cstddef>
#include <utility>
#include <type_traits>
#include "types.h"
#include <vector>

namespace ttt {


template<typename E>
constexpr std::underlying_type_t<E> to_int(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

template<typename E, typename T = Bitboard>
using Bitboards = std::array<T, to_int(E::Nb)>;

// 77 << (3 * (square_nb))
constexpr Bitboards<Square> SquareBB
{ 0x7,      0x38,     0x1C0,
  0xE00,    0x7000,   0x38000,
  0x1C0000, 0xE00000, 0x7000000
};

constexpr Bitboards<Line> LineBB
{
  0x1FF,     0x3FE00,  0x7FC0000,
  0x1C0E07,  0xE07038, 0x70381C0,
  0x7007007, 0x1C71C0
};

constexpr Bitboards<Token> TokenBB
{
  0x1249249, 0x2492492, 0x4924924
};


constexpr Bitboard square_bb(Square s) {
    return SquareBB[to_int(s)];
}

constexpr Bitboard line_bb(Line l) {
    return LineBB[to_int(l)];
}

constexpr Bitboard token_bb(Token t) {
    return TokenBB[to_int(t)];
}



} // namespace ttt

#endif // BITBOARD_H_
