#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <cstdint>
#include <vector>

namespace BT {

using Bitboard = uint64_t;

template<typename E>
inline std::underlying_type_t<E> to_int(E e) {
    return static_cast<std::underlying_type_t<E>>(e);
}

enum class Color : char {
    White = 0, Black, Nb = 2
};

enum class Piece : uint8_t {
    White = 0, Black, None
};

/**
 * bit 0-2: Square's file
 * bit 3-5: Square's rank
 */
enum class Square : uint8_t {
    A1 = 0, H1 = 7, A8 = 56, H8 = 63,
    Nb = 64
};

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

enum class File : char {
    FA, FB, FC, FD, FE, FF, FG, FH, Nb = 8
};

enum class Rank : char {
    R1, R2, R3, R4, R5, R6, R7, R8, Nb = 8
};

inline Color operator~(Color c) {
    return Color(to_int(c) ^ 1);
}

/**
 * Vertical flip: The rank of a square is encoded in bits 3-5,
 * so to flip it vertically we XOR with '0b111000' = '0x38' = 56
 */
inline Square operator~(Square s) {
    return Square(to_int(s) ^ 0x38);
}

inline bool is_valid(Square s) {
    return s >= Square::A1 && s <= Square::H8;
}

inline Color color_of(Piece pc) {
    return Color(to_int(pc));
}

inline File file_of(Square s) {
    return File(to_int(s) & 7);
}

inline Rank rank_of(Square s) {
    return Rank(to_int(s) >> 3);
}

/**
 * In the relative methods, return what you'd get if the colors were switched.
 *
 * This way, we can use the same board arithmetics
 * from the point of view of both players.
 */
inline Square relative_square(Color c, Square s) {
    return Square(to_int(s) ^ (to_int(c) * 0x38));
}

inline Rank relative_rank(Color c, Square s) {
    return Rank(to_int(s) ^ (to_int(c) * 7));
}

inline Square from_sq(Move m) {
    return Square(to_int(m) >> 6);
}

/**
 * Destination square is encoded in the first 6 bits,
 * so we can mask with '0b111111' = '0x3F' = 63
 */
inline Square to_sq(Move m) {
    return Square(to_int(m) & 0x3F);
}

inline Move make_move(Square from, Square to) {
    return Move((to_int(from) >> 6) + to_int(to));
}

inline bool is_valid(Move m) {
    return from_sq(m) != to_sq(m);
}



} // BT

#endif // __TYPES_H_
