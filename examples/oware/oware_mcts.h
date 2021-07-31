#ifndef __OWARE_MCTS_H_
#define __OWARE_MCTS_H_

#include "oware.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>


namespace oware {

/**
 * Remove the contribution from the log term after a given number of visits.
 * NOTE This really improves the run-time and average score!
 */
template<size_t N>
struct TimeCutoff_UCB_Func
{
    auto operator()(double expl_cst, unsigned int n_parent_visits)
    {
        return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge)
            {
            double ret = (n_parent_visits < N
                          ? expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0))
                          : 0.00001);
            return ret + edge.total_val / (edge.n_visits + 1.0);
        };
    }
};


class Oware_Weighted_Playout_Func
{
public:
    using reward_type = Board::reward_type;
    using action_type = Board::action_type;
    using weight_type = int;

    Oware_Weighted_Playout_Func(Board& _state) :
        state(_state) { }

    action_type operator()() const
    {
        std::vector<action_type> actions = state.valid_actions();

        if (actions.empty())
            return -1;

        std::vector<weight_type> weights(actions.size(), 1);
        set_weights(state, actions, weights);

        action_type action = choose_action(actions, weights);
        bool success = state.apply_action(action);

        return success ? action : -1;
    }

    /**
     * Assigns weights to moves to create a bias based on a heuristic
     * evaluation.
     *
     * 1) If a move leads to the player playing another turn, give
     *    it a positive bias proportional on how close it is to
     *    the mancala (the closer it is, the more likely it won't
     *    affect the other moves and lead to suboptimal play).
     * 2) If a move leads to a capture, give it a positive bias
     *    proportional to the size of the capture.
     * 3) TODO: If a move leads to the opponent not being able to capture
     *    it otherwise could have done, give it a positive bias
     *    proportional to the amount of beads saved.
     */
    void set_weights(
        const Board& board,
        const std::vector<action_type>& actions,
        std::vector<weight_type>& weights) const
    {
        for (int i=0; i<actions.size(); ++i)
        {
            action_type a = actions[i];
            weight_type& w = weights[i];

            if (is_doubleplay(board, a))
            {
                w += board.side_to_move() ? a : 5 - a;
            }
            else
            {
                w = is_capture(board, a);
            }
        }
    }

private:
    Board& state;
    /** The random engine to pick the moves. */
    mutable std::mt19937 gen { std::random_device {}() };

    action_type choose_action(
        const std::vector<action_type>& actions,
        const std::vector<weight_type>& weights) const
    {
        std::discrete_distribution dd(weights.begin(), weights.end());
        int chosen_ndx = dd(gen);
        return actions[chosen_ndx];
    }

    /**
     * Does nothing if the current player plays on the "Left-to-Right"
     * side of the board, reflects the hole index otherwise. (Basically,
     * rotates the board so that we can always consider the current
     * player is playing from our side.)
     *
     * The way we characterize the conditions of 'leading to a capture'
     * and 'being a doubleplay' are invariant under this transformation.
    */
    static inline int normalize_ndx(const Board& board, int hole_ndx)
    {
        return board.side_to_move() * hole_ndx
            + (1 - board.side_to_move()) * (5 - hole_ndx);
    }

    /**
     * Return true if the move leads to the player playing
     * again, false otherwise.
     */
    bool is_doubleplay(const Board& board, action_type hole_ndx) const
    {
        const auto& holes = board.holes(board.side_to_move());
        int n_beads = holes[hole_ndx] % 13;
        hole_ndx = normalize_ndx(board, hole_ndx);

        return hole_ndx + n_beads == 6;
    }

    /**
     * Return the number of beads captured if the move leads
     * to a capture, 0 otherwise.
     */
    int is_capture(const Board& board, action_type hole_ndx) const
    {
        const auto& holes = board.holes(board.side_to_move());
        const auto& opp_holes = board.holes(!board.side_to_move());
        int n_beads = holes[hole_ndx];

        // If we make more than a full circle around the board,
        // we cannot land on an empty hole anymore.
        if (n_beads > 12)
            return 0;

        // 'Rotate' the board so that we don't have to treat the
        // two players separately.
        hole_ndx = normalize_ndx(board, hole_ndx) + n_beads;

        bool still_my_side = hole_ndx < 6 || hole_ndx > 12;
        bool valid_capture = holes[hole_ndx] == 0 && opp_holes[hole_ndx] > 0;

        return (
            still_my_side && valid_capture ?
            opp_holes[hole_ndx] :
            0);
    }
};


/**
 * NOTE To go from player1 holes to player0, do hole --> 5 - hole
 * and vice-versa.
 */
class Oware_Playout_Func {
public:
    using action_type = Board::action_type;
    using weight_type = double;
    using weighted_action_type = std::pair<weight_type, action_type>;

    Oware_Playout_Func(Board&);

    int operator()();

    /**
     * As a simplified approach, this function either returns {false, *} if
     * it did not find a viable candidate, or it picks out a specific action
     * {true, action} to be applied unconditionally.
     *
     * (Localised probability distribution)
    */
    std::pair<bool, int> hard_choice(const std::vector<action_type>& va) const;

private:
    Board& board;

    /**
     * True if playing hole_ndx leads to a capture
     */
    std::pair<bool, int> is_capture(int hole_ndx) const;

    /**
     * Returns a vector of all moves leading to a capture along with
     * the number of beads that would be captured
     */
    std::vector<std::pair<action_type, int>> get_captures(const std::vector<action_type>&) const;

    /**
     * Pick the move with the largest amount of beads to be captured
     */
    std::pair<action_type, int> pick_capture(const std::vector<std::pair<int, int>>& captures) const;

    /**
     * Checks if the opponent has a move next that would end up in
     * a worse situation.
     */
    std::pair<bool, action_type> protect_captures(action_type) const;

    /**
     * Return true if playing that hole results in an extra turn.
     */
    bool is_double_play(int hole_ndx) const;

    /**
     * Pick the double_play closest to the player's mancala so as to
     * not mess with the other double plays.
     */
    int pick_double_play(std::vector<action_type>& doubles) const;

    /**
     * Add weights to the list of valid actions based on some heuristics
    */
    // std::vector<weighted_action_type> weighted_choice(const std::vector<action_type>& va) const
    // {
    //     // Return default weighted actions
    //     return trivial(va);
    // }

    // std::vector<weighted_action_type> trivial(const std::vector<action_type>& va) const
    // {
    //     std::vector<weighted_action_type> ret;
    //     std::vector<weight_type> w(va.size(), 1.0 / va.size());
    //     std::transform(va.begin(), va.end(), w.begin(), std::back_inserter(ret),
    //         [](double weight, int ndx) {
    //             return std::make_pair(ndx, weight);
    //         });
    //     return ret;
    // }

    // std::vector<weighted_action_type> make_weight_for_captures(
        //     const std::vector<action_type>& va,
        //     const std::vector<std::pair<int, int>>& captures) const
        // {
        //     std::vector<weighted_action_type> ret;

        //     double total = std::accumulate(captures.begin(), captures.end(), 0.0, [](auto m, const auto& p) {
        //         return m + p.second;
        //     });

        //     // Note that captures is sorted by hole_nb (the first parameter of the entries)
        //     auto it = captures.begin();
        //     for (const auto& a : va) {
        //         while (it->first < a) {
        //             ++it;
        //         }
        //         if (it->first > a)
        //             continue;

        //         // Now `a` corresponds to *it
        //         double ratio = it->second / total;
        //         ret.push_back(std::pair { a, ratio });
        //     }

        //     return ret;
        // }

    // std::vector<weighted_action_type>
    // make_weight_for_double_play(
    //     const std::vector<action_type>& va,
    //     std::vector<int>& doubles) const
    // {
    //     std::vector<weighted_action_type> ret;
    //     // Sort the doubles so that we can play the rightmost one
    //     // ( this way, we preserve the other doubles  )
    //     std::sort(doubles.begin(), doubles.end(), [](auto a, auto b) {
    //         return a < b;
    //     });

    //     // NOTE: When the current player is the one accross, we want
    //     // to pick the one with smallest index!
    //     int double_play = board.side_to_move() ? doubles.back() : doubles.front();

    //     auto weighted_actions = [&double_play](int ndx) {
    //         return ndx == double_play ? std::make_pair(1.0, ndx) : std::make_pair(0.0, ndx);
    //     };

    //     std::transform(va.begin(), va.end(), std::back_inserter(ret), weighted_actions);
    //     return ret;
    // }
};

} // namespace oware



#endif
