#include "ttt.hpp"
#include <array>
#include <iostream>
#include <string>

namespace ttt {


//******************************  Constants  ************************/

const std::array<std::array<int, 3>, 8> WIN_COMBIN ={{{ 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 },
            { 0, 3, 6 }, { 1, 4, 7 }, { 2, 5, 8 }, { 0, 4, 8 }, { 2, 4, 6 } } };
const std::array<Token, 9> EMPTY_GRID = {Token::EMPTY};


//*******************************  Action  ****************************/


const std::string to_s(const Token _token)
 {
    switch (_token) {
    case Token::X:
        return "X";
    case Token::O:
        return "O";
    default:
        return " ";
    }
}

Action::Action(int _ndx, Token _token) : ndx(_ndx) , token(_token) { }
bool Action::operator==(const Action& other) const
{
    return ndx == other.ndx && token == other.token;
}


//*********************************  State  ******************************/

State::State() : grid(EMPTY_GRID) { }

bool State::is_terminal() const
{
    return n_empty_cells() == 0 || get_winner() != Token::EMPTY;
}

Token State::get_next_player() const
{
    return n_empty_cells() & 1 ? Token::X : Token::O;
}

std::vector<Action> State::get_valid_actions() const
{
    std::vector<Action> ret {};
    std::for_each(cbegin(grid), cend(grid), [ndx = 0, nex_player = get_next_player(), &ret](const auto& t) mutable {
        if (t == Token::EMPTY) {
            ret.emplace_back(ndx, nex_player);
        }
        ++ndx;
    });
    return ret;
}

State& State::apply_action(const Action& action)
{
    grid[action.ndx] = action.token;
    return *this;
}

bool State::three_in_row(const std::array<Token, 3>& line, Token tok) const
{
    return std::all_of(cbegin(line), cend(line), [&tok](const auto& t)
    {
        return t == tok;
    });
}

Token State::get_winner() const
{
    auto line_tok = line_token_t();
    for (const auto& line_ : WIN_COMBIN) {
        line_tok = get_tokens(line_);
        if (three_in_row(line_tok, Token::X))
        {
            return Token::X;
        }
        else if (three_in_row(line_tok, Token::O))
        {
            return Token::O;
        }
    }
    return Token::EMPTY;
}

size_t State::n_empty_cells() const
{
    return std::count_if(begin(grid), end(grid), [](Token t) { return t == Token::EMPTY; });
}

State::line_token_t State::get_tokens(const line_t& line) const
{
    return {{grid[line[0]], grid[line[1]], grid[line[2]]}};
}

// double State::eval_terminal(Token player_token) const
// {
//     Token winner = get_winner();
//     if (winner == player_token) {
//         return 1.0;
//     }
//     return winner == Token::EMPTY ? 0.0 : -1.0;
// }



} // namespace ttt

// bool State::is_draw() const
// {
//     return n_empty_cells() == 0 && get_winner() == Token::EMPTY;
// }
