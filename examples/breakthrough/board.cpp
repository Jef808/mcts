#include "board.h"
#include "types.h"
#include "bitboard.h"
#include "utils/zobrist.h"

#include <vector>

namespace BT {

namespace {

    static constexpr size_t N_keys = to_int(Square::Nb) * to_int(Color::Nb) * to_int(Color::Nb);

    struct HashIndex {
        size_t operator()(Pawn p, Square s) {
            return to_int(color_of(p)) + to_int(s);
        }
    };

    zobrist::KeyTable<HashIndex, BT::Position::key_type, N_keys> KTable { };

} // namespace

    constexpr void Position::init(Position& pos)
    {
        for (int i=0; i<16; ++i) {
            pos.m_board[i] = make_pawn(BT::Color::White);
            BT::File _f = BT::File(i & 7);
            BT::Rank _r_w = BT::Rank(i & 8);
            BT::Square _s = BT::make_square(_f, _r_w);
            Pawn& p = pos.m_board[to_int(_s)] = BT::make_pawn(BT::Color::White);
            pos.m_byColorBB[to_int(BT::Color::White)] |= BT::square_bb(_s);
            pos.m_key ^= KTable(p, _s);
            p = pos.m_board[to_int(~_s)] = BT::make_pawn(BT::Color::Black);
            pos.m_byColorBB[to_int(BT::Color::Black)] |= BT::square_bb(~_s);
            pos.m_key ^= KTable(p, _s);
        }
    }



constexpr Position::Position() :
    m_board{Pawn::None},
    m_byColorBB{},
    m_key{},
    m_side_to_move(Color::White)
{
    init(*this);
}

constexpr std::array<Square_d, 3> delta(Color c) {
    return (
        c == Color::White ?
            std::array<Square_d, 3> { Square_d::North, Square_d::North_east, Square_d::North_west } :
            std::array<Square_d, 3> { Square_d::South, Square_d::South_east, Square_d::South_west }
    );
}

std::vector<Move> Position::valid_actions() const
{
    std::vector<Move> ret;
    for (Square s = Square::A1; s != Square::H8; ++s)
    {
        Bitboard valid_bb = valid_moves_bb_(m_side_to_move, s, m_byColorBB);
        for (Square_d d : delta(m_side_to_move))
        {
            if (valid_bb & square_bb(s + d))
                ret.push_back(make_move(s, s + d));
        }
    }

    return ret;
}

bool constexpr Position::apply_action(Move m) {
    bool ret = false;

    Square from = from_sq(m);
    Square to = to_sq(m);

    bool is_capture = m_byColorBB[to_int(~m_side_to_move)] & square_bb(to);

    if (is_capture)
    {
        Pawn opp_pawn = make_pawn(~m_side_to_move);
        remove_piece(opp_pawn, to);
        m_key ^= KTable(opp_pawn, to);
    }

    Pawn my_pawn = make_pawn(m_side_to_move);
    move_pawn(my_pawn, from, to);
    m_key ^= KTable(my_pawn, from);
    m_key ^= KTable(my_pawn, to);

    return ret;
}


} //namespace BT
