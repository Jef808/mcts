#include "board.h"
#include "bitboard.h"
#include "types.h"
#include "utils/rand.h"
#include "utils/zobrist.h"

#include <algorithm>
#include <bitset>
#include <vector>

namespace BT {

namespace {

    struct BT_HashIndex {
        constexpr size_t operator()(Pawn p, Square s)
        {
            return (1 + to_int(color_of(p))) * to_int(s);
        }
    };

    constexpr size_t BT_NKeys = to_int(Square::Nb) * to_int(Color::Nb);

    zobrist::KeyTable<BT_HashIndex, Position::key_type, BT_NKeys> KTable(1); // Reserve one bit For the player's turn.

    Rand::Util<Position::key_type> rand_util {};

    std::array<std::vector<std::pair<Square, Pawn>>, to_int(Color::Nb)> initial_board()
    {
        std::array<std::vector<std::pair<Square, Pawn>>, to_int(Color::Nb)> ret {};

        const Pawn p_w = make_pawn(Color::White);
        const Pawn p_b = make_pawn(Color::Black);

        for (int i = 0; i < 16; ++i) {
            ret[0].push_back(std::make_pair(Square(i), p_w));
            ret[1].push_back(std::make_pair(~Square(i), p_b));
        }

        return ret;
    }

    Bitboards<Color> initial_bb()
    {
        Bitboards<Color> ret;

        ret[0] = 0xFFFF;
        ret[1] = 0xFFFF800000000000;
        return ret;
    }

} // namespace

Position::key_type Position::compute_key() const
{
    key_type key = to_int(m_side_to_move);
    for (const auto& [s, p] : pawns(m_side_to_move)) {
        if (p != Pawn::None)
            key ^= KTable(p, s);
    }

    return key;
}

Position::Position()
    : m_byColorBB { initial_bb() }
    , m_pawns { initial_board() }
    , m_side_to_move { Color::White }
    , m_key { compute_key() }

{
}

const std::vector<Move>& Position::valid_actions() const
{
    m_move_list.clear();

    if (is_terminal())
        return m_move_list;

    for (const auto& [s, p] : pawns(m_side_to_move)) {
        Bitboard legal_bb = legal_moves_bb(m_side_to_move, s);

        Bitboard valid_bb = legal_bb ^ ((in_front(m_side_to_move, square_bb(s)) & color_bb(~m_side_to_move)) | (legal_bb & color_bb(m_side_to_move)));

        while (valid_bb != 0) {
            Square to = lsb(valid_bb);
            m_move_list.push_back(make_move(s, to));
            valid_bb ^= square_bb(to);
        }
    }

    return m_move_list;
}

bool Position::apply_action(Move m)
{
    bool ret = is_valid(m);

    if (!ret)
        return ret;

    Square from = from_sq(m);
    Square to = to_sq(m);
    Bitboard to_bb = square_bb(to);

    auto& opp_color_bb = color_bb(~m_side_to_move);
    // If Capture
    if (opp_color_bb & to_bb) {
        remove_piece(make_pawn(~m_side_to_move), to);
        opp_color_bb &= ~to_bb;
    }

    Pawn my_pawn = make_pawn(m_side_to_move);

    m_key ^= KTable(my_pawn, from);
    m_key ^= KTable(my_pawn, to);

    move_pawn(my_pawn, from, to);

    m_side_to_move = ~m_side_to_move;
    m_key ^= 1;

    if (m_side_to_move == Color::White) {
        last_played_w = to;
    } else {
        last_played_b = to;
    }

    return ret;
}

/**
 * Same as valid_actions() but we iterate over the pawns in a random order
 * and we break the search as soon as a valid move is found (then we apply it).
 */
Move Position::apply_random_action()
{
    m_move_list.clear();
    auto& my_pawns = pawns(m_side_to_move);
    auto _shuffle = rand_util.gen_ordering<Max_w_pawns>(0, my_pawns.size());

    for (auto it = _shuffle.begin(); it != _shuffle.end(); ++it) {
        auto& [s, p] = my_pawns[*it];

        Bitboard legal_bb = legal_moves_bb(m_side_to_move, s);

        Bitboard valid_bb = legal_bb ^ ((in_front(m_side_to_move, square_bb(s)) & color_bb(~m_side_to_move)) | (legal_bb & color_bb(m_side_to_move)));

        if (valid_bb == 0)
            continue;

        while (1) {
            Square to = lsb(valid_bb);
            valid_bb ^= square_bb(to);
            m_move_list.push_back(make_move(s, to));

            if (valid_bb == 0)
                break;
        }

        Move m = rand_util.choose(m_move_list);

        auto& opp_color_bb = color_bb(~m_side_to_move);
        Bitboard from_bb = square_bb(s);
        Square& to = (s = to_sq(m));

        m_byColorBB[to_int(m_side_to_move)] ^= (from_bb ^ square_bb(to));

        // If Capture
        if (opp_color_bb & square_bb(to)) {
            remove_piece(make_pawn(~m_side_to_move), to);
            opp_color_bb &= ~square_bb(to);
        }

        if (m_side_to_move == Color::White) {
            last_played_w = to;
        } else {
            last_played_b = to;
        }

        m_side_to_move = ~m_side_to_move;

        return m;
    }

    return Move::Null;
}

Move Position::apply_random_action_gen()
{
    auto actions = valid_actions();

    if (actions.empty())
        return Move::Null;

    Move m = rand_util.choose(actions);

    apply_action(m);

    return m;
}

} //namespace BT
