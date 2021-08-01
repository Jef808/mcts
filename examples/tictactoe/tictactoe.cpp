#include "types.h"
#include "tictactoe.h"


#include <algorithm>
#include <assert.h>
#include <iostream>
#include <functional>
#include <limits>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <utility>

#include "bitboard.h"
#include "utils/rand.h"


namespace ttt {

namespace {

////////////////////////////////////////////////////////////////////////////////
// Utils
////////////////////////////////////////////////////////////////////////////////
Rand::Util<uint16_t> rand_util{ };

} // namespace


////////////////////////////////////////////////////////////////////////////////
// Tictactoe class
////////////////////////////////////////////////////////////////////////////////
State::State()
    : m_data{token_bb(Token::None)}

{
}

bool constexpr is_draw(const State& s) {
    return (s.bb() & token_bb(Token::None)) == 0;
}

Bitboard constexpr valid_actions_bb(const State& s) {
    return (s.bb() & token_bb(Token::None) & token_bb(s.token_to_play()));
}

bool constexpr has_won(Player p, const State& s) {
    return std::any_of(LineBB.begin(), LineBB.end(), [bb = (s.bb() & token_bb(token_of(p)))](auto line){
        return (bb & line) == line;
    });
}

bool constexpr is_terminal(const State& s) {
    return has_won(Player::X, s) || has_won(Player::O, s) || is_draw(s);
}

bool State::is_valid(Move move) const
{
    return move != Move::Nb;
}

std::vector<State::Move>& State::valid_actions()
{
    Bitboard valid_actions_bb(*this);
}

Token State::winner() const
{
    for (const auto& row : Winning_lineS) {
        auto [a, b, c] = row;

        if (1 == (m_grid[a] * m_grid[b] * m_grid[c])) { return Token::X; }
        if (6 == (m_grid[a] + m_grid[b] + m_grid[c])) { return Token::O; }
    }
    return Token::None;
}

bool State::is_terminal() const
{
    return is_full() || winner() != Token::None;
}

bool State::is_draw() const
{
    return is_full() && winner() == Token::None;
}

State::Move State::apply_action(Move m)
{
    Token t = m_side_to_play;
    add_token(t, m);

    m_key ^= get_key(t, m);
}

void State::apply_move(Move m, StateData& new_sd)
{
    key_type key = data->key;
    // Copy the needed StateData fields to the new one,
    // then replace it with the new one.
    new_sd.gamePly = gamePly + 1;
    new_sd.previous = data;
    data = &(new_sd);

    // Make sure the move corresponds to an empty cell
    auto cell = moveToSquare(m);
    assert(m_empty_cells.remove(cell) == 1);

    // Place the new token in the cell
    m_grid[(int)cell] = moveToToken(m);
    ++gamePly;

    // Update the key for the move
    key ^= Zobrist::moveKey(m);

    // Update the key for the game status
    key ^= Zobrist::sideKey;
    key ^= is_terminal();               // Indicate winner with second bit (next to play).
    key ^= (is_draw() << 2);            // Indicate draw with first and third bit.

    // Place the key in the new StateData.
    data->key = key;
}

// TODO Save the data discarded somehwere!
// TODO Just save the previous move in the stack of StateData,
// then use that move to call undo_move (as it is, there is no
// guarantee that the previous StateData corresponds to the state
// we're reverting back to)
void State::undo_move(Move m)
{
    // Make sure the move corresponds to a non-empty cell.
    auto cell = moveToSquare(m);
    assert(m_grid[(int)cell] != Token::None);

    // Remove the token from the grid.
    m_grid[(int)cell] = Token::None;
    m_empty_cells.push_back(cell);

    // Revert the StateData.
    --gamePly;
    data = data->previous;
}

const State::grid_t& State::grid() const
{
    return m_grid;
}

const std::list<Square>& State::empty_cells() const
{
    return m_empty_cells;
}

} // namespace ttt
