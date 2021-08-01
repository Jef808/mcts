#ifndef __BOARD_H_
#define __BOARD_H_

#include <algorithm>
#include <array>
#include <iosfwd>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <vector>

#include "bitboard.h"
#include "types.h"

namespace BT {

class Position
{
public:
    using key_type = uint64_t;
    using action_type = Move;
    using reward_type = double;
    using player_type = Color;

    using Move_list = std::array<Move, Max_w_pawns>;

    Position();

    key_type constexpr key() const;

    const std::vector<Move>& valid_actions() const;
    bool apply_action(Move);
    Move apply_random_action();
    Move apply_random_action_gen();

    bool constexpr is_terminal() const;
    reward_type constexpr evaluate(Move) const;
    static reward_type constexpr evaluate_terminal(const Position&);
    Color constexpr side_to_move() const;
    static constexpr Color winner(const Position& pos);

private:
    Bitboards<Color> m_byColorBB;
    std::array<std::vector<std::pair<Square, Pawn>>, to_int(Color::Nb)> m_pawns;
    Color m_side_to_move;
    key_type m_key;

    mutable std::vector<Move> m_move_list;

    void put_piece(Pawn, Square);
    void remove_piece(Pawn, Square);
    void move_pawn(Pawn, Square from, Square to);
    key_type compute_key() const;

    std::vector<std::pair<Square, Pawn>>& pawns(Color);
    const std::vector<std::pair<Square, Pawn>>& pawns (Color) const;
    Bitboard color_bb(Color c) const;
    Bitboard& color_bb(Color c);

    friend std::ostream& operator<<(std::ostream&, const Position&);
    Square last_played_w;
    Square last_played_b;
};

inline std::ostream& operator<<(std::ostream& out, const Position& pos) {

    std::array<Pawn, Square_Nb> game_pawns{};
    std::fill(game_pawns.begin(), game_pawns.end(), Pawn::None);

    for (Color c : { Color::White, Color::Black })
    {
        for (const auto& [s, p] : pos.pawns(c))
        {
            game_pawns[to_int(s)] = p;
        }
    }

    for (int i=0; i < Square_Nb; ++i)
    {
        Square s = ~Square(i);

        if (s == pos.last_played_w)
        {
            out << "\e[38;5;72m"
                << game_pawns[to_int(s)]
                << "\e[m";
        }
        else if (s == pos.last_played_b)
        {
            out << "\e[38;5;105m"
                << game_pawns[to_int(s)]
                << "\e[m";
        }
        else
        {
            out << game_pawns[to_int(s)];
        }

        if ((square_bb(s) & FileHBB) > 0)
            out << '\n';
    }

    return out;
}

inline std::vector<std::pair<Square, Pawn>>& Position::pawns(Color c) {
    return m_pawns[to_int(c)];
}

inline const std::vector<std::pair<Square, Pawn>>& Position::pawns(Color c) const {
    return m_pawns[to_int(c)];
}

inline Bitboard& Position::color_bb(Color c) {
    return m_byColorBB[to_int(c)];
}

inline Bitboard Position::color_bb(Color c) const {
    return m_byColorBB[to_int(c)];
}

inline Color constexpr Position::side_to_move() const {
    return m_side_to_move;
}

inline Position::key_type constexpr Position::key() const {
    return m_key;
}

inline void Position::put_piece(Pawn p, Square s) {
    m_byColorBB[to_int(color_of(p))] |= square_bb(s);
    pawns(color_of(p)).push_back(std::make_pair(s, p));
}

inline void Position::remove_piece(Pawn p, Square s) {
    m_byColorBB[to_int(color_of(p))] &= ~square_bb(s);
    auto& my_pawns = pawns(color_of(p));
    my_pawns.erase(std::find(my_pawns.begin(), my_pawns.end(), std::make_pair(s, p)));
}

inline void Position::move_pawn(Pawn p, Square from, Square to) {
    m_byColorBB[to_int(color_of(p))] ^= (square_bb(from) ^ square_bb(to));
    auto& my_pawns = pawns(color_of(p));
    std::find(my_pawns.begin(), my_pawns.end(), std::make_pair(from, p))
        ->first = to;
}

// inline Bitboard constexpr Position::all_pawns_bb() const {
//     return m_byColorBB[to_int(Color::White)] | m_byColorBB[to_int(Color::Black)];
// }

inline constexpr Position::reward_type Position::evaluate(Move m) const {
    return 0.0;
}

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
