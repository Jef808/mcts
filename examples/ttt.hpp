// -*- c++ -*-

#ifndef __TTT_H_
#define __TTT_H_

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

namespace ttt {

/** The possible values for a Cell in a Tic-Tac-Toe grid. */
enum class Token { EMPTY,
    X,
    O };
/** Token to string. */
const extern std::string to_s(const Token _token);
/** The lines giving a win when filled up. */
const extern std::array<std::array<int, 3>, 8> WIN_COMBIN;
/** An empty 3x3 board. */
const extern std::array<Token, 9> EMPTY_GRID;

// inline bool operator==(const Token& token, const Token& other_token)
// {
//     return to_s(token) == to_s(other_token);
// }

/** The actions to be played. */
struct Action {
    int ndx;
    Token token;
    Action(int _ndx = -1, Token _token = Token::EMPTY);
    bool operator==(const Action& other) const;
    operator std::string() const;
};

/** State of a 3x3 Tic-Tac-Toe game. */
class State {
    // const std::array<line_t, 8>& WIN_COMBIN = ttt::WIN_COMBIN;
    // const std::array<Token, 9>& EMPTY_GRID = ttt::EMPTY_GRID;
    using grid_t = std::array<Token, 9>;
    using line_t = std::array<int, 3>;
    using line_token_t = std::array<Token, 3>;

public:
    /** Initialize a State from a given 3x3 grid. */
    State();
    /** Check if the current game is over */
    bool is_terminal() const;
    /** Return indices of empty cells. */
    std::vector<Action> get_valid_actions() const;
    /** Play a move on the board directly. */
    State& apply_action(const Action&);
    /** Return the token of the winner if any or the empty token */
    Token get_winner() const;
    /** Check if game is a draw. */
    bool is_draw() const;
    /** The player who's turn it is to play.*/
    Token get_next_player() const;
    /** Output a short string describing the state. */
    operator std::string() const;
    /** Pretty display the state. */
    std::string to_s() const;
    Token operator[](int ndx) const { return grid[ndx]; }
    bool operator==(const State& other) const { return grid == other.grid; }

    //double eval_terminal(Token) const;

private:
    /** The grid holding the cells of the game. */
    grid_t grid;
    /** Takes a triple of grid indices and return the corresponding tokens. */
    line_token_t get_tokens(const line_t&) const;
    /** Number of empty cells. */
    size_t n_empty_cells() const;
    /** Checks if the tokens at the given indices are all the same. */
    bool three_in_row(const line_token_t&, Token) const;
};



/** Action to string */
inline Action::operator std::string() const {
    return "(" + std::to_string(ndx) + "," + to_s(token) + ")";
}
/** State to string */
inline State::operator std::string() const {
    auto res = std::string();
    for (int i = 0; i < 3; ++i) {
        res += "| ";
        for (int j = 0; j < 3; ++j) {
            res += ttt::to_s(grid[i * 3 + j]) + " ";
        }
        res += " |\n";
    }
    return res;
}
/** To print a token. */
inline std::ostream& operator<<(std::ostream& _out, ttt::Token _token) {
    return _out << to_s(_token);
}
/** To print a state. */
inline std::ostream& operator<<(std::ostream& _out, const ttt::State& _state) {
    return _out << std::string(_state);
}
/** To print an action. */
inline std::ostream& operator<<(std::ostream& _out, ttt::Action _action) {
    return _out << '(' << _action.ndx << ", " << _action.token << ')';
}

}

#endif // __TTT_H_
