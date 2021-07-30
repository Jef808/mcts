#include "bitboard.h"
#include "types.h"

#include <bitset>
#include <iostream>

namespace BT {

Bitboards<Square> SquareBB { };
Bitboards<File> FileBB { };
Bitboards<Rank> RankBB { };
Bitboards<Color, Rank> ForwardRankBB {{}};
Bitboards<Color, Rank> ForwardRanksBB {{}};
Bitboards<Color, Square> ForwardFileBB {{}};
Bitboards<File> AdjacentFilesBB { };
Bitboards<Color, Square> PawnAttackSpan {{}};
Bitboards<Color, Square> PassedPawnMask {{}};
Bitboards<Color, Square> ForwardCapturesBB {{}};
Bitboards<Color, Square> ForwardMovesBB {{}};

void bits::init()
{
    for (Square s = Square::A1; s != Square::Nb; ++s)
    {
        SquareBB[to_int(s)] = 1ULL << to_int(s);
    }

    FileBB[to_int(File::FA)] = FileABB;
    for (File f = File::FB; f != File::Nb; ++f)
    {
        FileBB[to_int(f)] = (FileBB[to_int(f)-1] << 1);
    }

    RankBB[to_int(Rank::R1)] = Rank1BB;
    for (Rank r = Rank::R2; r < Rank::Nb; ++r)
    {
        RankBB[to_int(r)] = (RankBB[to_int(r)-1] << 8);
    }

    for (Rank r = Rank::R1; r < Rank::Nb; ++r)
    {
        ForwardRankBB[to_int(Color::White)][to_int(r)] = (RankBB[to_int(r)] << 8);
        ForwardRankBB[to_int(Color::Black)][to_int(r)] = (RankBB[to_int(r)] >> 8);
    }

    /**
     * All the ranks forward for white at r = (All the ranks BACKWARDS for black at r+1
     *                                         PLUS
     *                                         the rank r)
     */
    for (Rank r = Rank::R1; r < Rank::Nb; ++r)
    {
        ForwardRanksBB[to_int(Color::White)][to_int(r)] = ~(
            ForwardRanksBB[to_int(Color::Black)][to_int(r) + 1] = (
                ForwardRanksBB[to_int(Color::Black)][to_int(r)] | RankBB[to_int(r)]));
    }

    AdjacentFilesBB[to_int(File::FA)] = FileBB[to_int(File::FB)];
    AdjacentFilesBB[to_int(File::FH)] = FileBB[to_int(File::FG)];
    for (int f = to_int(File::FB); f < to_int(File::FH); ++f)
    {
        AdjacentFilesBB[f] = FileBB[f - 1] | FileBB[f + 1];
    }

    for (Color c : { Color::White, Color::Black })
    {
        for (Square s = Square::A1; s != Square::Nb; ++s)
        {
            int _c = to_int(c), _s = to_int(s), _r = to_int(rank_of(s)), _f = to_int(file_of(s));

            ForwardFileBB[_c][_s] = (
                ForwardRanksBB[_c][_r] & FileBB[_f]
            );
            PawnAttackSpan[_c][_s] = (
                ForwardRanksBB[_c][_r] & AdjacentFilesBB[_f]
            );
            PassedPawnMask[_c][_s] = (
                ForwardFileBB[_c][_s] | PawnAttackSpan[_c][_s]
            );
            ForwardMovesBB[_c][_s] = (
                ForwardRankBB[_c][_r] & (ForwardFileBB[_c][_s] | AdjacentFilesBB[_f])
            );
            ForwardCapturesBB[_c][_s] = (
                ForwardMovesBB[_c][_r] ^ ForwardFileBB[_c][_s]
            );
        }
    }
}


} // namespace BT::bits
