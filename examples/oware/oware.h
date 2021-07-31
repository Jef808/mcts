#ifndef __OWARE_H_
#define __OWARE_H_

#include <array>
#include <cstdint>
#include <iosfwd>
#include <vector>

class Board {
public:
    using reward_type = double;
    using key_type = uint64_t;
    using action_type = int;
    using player_type = bool;

    explicit Board(bool first_player);

    bool is_terminal() const;

    /**
     * Return all the valid actions from the current state.
     *
     * @Note The player who these actions are for is taken into account,
     * i.e. the indices returned are always between 0 and 5 but refers to
     * the player whose turn it is.
    */
    std::vector<int> valid_actions() const;

    /**
     * Return true if `action` is not a meaningful action able to modify the state.
    */
    bool is_trivial(int action) const;

    /**
     * This is only defined to be compatible with the current implementation of MCTS.
    */
    reward_type evaluate(int action) const
    {
        return 0.0;
    }

    /**
     * Return 1.0 for a win, 0.0 for a loss, 0.5 for a draw.
     *
     * @Note The reward is always return from the point of view
     * of the player who played the final move.
    */
    static reward_type evaluate_terminal(const Board&);

    /**
     * Simulate an action played
     *
     * TODO: Find a way to make keeping track of which player it is and
     * whether or not a player gets multiple turns in a row less involved
     * and less prone to bugs.
    */
    bool apply_action(int action);

    /**
     * Tries to play a random action for the player whose turn it is.
     *
     * @Note Return an invalid sentinel (int == -1) action in case the
     * game is already over.
    */
    int apply_random_action();

    /**
     * Computes an int that uniquely identifies the board from its state
    */
    key_type key() const;

    /**
     * Return the final score of both players.
    */
    std::pair<int, int> final_score() const;

    /**
     * Return 1 if it is the first player's turn, otherwise 0.
    */
    bool side_to_move() const;

    /**
     * Return const ref to first player's holes if bool is 1 or to
     * second player's holes otherwise.
    */
    const std::array<int, 6>& holes(bool) const;

    /**
     * Return the number of beads in the first player's mancala
     * if bool is 1, or in the second player's otherwise.
    */
    int mancala(bool) const;

    /**
     * Run some tests
    */
    bool test_init() const;

    bool operator!=(const Board&) const;
    friend std::ostream& operator<<(std::ostream&, const Board&);

private:
    std::array<int, 6> player1;
    std::array<int, 6> player2;

    int man_player2;
    int man_player1;
    bool m_player;
};

extern std::ostream& operator<<(std::ostream&, const Board&);

#endif
