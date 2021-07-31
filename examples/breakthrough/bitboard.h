#ifndef __BITBOARD_H_
#define __BITBOARD_H_

#include "types.h"
#include <algorithm>
#include <cstdint>

namespace BT {

/**
 * The 0'th bit is toggled for square A1.
 * Then the 8'th bit for square A2,
 * then the 16'th bit, etc...
 * We can write this as
 *  (1 << 0) + (1 << 8) + (1 << 16) + ...
 * which is the same as
 *  (2 ^ 0) + (2 ^ 8) + (2 ^ 16) + ... + (2 ^ 64)
 * Since 2 ^ 4 = 16, we could rewrite that as
 *  (16 ^ 0) + (16 ^ 2) + (16 ^ 4) + ... + (16 ^ 16)
 * which is easily expressed in hex as
 *   0x10101010101010101.
 */

/**
 * In binary, FileABB was 0b00000001000000010000000...00000001
 * To get the next file (0b00000001000000010...00000010)
 * we shift the bits left:
 */

/**
 * Etc...
 */
constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

constexpr Bitboards<File> _FileBB = []() {
    Bitboards<File> ret { 0x0101010101010101ULL };
    for (File f = File::FB; f < File::Nb; ++f) {
        int _f = to_int(f);
        ret[_f] = ret[_f - 1] << 1;
    }
    return ret;
}();
/**
 * The first rank is 0b00000000...11111111 = 255,
 * and to obtain the next ranks we shift the bits 8 times
 * to the left every time: R2BB = 0b00000000...1111111100000000 etc...
 */
constexpr Bitboard Rank1BB = 0xFF;
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

constexpr Bitboards<Rank> _RankBB = []() {
    Bitboards<Rank> ret {
        0xFF,
    };
    for (Rank r = Rank::R2; r < Rank::Nb; ++r) {
        int _r = to_int(r);
        ret[_r] = ret[_r - 1] << (8 * _r);
    }
    return ret;
}();

constexpr Bitboards<File> FileBB = { FileABB, FileBBB, FileCBB, FileDBB, FileEBB, FileFBB, FileGBB, FileHBB };
constexpr Bitboards<Rank> RankBB = { Rank1BB, Rank2BB, Rank3BB, Rank4BB, Rank5BB, Rank6BB, Rank7BB, Rank8BB };

constexpr Bitboards<Square> SquareBB = []() {
    Bitboards<Square> ret {};
    for (int i = 0; i < Square_Nb; ++i)
        ret[i] = (1ULL << i);
    return ret;
}();
constexpr Bitboards<Color, Rank> ForwardRankBB = []() {
    Bitboards<Color, Rank> ret { {} };
    for (Rank r = Rank::R1; r < Rank::Nb; ++r) {
        ret[to_int(Color::White)][to_int(r)] = (RankBB[to_int(r)] << 8);
        ret[to_int(Color::Black)][to_int(r)] = (RankBB[to_int(r)] >> 8);
    }
    return ret;
}();
/**
 * All the ranks forward for white at r = (
 *    All the ranks BACKWARDS for black at r+1
 *        PLUS the rank r
 * )
 */
constexpr Bitboards<Color, Rank> ForwardRanksBB = []() {
    Bitboards<Color, Rank> ret {};
    for (Rank r = Rank::R1; r < Rank::R8; ++r) {
        ret[to_int(Color::White)][to_int(r)] = ~(
            ret[to_int(Color::Black)][to_int(r) + 1] = (ret[to_int(Color::Black)][to_int(r)] | RankBB[to_int(r)]));
    }
    return ret;
}();
constexpr Bitboards<Color, Square> ForwardFileBB = []() {
    Bitboards<Color, Square> ret {};
    for (Color c : { Color::White, Color::Black }) {
        for (const Square& s : _BOARD) {
            ret[to_int(c)][to_int(s)] = (ForwardRanksBB[to_int(c)][to_int(rank_of(s))] & FileBB[to_int(file_of(s))]);
        }
    }
    return ret;
}();
constexpr Bitboards<File> AdjacentFilesBB = []() {
    Bitboards<File> ret {};
    ret[to_int(File::FA)] = FileBB[to_int(File::FB)];
    ret[to_int(File::FH)] = FileBB[to_int(File::FG)];
    for (int f = to_int(File::FB); f < to_int(File::FH); ++f) {
        ret[f] = FileBB[f - 1] | FileBB[f + 1];
    }
    return ret;
}();
constexpr Bitboards<Color, Square> ForwardMovesBB = []() {
    Bitboards<Color, Square> ret {};
    for (Color c : { Color::White, Color::Black }) {
        for (const Square& s : _BOARD) {
            int _c = to_int(c), _r = to_int(rank_of(s)), _f = to_int(file_of(s)), _s = to_int(s);
            ret[_c][_s] = ForwardRankBB[_c][_r] & (ForwardFileBB[_c][_s] | AdjacentFilesBB[_f]);
        }
    }
    return ret;
}();
constexpr Bitboards<Color, Square> ForwardCapturesBB = []() {
    Bitboards<Color, Square> ret {};
    for (Color c : { Color::White, Color::Black }) {
        for (Square s : _BOARD) {
            int _c = to_int(c), _s = to_int(s), _r = to_int(rank_of(s));
            ret[_c][_s] = ForwardMovesBB[_c][_r] ^ ForwardFileBB[_c][_s];
        }
    }
    return ret;
}();

// extern Bitboards<Color, Square> PawnAttackSpan;
// extern Bitboards<Color, Square> PassedPawnMask;

constexpr Bitboard square_bb(Square s)
{
    return SquareBB[to_int(s)];
}

constexpr Bitboard rank_bb(Rank r)
{
    return RankBB[to_int(r)];
}
constexpr Bitboard rank_bb(Square s)
{
    return RankBB[to_int(rank_of(s))];
}
constexpr Bitboard file_bb(File f)
{
    return FileBB[to_int(f)];
}
constexpr Bitboard file_bb(Square s)
{
    return FileBB[to_int(file_of(s))];
}

template <Square_d D>
constexpr Bitboard shift(Bitboard b)
{
    return D == Square_d::North ? b << 8 : D == Square_d::South ? b >> 8
        : D == Square_d::North_east                             ? (b & ~FileHBB) << 9
        : D == Square_d::South_east                             ? (b & ~FileHBB) >> 7
        : D == Square_d::North_west                             ? (b & ~FileABB) << 7
        : D == Square_d::South_west                             ? (b & ~FileABB) >> 9
                                                                : 0;
}

constexpr Bitboard in_front(Color c, Bitboard b) {
    return c == Color::White ? shift<Square_d::North>(b) : shift<Square_d::South>(b);
}

constexpr Bitboard forward_rank_bb(Color c, Square s)
{
    return ForwardRankBB[to_int(c)][to_int(rank_of(s))];
}

constexpr Bitboard valid_noncaptures_bb(Color c, Square s, Bitboard occupied)
{
    return ForwardMovesBB[to_int(c)][to_int(s)] & ~occupied;
}

constexpr Bitboard captures_bb(Color c, Square s, Bitboard opp_pieces)
{
    return ForwardCapturesBB[to_int(c)][to_int(s)] & opp_pieces;
}

constexpr Bitboard legal_moves_bb(Color c, Square s)
{
    return ForwardMovesBB[to_int(c)][to_int(s)];
}

/**
 * Count the number of non-zero bits
 */
constexpr int popcount(Bitboard b)
{
    return __builtin_popcountll(b);
}

/**
 * Least significant non-zero bit
 *
 * Note: b must be nonzero!
 */
constexpr Square lsb(Bitboard b)
{
    return Square(__builtin_ctzll(b));
}

/**
 * Most significant non-zero bit
 *
 * Note: b must be nonzero!
 */
constexpr Square msb(Bitboard b)
{
    return Square(63 ^ __builtin_clzll(b));
}

} // namespace BT

#endif // __BITBOARD_H_
