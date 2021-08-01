#ifndef __TICTACTOE_H_
#define __TICTACTOE_H_

#include <array>
#include <cstdint>
#include <list>
#include <vector>

#include "types.h"

namespace ttt {


struct StateData;
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

    static void init();

    State();
    // explicit State(const grid_t&);
    // explicit State(grid_t&&);
    // Game logic
    Token winner() const;
    Token next_player() const;
    bool is_full() const;
    bool is_terminal() const;
    bool is_draw() const;
    bool is_valid(Move move) const;
    std::vector<Move>& valid_actions();
    Move apply_action(Move);
    void apply_move(Move, StateData&);
    void undo_move(Move);


    // Zobrist keys
    //key_type key() const;

    StateData* data;

    Player side_to_play() const;
    Token token_to_play() const;

    const grid_t& grid() const;                 // Only for testing.
    const std::list<Square>& empty_cells() const; // Only for testing

    Bitboard bb() const;

private:
    Bitboard m_data;
    Player m_side_to_play;

    grid_t m_grid;
    std::list<Square> m_empty_cells;
    std::vector<Move> m_valid_actions;
};

inline Bitboard State::bb() const {
    return m_data;
}

inline Player State::side_to_play() const {
    return m_side_to_play;
}

inline Token State::token_to_play() const {
    return token_of(m_side_to_play);
}

inline State::key_type State::key() const {
    return m_data;
}

inline bool key_terminal(State::key_type key) {
    return key & 1;
}

inline Token key_next_player(State::key_type key) {
    return Token(1 + (key >> 1 & 1));
}

inline Token key_winner(State::key_type key) {
    return (key >> 2) & 1 ? Token::None                  // 1*1
                          : (key >> 1) & 1 ? Token::X         // *11
                                           : Token::O;        // *01
}

inline double key_ev_terminal(StateData sd) {
    return (sd.key >> 2) & 1 ? 0.5    // From the pov of the player who just played,
                             : 1;     // either it is a draw or a win (reflect token in backpropagation)
}

}  // namespace ttt

#endif // __TICTACTOE_H_
