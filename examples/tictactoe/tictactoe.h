#ifndef __TICTACTOE_H_
#define __TICTACTOE_H_

#include <algorithm>
#include <array>
#include <iosfwd>
#include <vector>

#include "utils/rand.h"
#include "types.h"
#include "bitboard.h"

namespace ttt {

/**
* We represent a state using bitboards of types uint32_t.
*
* There are 9 squares, and each one take 3 bits to
* fully desribe: 001 for empty, 010 for X, 100 for O.
*/
class State {
public:
    using grid_t = std::array<Token, 9>;
    using key_type = Key;
    using Move = Square;
    using action_type = Move;
    using player_type = Player;
    using reward_type = double;
    using actions_list = std::vector<Square>;
    static void init();

    State();
    key_type key() const;
    bool is_terminal() const;
    bool is_trivial(Move) const;
    bool is_valid(Move move) const;
    actions_list const& valid_actions() const;
    bool apply_action(Move);
    action_type apply_random_action();
    Player winner() const;
    bool is_draw() const;
    Player constexpr side_to_move() const;
    Token constexpr token_to_move() const;
    Bitboard constexpr bb() const;
    reward_type evaluate(Move) const;

    bool is_full() const;
    bool has_won(Player) const;
    reward_type static evaluate_terminal(const State&);

    friend std::ostream& operator<<(std::ostream&, const State&);
private:
    Bitboard m_bb;
    Player m_side_to_move;
    mutable actions_list m_actions_list;
};

class State_normal {
public:
    using Move = Square;
    using action_type = Move;
    using actions_list = std::vector<Square>;

    State_normal();

    bool is_terminal() const;
    bool apply_action(Move);
    action_type apply_random_action();
    Token constexpr token_to_move() const;
    Player winner() const;
    bool is_full() const;
    bool is_draw() const;
    bool has_won(Player p) const;
    actions_list const& valid_actions() const;

    friend std::ostream& operator<<(std::ostream&, const State_normal&);
private:
    std::array<Token, 9> m_board;
    Player m_side_to_move;
    mutable actions_list m_actions_list;
};

inline Player operator~(Player p) {
    return p == Player::X ? Player::O : Player::X;
}

inline State::State() :
    m_bb{token_bb(Token::None)},
    m_side_to_move{Player::X}
{
}

inline State_normal::State_normal() :
    m_board{ Token::None },
    m_side_to_move{ Player::X }
{
}

constexpr Bitboard State::bb() const {
    return m_bb;
}

// Only there to comply with the Mcts algorithm.
inline State::reward_type State::evaluate(Move m) const {
    return 0.0;
}

inline State::reward_type State::evaluate_terminal(const State& state) {
    return state.is_draw() ? 0.5 : 1.0;
}

inline bool State::is_terminal() const {
    return has_won(~m_side_to_move) || is_full();
    //return has_won(Player::X) || has_won(Player::O) || is_full();
}

inline bool State_normal::is_terminal() const {
    if (is_full())
        return true;

    const Token tok = token_of(~m_side_to_move);

    for (const auto& line : LinesAsIndex) {
        if (m_board[line[0]] == tok && m_board[line[1]] == tok && m_board[line[2]] == tok)
            return true;
    }
    return false;
}

constexpr Player State::side_to_move() const {
    return m_side_to_move;
}

constexpr Token State::token_to_move() const {
    return token_of(m_side_to_move);
}

constexpr Token State_normal::token_to_move() const {
    return token_of(m_side_to_move);
}

inline State::key_type State::key() const {
    return m_bb;
}

inline Player State::winner() const {
    return ~m_side_to_move;
}

inline Player State_normal::winner() const {
    return ~m_side_to_move;
}

inline bool State::has_won(Player p) const
{
    auto tok_bb = token_bb(token_of(p));
    auto bb = m_bb & tok_bb;
    return std::any_of(LineBB.begin(), LineBB.end(), [&bb, &tok_bb](auto line){
        return (bb & line) == (tok_bb & line);
    });
}

inline bool State_normal::has_won(Player p) const
{
    for (const auto& line : LinesAsIndex) {
        const Token tok = m_board[line[0]];
        if (tok == token_of(p) && m_board[line[1]] == tok && m_board[line[2]] == tok)
            return true;
    }
    return false;
}

inline bool State::is_full() const
{
    return (m_bb & token_bb(Token::None)) == 0;
}

inline bool State_normal::is_full() const
{
    return std::none_of(m_board.begin(), m_board.end(), [](const auto tok){
        return tok == Token::None;
    });
}

inline bool State::is_draw() const
{
    return is_full() && !has_won(~m_side_to_move);// && !has_won(Player::O);
}

inline bool State_normal::is_draw() const
{
    return is_full() && !has_won(~m_side_to_move);
}

inline bool State::is_trivial(Move move) const
{
    return (m_bb & square_bb(move)) != 0;
}

inline bool State::is_valid(Move move) const
{
    return move != Move::Nb;
}

constexpr Bitboard valid_actions_bb(const State& s) {
    return (s.bb() & token_bb(Token::None)) << (s.side_to_move() == Player::X ? 1 : 2);
}

inline const State::actions_list& State::valid_actions() const
{
    m_actions_list.clear();

    if (is_terminal())
    {
        return m_actions_list;
    }

    Player p = m_side_to_move;
    Bitboard va_bb = ((m_bb & token_bb(Token::None)) << (to_int(m_side_to_move) + 1));

    for (Square s = Square::A1; s < Square::Nb; ++s)
    {
        if (square_bb(s) & va_bb)
        {
            m_actions_list.push_back(s);
        }

    }

    return m_actions_list;
}

inline const State_normal::actions_list& State_normal::valid_actions() const
{
    m_actions_list.clear();
    for (size_t i = 0; i<9; ++i) {
        if (m_board[i] == Token::None) {
            m_actions_list.push_back(Square(i));
        }
    }
    return m_actions_list;
}

inline bool State::apply_action(Move m)
{
    Bitboard new_bb = m_bb ^ (move_bb(m_side_to_move, m));

    bool res = new_bb != m_bb;
    if (res)
    {
        m_bb = new_bb;
        m_side_to_move = ~m_side_to_move;
    }

    return res;
}

inline bool State_normal::apply_action(Move m)
{
    bool ret = false;
    if (m_board[to_int(m)] == Token::None) {
        m_board[to_int(m)] = token_to_move();
        ret = true;
        m_side_to_move = ~m_side_to_move;
    }
    return ret;
}

extern Rand::Util<uint8_t> rand_util;

inline State::action_type State::apply_random_action()
{
    //auto actions = valid_actions();
    auto action = rand_util.choose(valid_actions());
    apply_action(action);

    return action;
}

inline State_normal::action_type State_normal::apply_random_action()
{
    auto action = rand_util.choose(valid_actions());
    apply_action(action);
    return action;
}

extern std::ostream& operator<<(std::ostream&, Player);
extern std::ostream& operator<<(std::ostream&, Token);
extern std::ostream& operator<<(std::ostream&, Square);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const State_normal&);

}  // namespace ttt

#endif // __TICTACTOE_H_
