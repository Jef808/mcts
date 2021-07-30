#include "bitboard.h"

constexpr void BT::bits::init()
{
    for (int s = to_int(Square::A1); s < to_int(Square::H8); ++s)
    {
        SquareBB[s] = 1ULL << s;
    }

    FileBB[0] = FileABB;
    for (int f = to_int(File::FB); f < to_int(File::Nb); ++f)
    {
        FileBB[f] = FileBB[f-1] << 1;
    }

    RankBB[0] = Rank1BB;
    for (int r = to_int(Rank::R1); r < to_int(Rank::Nb); ++r)
    {
        RankBB[r] = RankBB[r-1] << 8;
    }

    for (Rank r = Rank::R1; r < Rank::R8; ++r)
    {
        ForwardRankBB[to_int(Color::White)][to_int(r)] = RankBB[to_int(r)] << 8;
        ForwardRankBB[to_int(Color::Black)][to_int(r)] = RankBB[to_int(r)] >> 8;
    }

    /**
     * All the ranks forward for white at r = (All the ranks BACKWARDS for black at r+1
     *                                         PLUS
     *                                         the rank r)
     */
    for (Rank r = Rank::R1; r < Rank::R8; ++r)
    {
        ForwardRanksBB[to_int(Color::White)][to_int(r)] = ~(
            ForwardRanksBB[to_int(Color::Black)][to_int(r) + 1] = (
                ForwardRanksBB[to_int(Color::Black)][to_int(r)] | RankBB[to_int(r)]));
    }

    AdjacentFilesBB[to_int(File::FA)] = FileBB[to_int(File::FB)];
    AdjacentFilesBB[to_int(File::FH)] = FileBB[to_int(File::FG)];
    for (int f = to_int(File::FB); f < to_int(File::FG); ++f)
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
            ForwardCapturesBB[_c][_s] = (
                ForwardRankBB[_c][_r] & AdjacentFilesBB[_f]
            );
            ForwardMovesBB[_c][_s] = (
                ForwardCapturesBB[_c][_s] & FileBB[_f]
            );
        }
    }


}
