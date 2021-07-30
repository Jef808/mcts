#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>
#include <type_traits>

namespace BT {

template<typename E>
inline constexpr std::underlying_type_t<E> to_int(E e) {
    return static_cast<std::underlying_type_t<E>>(e);
}

using Bitboard = uint64_t;

template<typename T, std::enable_if_t<!std::is_void_v<T>, std::size_t> N>
using crossBB = typename std::conditional<!std::is_void_v<T>,
                                          std::array<Bitboard, N>,
                                          Bitboard>::type;

template<typename E>
constexpr std::size_t Nb() {
    if constexpr (std::is_void_v<E>) {
        return 0;
    } else {
        return to_int<E>(E::Nb);
    }
}

template<typename E, typename F = void>
using Bitboards = std::array<
    typename std::conditional<std::is_void_v<F>,
                              Bitboard,
                              std::array<Bitboard,
                                         Nb<F>()>
                              >::type, Nb<E>()>;

enum class Color : char {
    White = 0, Black, Nb = 2
};

enum class Pawn {
    White = 0, Black, None
};
constexpr std::size_t Max_pawns = 32;

/**
 * bit 0-2: Square's file
 * bit 3-5: Square's rank
 */
enum class Square : uint8_t {
    A1, H1=7, A3=16, A7 = 48, A8=56, H8,
    Nb = 64,
};

enum class Square_d : int {
    North = 8,
    East = 1,
    South = -North,
    West = -East,

    North_east = North + East,
    South_east = South + East,
    South_west = South + West,
    North_west = North + West
};

inline constexpr Square operator+(Square s, Square_d d) {
    return Square(to_int(s) + to_int(d));
}

/**
 * bit 0- 5: Destination square
 * bit 6-11: Source square
 *
 * The moves None and Null are represented as
 * 0000 000000 000000 and 0000 000001 0000001
 * respectively. Since their source and dest are
 * the same, they don't conflict with valid moves.
 */
enum class Move : uint16_t {
    None = 0,
    Null = 0x3F
};

enum class File : uint8_t {
    FA, FB, FC, FD, FE, FF, FG, FH, Nb = 8
};

enum class Rank : uint8_t {
    R1, R2, R3, R4, R5, R6, R7, R8, Nb = 8
};

inline constexpr Color operator~(Color c) {
    return Color(to_int(c) ^ 1);
}

inline constexpr Color color_of(Pawn p) {
    return p == Pawn::White ? Color::White :
        p == Pawn::Black ? Color::Black : Color::Nb;
}

inline constexpr Pawn make_pawn(Color c) {
    return c == Color::White ? Pawn::White : Pawn::Black;
}

/**
 * Vertical flip: The rank of a square is encoded in bits 3-5,
 * so to flip it vertically we XOR with '0b111000' = '0x38' = 56
 */
inline constexpr Square operator~(Square s) {
    return Square(to_int(s) ^ 0x38);
}

inline constexpr bool is_valid(Square s) {
    return s >= Square::A1 && s <= Square::H8;
}

inline constexpr Square make_square(File f, Rank r) {
    return Square((to_int(r) << 3) + to_int(f));
}

inline constexpr File file_of(Square s) {
    return File(to_int(s) & 7);
}

inline constexpr Rank rank_of(Square s) {
    return Rank(to_int(s) >> 3);
}

/**
 * In the relative methods, return what you'd get if the colors were switched.
 *
 * This way, we can use the same board arithmetics
 * from the point of view of both players.
 */
inline constexpr Square relative_square(Color c, Square s) {
    return Square(to_int(s) ^ (to_int(c) * 0x38));
}

inline constexpr Rank relative_rank(Color c, Square s) {
    return Rank(to_int(s) ^ (to_int(c) * 7));
}

inline constexpr Square from_sq(Move m) {
    return Square(to_int(m) >> 6);
}

/**
 * Destination square is encoded in the first 6 bits,
 * so we can mask with '0b111111' = '0x3F' = 63
 */
inline constexpr Square to_sq(Move m) {
    return Square(to_int(m) & 0x3F);
}

inline constexpr Move make_move(Square from, Square to) {
    return Move((to_int(from) << 6) + to_int(to));
}

inline constexpr bool is_valid(Move m) {
    return from_sq(m) != to_sq(m);
}


// inline constexpr Square& operator++(Square& s) {
//     return s == Square::Nb ? s : s = Square(to_int(s) + 1);
// }


template<typename E>
inline constexpr E& operator++(E& e) {
    return e == E::Nb ? e : e = E(to_int<E>(e) + 1);
}

template<typename E>
inline constexpr E& operator--(E& e) {
    return to_int(e) == 0 ? e : e = E(to_int(e) - 1);
}

template<typename E>
inline constexpr bool operator<(E e1, E e2) {
    return to_int<E>(e1) < to_int<E>(e2);
}

template<typename E>
inline constexpr bool operator>(E e1, E e2) {
    return to_int<E>(e1) > to_int<E>(e2);
}

inline std::ostream& operator<<(std::ostream& out, Pawn p) {
    switch (color_of(p)) {
        case Color::White: { out << "W"; break; }
        case Color::Black: { out << "B"; break; }
        default: { out << "."; break; }
    }
    return out;
}

inline std::ostream& operator<<(std::ostream& out, Square s) {
    File f = file_of(s);
    Rank r = rank_of(s);
    switch(f) {
        case File::FA: { out << "a"; break; }
        case File::FB: { out << "b"; break; }
        case File::FC: { out << "c"; break; }
        case File::FD: { out << "d"; break; }
        case File::FE: { out << "e"; break; }
        case File::FF: { out << "f"; break; }
        case File::FG: { out << "g"; break; }
        case File::FH: { out << "h"; break; }
        default: break;
    }
    switch (r) {
        case Rank::R1: { out << "1"; break; }
        case Rank::R2: { out << "2"; break; }
        case Rank::R3: { out << "3"; break; }
        case Rank::R4: { out << "4"; break; }
        case Rank::R5: { out << "5"; break; }
        case Rank::R6: { out << "6"; break; }
        case Rank::R7: { out << "7"; break; }
        case Rank::R8: { out << "8"; break; }
        default: break;
    }

    return out;
}

} // BT

#endif // __TYPES_H_
