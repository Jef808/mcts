#ifndef __BITBOARD_H_
#define __BITBOARD_H_

#include "types.h"

namespace BT {

namespace bits {

constexpr void init();

} // namespace BT::bits


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
constexpr Bitboard FileABB = 0x0101010101010101ULL;
/**
 * In binary, FileABB was 0b00000001000000010000000...00000001
 * To get the next file (0b00000001000000010...00000010)
 * we shift the bits left:
 */
constexpr Bitboard FileBBB = FileABB << 1;
/**
 * Etc...
 */
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

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

extern Bitboards<Square> SquareBB;
extern Bitboards<File> FileBB;
extern Bitboards<Rank> RankBB;
extern Bitboards<Color, Rank> ForwardRankBB;
extern Bitboards<Color, Rank> ForwardRanksBB;
extern Bitboards<Color, Square> ForwardFileBB;
extern Bitboards<File> AdjacentFilesBB;
extern Bitboards<Color, Square> PawnAttackSpan;
extern Bitboards<Color, Square> PassedPawnMask;
extern Bitboards<Color, Square> ForwardCapturesBB;
extern Bitboards<Color, Square> ForwardMovesBB;

inline constexpr Bitboard square_bb(Square s) {
    return SquareBB[to_int(s)];
}

inline constexpr Bitboard rank_bb(Rank r) {
    return RankBB[to_int(r)];
}
inline constexpr Bitboard rank_bb(Square s) {
    return RankBB[to_int(rank_of(s))];
}
inline constexpr Bitboard file_bb(File f) {
    return FileBB[to_int(f)];
}
inline constexpr Bitboard file_bb(Square s) {
    return FileBB[to_int(file_of(s))];
}

template<Square_d D>
inline constexpr Bitboard shift(Bitboard b) {
    return D == Square_d::North ? b << 8 : D == Square_d::South ? b >> 8
        : D == Square_d::North_east ? (b & ~FileHBB) << 9 : D == Square_d::South_east ? (b & ~FileHBB) >> 7
        : D == Square_d::North_west ? (b & ~FileABB) << 7 : D == Square_d::South_west ? (b & ~FileABB) >> 9
        : 0;
}

inline constexpr Bitboard forward_rank_bb(Color c, Square s) {
    return ForwardRankBB[to_int(c)][to_int(rank_of(s))];
}

inline constexpr Bitboard valid_noncaptures_bb(Color c, Square s, Bitboard occupied) {
    return ForwardMovesBB[to_int(c)][to_int(s)] & ~occupied;
}

inline constexpr Bitboard captures_bb(Color c, Square s, Bitboard opp_pieces) {
    return ForwardCapturesBB[to_int(c)][to_int(s)] & opp_pieces;
}

inline constexpr Bitboard valid_moves_bb_(Color c, Square s, const Bitboards<Color>& byColorBB) {
    return ForwardMovesBB[to_int(c)][to_int(s)] & (~byColorBB[to_int(c)] | byColorBB[to_int(~c)]);
}

} // namespace BT

#endif // __BITBOARD_H_
