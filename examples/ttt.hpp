#ifndef __TTT_H_
#define __TTT_H_

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <ostream>
#include <vector>

namespace ttt {

using line_t = std::array<int, 3>;

/** The possible values for a Cell in a Tic-Tac-Toe grid. */
enum class Token { EMPTY,
    X,
    O };
/** The lines giving a win when filled up. */
static constexpr std::array<line_t, 8> WIN_COMBIN { { { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 }, { 0, 3, 6 }, { 1, 4, 7 }, { 2, 5, 8 }, { 0, 4, 8 }, { 2, 4, 6 } } };
/** An empty 3x3 board. */
constexpr std::array<Token, 9> EMPTY_GRID { { Token::EMPTY, Token::EMPTY, Token::EMPTY, Token::EMPTY, Token::EMPTY, Token::EMPTY, Token::EMPTY, Token::EMPTY, Token::EMPTY } };

/** The actions to be played. */
struct Action {
    int ndx;
    Token token;
    Action(int _ndx = -1, Token _token = Token::EMPTY)
        : ndx(_ndx)
        , token(_token)
    {
    }
    bool operator==(const Action& other) const
    {
        return ndx == other.ndx && token == other.token;
    }
};

/** State of a 3x3 Tic-Tac-Toe game. */
class State {
public:
    using grid_t = std::array<Token, 9>;
    using token_line_t = std::array<Token, 3>;

    /** Initialize a State from a given 3x3 grid. */
    State(grid_t _grid = EMPTY_GRID)
        : grid(_grid)
    {
    }

private:
    /** The grid holding the cells of the game */
    grid_t grid;

public:
    /** Check if the current game is over */
    bool is_terminal() const;
    /** Return indices of empty cells. */
    std::vector<Action> get_valid_actions() const;
    /** Play a move on the board directly. */
    State& apply_action(const Action&);
    /**
     * Return Token::X (resp Token::O) if X wins (resp if O wins)
     * or Token::EMPTY if there is no winner.
     */
    Token get_winner() const;
    /** Check if game is a draw. */
    bool is_draw() const;
    /** Return token of player whose turn it is. */
    Token get_next_player() const;

    Token operator[](int) const;
    bool operator==(const State& other) const { return grid == other.grid; }

private:
    size_t n_empty_cells() const;
    token_line_t get_tokens(const line_t&) const;
};

// inline bool operator==(const Action& a, const Action& b)
// {
//     return a.operator==(b);
// }
// inline bool operator==(const State& a, const State& b)
// {
//     return a.operator==(b);
// }
inline Token State::operator[](int ndx) const { return grid[ndx]; }

/** To print a token. */
inline std::ostream& operator<<(std::ostream& _out, Token t)
{
    switch (t) {
    case Token::X:
        return _out << "X";
    case Token::O:
        return _out << "O";
    default:
        return _out << ' ';
    }
}

inline std::ostream& operator<<(std::ostream& _out, Action action)
{
    return _out << '(' << action.ndx << ", " << action.token << ')';
}

/** To print a state. */
inline std::ostream& operator<<(std::ostream& _out, const State& board)
{
    for (int i = 0; i < 3; ++i) {
        _out << "| ";
        for (int j = 0; j < 3; ++j) {
            _out << board[i * 3 + j] << ' ';
        }
        _out << " |\n";
    }
    return _out;
}
}

#endif // __TTT_H_
