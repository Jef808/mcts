#include "board.h"
#include "types.h"
#include "bitboard.h"
#include "utils/rand.h"
#include "utils/zobrist.h"

#include <algorithm>
#include <bitset>

#include <vector>

namespace BT {

namespace {

    struct BT_HashIndex {
        constexpr size_t operator()(Pawn p, Square s) {
            return to_int(color_of(p)) + to_int(s);
        }
    };

    constexpr size_t BT_NKeys = to_int(Square::Nb) * to_int(Color::Nb) * to_int(Color::Nb);

    zobrist::KeyTable<BT_HashIndex, Position::key_type, BT_NKeys> KTable(1);  // Reserve one bit For the player's turn.

    Rand::Util<Position::key_type> rand_util{};

    std::array<Pawn, to_int(Square::Nb)> empty_board() {
        std::array<Pawn, to_int(Square::Nb)> ret{};
        std::generate(ret.begin(), ret.end(), [](){ return Pawn::None; });

        return ret;
    }

} // namespace

    void Position::init(Position& pos)
    {
        bits::init();

        for (Square s = Square::A1; s != Square::A3; ++s)
        {
            std::cout << "Putting a White pawn at "
                      << s << '\n';

            Pawn p = make_pawn(Color::White);
            pos.m_key ^= KTable(p, s);
            pos.put_piece(p, s);

            std::cout << "Putting a Black pawn at "
                      << ~s << '\n';

            Square opp_sq = ~s;
            p = make_pawn(Color::Black);
            pos.m_key ^= KTable(p, opp_sq);
            pos.put_piece(p, opp_sq);

            std::cout << "State is " << pos << std::endl;
        }
}


Position::Position() :
    m_board{empty_board()},
    m_byColorBB{{}},
    m_key{},
    m_side_to_move{Color::White}
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

    for (const auto& [s, p] : m_pawns[to_int(m_side_to_move)])
    {
        Bitboard valid_bb = legal_moves_bb(m_side_to_move, s);
        int n_candidates = popcount(valid_bb);

        if (n_candidates == 0)
            continue;

        for (int i=0; i<n_candidates; ++i)
        {
            Square candidate = lsb(valid_bb);
            Bitboard candidate_bb = square_bb(candidate);
            valid_bb ^= candidate_bb;

            // If candidate is occupied by one of our own pieces
            if (candidate_bb & m_byColorBB[to_int(m_side_to_move)])
            {
                continue;
            }

            // If candidate is straight forward but is occupied by enemy piece
            if (candidate_bb
                & ForwardFileBB[to_int(m_side_to_move)][to_int(s)]
                & m_byColorBB[to_int(~m_side_to_move)])
            {
                continue;
            }

            // Otherwise candidate is valid.
            ret.push_back(make_move(s, candidate));
        }
    }

    return ret;
}

bool Position::apply_action(Move m) {
    bool ret = false;

    Square from = from_sq(m);
    Square to = to_sq(m);

    bool is_capture = m_byColorBB[to_int(~m_side_to_move)] & square_bb(to);

    if (is_capture)
    {
        Pawn opp_pawn = m_pawns[to_int(~m_side_to_move)][to];
        remove_piece(opp_pawn, to);
        m_key ^= KTable(opp_pawn, to);
    }

    Pawn my_pawn = m_pawns[to_int(m_side_to_move)][from];

    move_pawn(my_pawn, from, to);
    m_key ^= KTable(my_pawn, from);
    m_key ^= KTable(my_pawn, to);

    m_side_to_move = ~m_side_to_move;
    m_key ^= 1;

    return ret;
}

Move Position::apply_random_action()
{
    std::vector<Move> moves = valid_actions();
    Move move = rand_util.choose(moves);
    bool success = apply_action(move);

    return (success ? move : Move::Null);
}


} //namespace BT
