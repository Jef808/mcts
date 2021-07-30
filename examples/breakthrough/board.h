#ifndef __BOARD_H_
#define __BOARD_H_

#include <array>

#include "bitboard.h"
#include "types.h"

namespace BT {



class Position
{
public:
    using key_type = uint64_t;
    using action_type = Move;
    using reward_type = double;

    constexpr Position();

    key_type constexpr key() const;
    std::vector<Move> valid_actions() const;
    bool constexpr apply_action(Move);
    bool constexpr is_terminal() const;
    reward_type constexpr evaluate(Move) const;
    static reward_type constexpr evaluate_terminal(const Position&);

    Color constexpr side_to_move() const;

private:
    template<typename T>
    using Board_map = std::array<T, to_int(Square::Nb)>;

    Board_map<Pawn> m_board;
    Bitboards<Color> m_byColorBB;
    key_type m_key;
    // std::array<Pawn, Max_pawns> m_pieces;
    // int m_pawns_count;
    //Board_map<std::size_t> m_index;
    Color m_side_to_move;

    constexpr static void init(Position& pos);
    constexpr void put_piece(Pawn, Square);
    constexpr void remove_piece(Pawn, Square);
    constexpr void move_pawn(Pawn, Square from, Square to);
    static constexpr Color winner(const Position& pos);
    constexpr Bitboard all_pawns_bb() const;
    constexpr key_type key_after(Move m) const;
};

inline Color constexpr Position::side_to_move() const {
    return m_side_to_move;
}

inline Position::key_type constexpr Position::key() const {
    return m_key;
}

inline void constexpr Position::put_piece(Pawn p, Square s) {
    m_board[to_int(s)] = p;
    m_byColorBB[to_int(color_of(p))] |= square_bb(s);
}

inline void constexpr Position::remove_piece(Pawn p, Square s) {
    m_board[to_int(s)] = Pawn::None;
    m_byColorBB[to_int(color_of(p))] ^= square_bb(s);
}

inline void constexpr Position::move_pawn(Pawn p, Square from, Square to) {
    Bitboard from_to = square_bb(from) & square_bb(to);
    m_byColorBB[to_int(color_of(p))] ^= from_to;
    m_board[to_int(from)] = Pawn::None;
    m_board[to_int(to)] = p;
}

inline Bitboard constexpr Position::all_pawns_bb() const {
    return m_byColorBB[to_int(Color::White)] | m_byColorBB[to_int(Color::Black)];
}

inline Color constexpr Position::winner(const Position& pos) {
    Bitboard all_pawns = pos.all_pawns_bb();

    return all_pawns & rank_bb(Rank::R1) ? Color::Black :
        all_pawns & rank_bb(Rank::R8) ? Color::White :
        Color::Nb;
}

inline bool constexpr Position::is_terminal() const {
    return (rank_bb(Rank::R1) | rank_bb(Rank::R8)) & all_pawns_bb();
}

inline Position::reward_type constexpr Position::evaluate_terminal(const Position& pos) {
    return winner(pos) == ~pos.m_side_to_move ? 1.0 : 0.0;
}

} // namespace BT

#endif // BOARD_H_
