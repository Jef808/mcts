#include "ttt.hpp"
#include <array>
#include <iostream>
#include <string>

namespace ttt {

std::string to_s(ttt::Token t)
{
    switch (t) {
    case Token::X:
        return "X";
    case Token::O:
        return "O";
    default:
        return " ";
    }
}

size_t ttt::State::n_empty_cells() const
{
    return std::count_if(begin(grid), end(grid), [](const Token t) { return t == Token::EMPTY; });
}

Token ttt::State::get_next_player() const
{
    return n_empty_cells() & 1 ? Token::X : Token::O;
}

std::vector<Action> ttt::State::get_valid_actions() const
{
    std::vector<Action> ret{ };
    std::for_each(cbegin(grid), cend(grid), [ndx = 0, nex_player = get_next_player(), &ret](const auto& t) mutable {
        if (t == Token::EMPTY) {
            ret.emplace_back(ndx, nex_player);
        }
        ++ndx;
    });
    return ret;
}

State& ttt::State::apply_action(const Action& action)
{
    grid[action.ndx] = action.token;
    return *this;
}

bool ttt::State::is_terminal() const
{
    return n_empty_cells() == 0 || get_winner() != Token::EMPTY;
}

bool ttt::State::is_draw() const
{
    return n_empty_cells() == 0 && get_winner() == Token::EMPTY;
}

State::token_line_t State::get_tokens(const line_t& line) const
{
    token_line_t ret { Token::EMPTY };
    std::copy_if(cbegin(grid), cend(grid), begin(ret), [it = cbegin(line), grid_ndx = -1](const auto& t) mutable {
        if (*it == ++grid_ndx) {
            ++it;
            return true;
        }
        return false;
    });
    return ret;
}

auto three_in_row = [](const State::token_line_t& line, Token tok) {
    return std::all_of(cbegin(line), cend(line), [&tok](const auto& t) { return t == tok; });
};

Token ttt::State::get_winner() const
{
    for (const auto& win_line : WIN_COMBIN) {
        auto token_line =get_tokens(win_line);
        for (auto& t : { Token::X, Token::O }) {
            if (three_in_row(token_line, t)) {
                return t;
            }
        }
    }
    return Token::EMPTY;
}

double ttt::State::eval_terminal(Token player_token) const
{
    auto winner = get_winner();
    if (winner == player_token) {
        return 1.0;
    }
    return winner == Token::EMPTY ? 0.0 : -1.0;
}



} // namespace ttt
