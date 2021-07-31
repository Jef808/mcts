#include "oware_mcts.h"
#include "oware.h"

#include <functional>
#include <iostream>
#include <stack>
#include <vector>

#include "utils/rand.h"

namespace oware {

Oware_Playout_Func::Oware_Playout_Func(Board& _board)
    : board(_board)
{
}

int Oware_Playout_Func::operator()()
{
    auto va = board.valid_actions();
    auto [found, hole_ndx] = hard_choice(va);

    if (found)
    {
        board.apply_action(hole_ndx);
        return hole_ndx;
    }

    // Otherwise could still use a fancier weight scheme instead of random

    return board.apply_random_action();
}

////////////////////////////////////////////////////////////////////////////////
// Free functions
////////////////////////////////////////////////////////////////////////////////

/**
 * Return [true, capture_size] if playing that hole results in a capture.
*/
std::pair<bool, int> Oware_Playout_Func::is_capture(
    int hole_ndx) const
{
    const auto& holes = board.holes(board.side_to_move());
    const auto& other_holes = board.holes(!board.side_to_move());

    auto capture_cond = [&holes, &other_holes](int _hole_ndx){
        return holes[_hole_ndx] == 0
            && other_holes[_hole_ndx] != 0;
    };

    // By reducing mod 13, we get the landing hole in a very neat way:
    int mod13 = holes[hole_ndx] % 13;
    bool land_same_side = (
        board.side_to_move() ?
        mod13 < 6 - hole_ndx :
        mod13 < hole_ndx + 1
    );

    if (land_same_side)
    {
        int dest = board.side_to_move() ? hole_ndx + mod13 : hole_ndx - mod13;
        if (capture_cond(dest))
            return std::make_pair(true, other_holes[dest]);
    }

    return std::make_pair(false, 0);
}

std::vector<std::pair<Oware_Playout_Func::action_type, int>> Oware_Playout_Func::get_captures(
    const std::vector<action_type>& va) const
{
    std::vector<std::pair<int, int>> captures;

    for (action_type hole_ndx : va) {
        std::pair<bool, int> pbi = is_capture(hole_ndx);
        if (pbi.first) {
            captures.push_back(std::pair { hole_ndx, pbi.second });
        }
    }

    return captures;
}

std::pair<Board::action_type, int> Oware_Playout_Func::pick_capture(
    const std::vector<std::pair<int, int>>& captures) const
{
    return *std::max_element(captures.begin(), captures.end(), [](const auto a, const auto b) {
        return a.second < b.second;
    });
}

/**
 * Check if there are dangerous captures for the opponent on next
 * turn if we play some given action.
 */
std::pair<bool, Oware_Playout_Func::action_type> Oware_Playout_Func::protect_captures(
    action_type action) const
{
    auto ret = std::make_pair(false, action);

    Board play_board = board;
    auto my_old_man = play_board.mancala(play_board.side_to_move());
    auto opp_old_man = play_board.mancala(!play_board.side_to_move());

    play_board.apply_action(action);

    // FIXME get_captures uses the Board reference of *this... 
    auto nex_va = play_board.valid_actions();
    auto opp_captures = get_captures(nex_va);

    if (opp_captures.empty())
    {
        return ret;
    }

    auto [opp_action, opp_capture_size] = pick_capture(opp_captures);
    play_board.apply_action(opp_action);
        
    auto my_new_man = play_board.mancala(!play_board.side_to_move());
    auto opp_new_man = play_board.mancala(play_board.side_to_move());

    bool i_lose = opp_new_man > 24;
    bool is_worse = my_old_man - opp_old_man > my_new_man - opp_new_man;

#ifdef DEBUG
    std::cerr << "\n***************"
              << "\nPlayer" << board.side_to_move()
              << " is considering playing "
              << action
              << " on board\n"
              << board
              << "\nOpponent would have move "
              << opp_action
              << " resulting in\n"
              << play_board
              << "\nWe choose "
              << (i_lose || is_worse ? "NOT" : "")
              << " to play this move"
              << "\n***************"
              << std::endl;
#endif

    // If that capture would be bad, distribute the beads of that hole
    if (i_lose || is_worse)
    {
        const auto& my_holes = play_board.holes(!play_board.side_to_move());
        const auto& their_holes = play_board.holes(play_board.side_to_move());

        int their_n_beads = their_holes[opp_action];

        int last_hole_nb = (
            play_board.side_to_move() ?
            (opp_action + their_n_beads) % 13 :
            (opp_action + 13 - their_n_beads) % 13);

        return std::make_pair(true, last_hole_nb);
    }

    return ret;
}


std::pair<bool, int> Oware_Playout_Func::hard_choice(const std::vector<action_type>& va) const
{
    #ifdef DEBUG
    std::cerr << "\n\nIn hard_choice()"
              << "\nPlayer is " << board.side_to_move()
              << ", valid actions are:\n";
    for (auto a : va)
        std::cerr << a << ' ';
    #endif

    // If the game is already won, just play a random move
    if (board.mancala(0) > 23 || board.mancala(1) > 23)
        return std::make_pair(0, 0);

    std::vector<int> doubles = [&]() {
        std::vector<int> ret;
        std::copy_if(va.begin(), va.end(), std::back_inserter(ret), [&](int hole_ndx) {
            return is_double_play(hole_ndx);
        });
        return ret;
    }();

    // Prioritize actions that give the player an extra turn
    if (!doubles.empty()) {
        #ifdef DEBUG
        std::cerr << "\nFound a doubleplay" << std::endl;
        #endif
        return std::make_pair(true, pick_double_play(doubles));
    }

    auto captures = get_captures(va);

    // Next prioritize the captures
    if (!captures.empty()) {
        #ifdef DEBUG
        for (auto c : captures)
        {
            std::cerr << "Index: " << c.first
                      << " for a capture of "
                      << c.second
                      << std::endl;
        }
        #endif

        auto [action, capture_size] = pick_capture(captures);
        auto ret = std::make_pair(true, action);

        auto man = board.mancala(board.side_to_move());

        // If it is winning, just do it.
        if (man + 1 + capture_size > 24)
            return ret;

        //Otherwise check to see if opponent has some dangerous capture
        //next turn
        // auto [danger, new_action] = protect_captures(action);

        // if (danger)
        //     return std::make_pair(true, new_action);

        return ret;
    }

    return std::make_pair(false, 0);
}

bool Oware_Playout_Func::is_double_play(int hole_ndx) const
{
    const auto& holes = board.holes(board.side_to_move());
    // After removing the difference between the mancala and the hole,
    // we can easily check if the final bead is dropped in the mancala
    // by reducing mod 13 (the number of holes when going around the board)
    return board.side_to_move() ?
        (holes[hole_ndx] % 13 == hole_ndx-6):
        (holes[hole_ndx] % 13 == hole_ndx+1);
}

int Oware_Playout_Func::pick_double_play(std::vector<action_type>& doubles) const
{
    /**
     * The bigger indices are the ones close to the mancala for the
     * `oppposite` player.
    */
    auto cmp = [&](auto a, auto b) {
        return board.side_to_move() ? a < b : a > b;
    };

    std::sort(doubles.begin(), doubles.end(), cmp);

    return doubles.back();
}

} // namespace oware
