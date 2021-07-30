#ifndef __BOARD_H_
#define __BOARD_H_

#include <array>

#include "types.h"
#include "bitboard.h"

namespace BT {



class Position
{
public:
    using key_type = uint64_t;
    using action_type = Move;

    Position();

    key_type key() const;
    std::vector<Move> valid_actions() const;
    bool apply_action(Move);
    bool is_terminal() const;
    double evaluate(Move) const;
    static double evaluate_terminal(const Position&);

    Color side_to_move() const;

private:
    std::array<Piece, Square_nb> m_board;
    Color m_side_to_move;

    void do_move(Square from, Square to);
};

inline Color Position::side_to_move() const {
    return m_side_to_move;
}

inline void Position::do_move(Square from, Square to) {
    Bitboard from_to_bb = SquareBB[from] ^ SquareBB[to];
}

} // namespace BT

#endif // BOARD_H_
