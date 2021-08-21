#ifndef TYPES_H_
#define TYPES_H_

#include <array>
#include <cstdint>

namespace ttt {

template<typename E>
constexpr std::underlying_type_t<E> to_int(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

using Key = uint32_t;
using Bitboard = uint32_t;

enum class Player {
    X, O,
    Nb = 2
};

enum class Token {
    None, X, O,
    Nb = 3
};

constexpr Token token_of(Player p) {
    return p == Player::X ? Token::X : Token::O;
}

constexpr int Square_nb = 9;

enum class Square {
    A1, B1, C1,
    A2, B2, C2,
    A3, B3, C3,
    Nb = Square_nb
};

constexpr Square make_square(int col, int row)
{
    return Square(col + 3 * row);
}

using Move = Square;

constexpr std::array<Token, Square_nb> Empty_Grid
{ Token::None, Token::None, Token::None,
  Token::None, Token::None, Token::None,
  Token::None, Token::None, Token::None
};

enum class Line : uint8_t {
    A1C1, A2C2, A3C3,
    A1A3, B1B2, C1C3,
    A1C3, C1A3,
    Nb = 8
};

constexpr std::array<std::array<Square, 3>, 8>
LinesAsSquares
{ std::array<Square, 3>
    { Square::A1, Square::B1, Square::C1 },
    { Square::A2, Square::B2, Square::C2 },
    { Square::A3, Square::B3, Square::C3 },
    { Square::A1, Square::A2, Square::A3 },
    { Square::B1, Square::B2, Square::B3 },
    { Square::C1, Square::C2, Square::C3 },
    { Square::A1, Square::B2, Square::C3 },
    { Square::A3, Square::B2, Square::C1 }
};

constexpr std::array<std::array<std::size_t, 3>, 8>
LinesAsIndex
{ std::array<std::size_t, 3>
    { 0, 1, 2 },
    { 3, 4, 5 },
    { 6, 7, 8 },
    { 0, 3, 6 },
    { 1, 4, 7 },
    { 2, 5, 8 },
    { 0, 4, 8 },
    { 2, 4, 7 }
};

template<typename E>
inline E& operator++(E& e) {
    return e = (e == E::Nb ? e : E(to_int(e) + 1));
}

template<typename E>
inline bool operator<(E a, E b) {
    return to_int(a) < to_int(b);
}

} // namespace ttt

#endif // TYPES_H_
