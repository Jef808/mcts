#ifndef __BOARD_H_
#define __BOARD_H_

#include <array>
#include <iosfwd>
#include <unordered_map>

#include "bitboard.h"
#include "types.h"

namespace BT {

class Position
{
public:
    using key_type = uint64_t;
    using action_type = Move;
    using reward_type = double;

    Position();

    key_type constexpr key() const;
    std::vector<Move> valid_actions() const;
    bool apply_action(Move);
    Move apply_random_action();
    bool constexpr is_terminal() const;
    reward_type constexpr evaluate(Move) const;
    static reward_type constexpr evaluate_terminal(const Position&);
    static constexpr Color winner(const Position& pos);

    Color constexpr side_to_move() const;

private:
    template<typename T>
    using Board_map = std::array<T, to_int(Square::Nb)>;

    Board_map<Pawn> m_board;
    Bitboards<Color> m_byColorBB;
    std::array<std::unordered_map<Square, Pawn>, to_int(Color::Nb)> m_pawns;
    key_type m_key;

    // std::array<Pawn, Max_pawns> m_pieces;
    // int m_pawns_count;
    //Board_map<std::size_t> m_index;
    Color m_side_to_move;

    static void init(Position& pos);
    void put_piece(Pawn, Square);
    void remove_piece(Pawn, Square);
    void move_pawn(Pawn, Square from, Square to);
    //constexpr key_type key_after(Move m) const;

    friend std::ostream& operator<<(std::ostream&, const Position&);
};

inline std::ostream& operator<<(std::ostream& out, const Position& pos) {
    for (Rank r = Rank::R8; r > Rank::R1; --r) {
        for (File f = File::FA; f != File::Nb; ++f) {
            Square s = make_square(f, r);
            out << pos.m_board[to_int(s)];
        }
        out << '\n';
    }
    for (File f = File::FA; f != File::Nb; ++f) {
        Square s = make_square(f, Rank::R1);
        out << pos.m_board[to_int(s)];
    }
    return out;
}

inline Color constexpr Position::side_to_move() const {
    return m_side_to_move;
}

inline Position::key_type constexpr Position::key() const {
    return m_key;
}

inline void Position::put_piece(Pawn p, Square s) {
    int _c = to_int(color_of(p));
    m_board[to_int(s)] = p;
    m_byColorBB[_c] |= square_bb(s);
    m_pawns[_c].insert(std::pair{s, p});
}

inline void Position::remove_piece(Pawn p, Square s) {
    int _c = to_int(color_of(p));
    m_board[to_int(s)] = Pawn::None;
    m_byColorBB[_c] ^= square_bb(s);
    m_pawns[_c].erase(s);
}

inline void Position::move_pawn(Pawn p, Square from, Square to) {
    Bitboard from_to = square_bb(from) ^ square_bb(to);
    int _c = to_int(color_of(p));
    m_byColorBB[_c] ^= from_to;
    m_board[to_int(from)] = Pawn::None;
    m_board[to_int(to)] = p;
    auto node_handler = m_pawns[_c].extract(from);
    node_handler.key() = to;
    m_pawns[_c].insert(std::move(node_handler));
}

// inline Bitboard constexpr Position::all_pawns_bb() const {
//     return m_byColorBB[to_int(Color::White)] | m_byColorBB[to_int(Color::Black)];
// }

inline Color constexpr Position::winner(const Position& pos) {
    return rank_bb(Rank::R8) & pos.m_byColorBB[to_int(Color::White)] ? Color::White :
        Color::Black;
}

inline bool constexpr Position::is_terminal() const {
    return (rank_bb(Rank::R1) & m_byColorBB[to_int(Color::Black)])
            | (rank_bb(Rank::R8) & m_byColorBB[to_int(Color::White)]);
}

Position::reward_type constexpr Position::evaluate_terminal(const Position& pos) {
    return winner(pos) == ~pos.m_side_to_move ? 1.0 : 0.0;
}

} // namespace BT

#endif // BOARD_H_
