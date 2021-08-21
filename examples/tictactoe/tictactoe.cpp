#include "types.h"
#include "tictactoe.h"
#include "bitboard.h"

#include <algorithm>
#include <bitset>
#include <iostream>
#include <functional>
#include <limits>
#include <iterator>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>

#include "utils/rand.h"


namespace ttt {

//namespace {
////////////////////////////////////////////////////////////////////////////////
// For Outputting random actions
////////////////////////////////////////////////////////////////////////////////
Rand::Util<uint8_t> rand_util{ };

//} // namespace

////////////////////////////////////////////////////////////////////////////////
// Display methods
////////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& _out, Token tok) {
    return _out << ' '
                << (tok == Token::X ? " X ":
                    tok == Token::O ? " O " : "   ");
}

std::ostream& operator<<(std::ostream& _out, Player p) {
    return _out << token_of(p);
}

std::ostream& operator<<(std::ostream& _out, Square s) {
    int file = to_int(s) % 3;
    int row = to_int(s) / 3;

    switch(file) {
        case 0: { _out << 'a'; break; }
        case 1: { _out << 'b'; break; }
        case 2: { _out << 'c'; break; }
    }
    switch(row) {
        case 0: { _out << '1'; break; }
        case 1: { _out << '2'; break; }
        case 2: { _out << '3'; break; }
    }

    return _out;
}

std::ostream& operator<<(std::ostream& _out, const State& state) {
    const Bitboard bb = state.bb();
    for (int row = 2; row >= 0; --row)
    {
        for (int col = 0; col < 3; ++col)
        {
            auto b = square_bb(make_square(col, row)) & bb;
            for (Token tok : { Token::X, Token::O, Token::None })
            {
                if ((token_bb(tok) & b) > 0)
                {
                    _out << tok;
                }
            }
            _out << (col < 2 ? '|' : '\n');
        }
    }
    return _out;
}

std::ostream& operator<<(std::ostream& _out, const State_normal& state) {
    for (int row = 2; row >= 0; --row)
    {
        for (int col = 0; col < 3; ++col)
        {
            auto n = col + 3 * row;
            _out << state.m_board[n]
                 << (col < 2 ? '|' : '\n');
        }
    }
    return _out;
}

} // namespace ttt
