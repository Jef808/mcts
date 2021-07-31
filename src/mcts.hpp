#ifndef __MCTS_HPP_
#define __MCTS_HPP_

#include "mcts.h"
#include "mcts_tree.h"
#include "policies.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <thread>
#include <vector>


#include "utils/stopwatch.h"

namespace mcts {

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::Mcts(
    StateT& state, UCB_Functor ucb_func)
    : m_state(state)
    , m_tree(state.key())
    , p_current_node(m_tree.get_root())
    , m_root_state(state)
    , UCB_Func { ucb_func }
    , m_stopwatch {}
{
    m_tree.reserve(m_config.max_iterations);
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline ActionT Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::best_action(
    ActionSelection method)
{
    run();
    return_to_root();
    edge_pointer edge = get_best_edge(method);

    return edge->action;
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::ActionSequence
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::best_action_sequence(
    ActionSelection method)
{
    run();
    return best_traversal(method);
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::run()
{
    init_counters();
    return_to_root();
    if (p_current_node->n_visits > 0 && p_current_node->children.size() == 0) {
        return;
    }
    while (computation_resources()) {
        step();
    }
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::step()
{
    return_to_root();
    select_leaf();
    expand_current_node();
    backpropagate();
    ++iteration_cnt;
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::select_leaf()
{
    // Once there is only one terminal node in the tree, we have no choice
    // but to pick that as the leaf
    if (p_current_node->n_visits > 0 && p_current_node->children.empty())
    {
        ++p_current_node->n_visits;
        return;
    }

    while (p_current_node->n_visits > 0 && !p_current_node->children.empty())
    {
        ++p_current_node->n_visits;

        // The next edge along which to travel (Note that it is the best edge
        // from the point of view of the current player, so we get minimax behavior)
        edge_pointer edge = get_best_edge(ActionSelection::by_ucb);

        #ifdef DEBUG
            if (edge->player != m_state.side_to_move())
            {
                std::cerr << "WARNING: In select_leaf(): "
                      << "Edge / state player mismatch..." << std::endl;
            }
        #endif

        traverse_edge(edge);
    }
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::edge_pointer
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::get_best_edge(
    ActionSelection method)
{
    auto cmp = [&](const auto& a, const auto& b) {
        if (method == ActionSelection::by_ucb) {
            auto ucb = UCB_Func( m_config.exploration_constant,
                p_current_node->n_visits );
            return ucb(a) < ucb(b);
        }
        if (method == ActionSelection::by_n_visits)
            return a.n_visits < b.n_visits;

        if (method == ActionSelection::by_avg_value)
            return a.total_val / (a.n_visits + 1.0) < b.total_val / (b.n_visits + 1.0);

        return a.best_val < b.best_val;
    };

    #ifdef DEBUG
        std::cerr << "\n\nAt node with player state.side_to_move() = "
                  << m_state.side_to_move()
                  << "...\nChoosing the best edge amongst "
                  << p_current_node->children.size()
                  << " children"
                  << std::endl;
    #endif

    auto& children = p_current_node->children;
    auto it = std::max_element(children.begin(), children.end(), cmp);

    return (!children.empty() ? &*it : nullptr);
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
typename StateT::reward_type
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::simulate_playout(
    const ActionT& action, int n_reps)
{
    player_type player = m_state.side_to_move();

    // If the current state is already terminal,
    // return the terminal evaluation from the correct player point of view.
    if (m_state.is_terminal())
    {
        reward_type terminal_eval = evaluate_terminal();

#ifdef DEBUG
            std::cerr << "\nState is terminal with evaluation "
                  << std::setprecision(2) << terminal_eval
                  << std::endl;
#endif

        // This means the whole tree consists of one terminal node and we have no information on
        // the previous player (nothing matters at this point)
        if (m_tree.depth() == 0)
        {
            return 1.0 - terminal_eval;
        }

        // If there is a parent edge, it represents the last action
        // of the player who finished the game, so the evaluate_terminal() result
        // is from their point of view.
        else
        {
            if (m_tree.parent()->player != player)
            {
                terminal_eval = 1.0 - terminal_eval;
            }

            return terminal_eval;
        }
    }

    // Backup the state, initialize local vars and apply the initial action.
    StateT backup = m_state;
    ActionT _action = action;
    reward_type score = 0.0;

    score += backup.evaluate(_action) * n_reps;
    backup.apply_action(_action);

    for (int i=0; i<n_reps; ++i)
    {
        reward_type _sim_score = 0.0;
        StateT _sim = backup;

        // NOTE: The Playout functor stores a reference to the state it's
        // passed in its constructor
        Playout_Functor Playout_Func{ _sim };

        // NOTE: To evaluate actions returned by the Playout_Functor from the
        // state before applying the action. (Should be taken cared of inside
        // of Playout_Func ideally...)
        StateT _sim_prev = _sim;

        while (!_sim.is_terminal())
        {
            _sim_prev = _sim;
            _action = Playout_Func();
            _sim_score += _sim_prev.evaluate(_action);
#ifdef DEBUG
            std::cout << _sim_prev << '\n'
                      << "Action: "
                      << _action << "\n**************" << std::endl;
#endif
        }

        // Compute the terminal evaluation and negate it if the player
        // who played last is not the same as the player 'running' this
        // simulation.
        reward_type eval_terminal = StateT::evaluate_terminal(_sim);

        if (~_sim.side_to_move() != player)  // Last_player = !_sim.side_to_move()
        {
            eval_terminal = 1.0 - eval_terminal;
        }

        score += _sim_score + eval_terminal;
    }

    // Return the average of the above scores
    return score / n_reps;
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::expand_current_node()
{
    auto valid_actions = m_state.valid_actions();
    const player_type player = m_state.side_to_move();

    for (auto a : valid_actions) {
        edge_type new_edge {
            .action = a,
            .player = player,
        };
        reward_type val = simulate_playout(a, m_config.n_rollouts);
        new_edge.best_val = new_edge.total_val = val;
        p_current_node->children.push_back(new_edge);
    }

    ++p_current_node->n_visits;

    #ifdef DEBUG
        std::cerr << "\n\n\nUsing 5 simulations each, expanding the "
              << valid_actions.size()
              << " children of "
              << m_state
              << " **Player " << player
              << "...\nFound:\n";
        for (const auto e : p_current_node->children)
        {
            std::cerr << "Action " << e.action
                      << " Player " << e.player
                      << " total_val " << e.total_val
                      << " best_val " << e.best_val
                      << " n_visits " << e.n_visits
                      << std::endl;
        }
    #endif
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::backpropagate()
{
    reward_type val = 0.0;
    player_type player_pov = m_state.side_to_move();

    auto cmp_avg_val = [](const auto& a, const auto& b)
    {
        return a.total_val/(1.0 + a.n_visits)
            < b.total_val/(1.0 + b.n_visits);
    };

    if (m_state.is_terminal())
    {
        val = evaluate_terminal();

        if (m_tree.depth() > 0)
        {
            val = m_tree.parent()->player != player_pov ? 1.0 - val : val;
        }
        else
        {
            val = 1.0 - val;
        }
    }
    else
    {
        // Collect the best average reward amongst the current node's children
        auto with_best_avg = std::max_element(p_current_node->children.begin(),
                                              p_current_node->children.end(),
                                              cmp_avg_val);

        val = with_best_avg->total_val/(1.0 + with_best_avg->n_visits);

        #ifdef DEBUG

            std::cerr << "\n\nWe now backpropagate the best value we got from the simulations, "
                  << "which is "
                  << std::setprecision(2)
                  << val
                  << std::endl;
        #endif
    }
    
    m_tree.backpropagate(val, player_pov);
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::traverse_edge(
    edge_pointer edge)
{
    if (m_state.apply_action(edge->action))
    {
        m_tree.traversal_push(edge);
        p_current_node = m_tree.get_node(m_state.key());
    }
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::ActionSequence
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::best_traversal(
    ActionSelection method)
{
    if (m_state != m_root_state) {
        return_to_root();
    }

    edge_pointer p_nex_edge;
    while (p_current_node->n_visits > 0 && p_current_node->children.size() > 0) {
        p_nex_edge = get_best_edge(method);
        traverse_edge(p_nex_edge);

        m_actions_done.push_back(p_nex_edge->action);
    }

    if (p_current_node->n_visits > 0) {
        return m_actions_done;
    }

    ActionT action = m_state.apply_random_action();

    while (!m_state.is_trivial(action)) {
        m_actions_done.push_back(action);
        action = m_state.apply_random_action();
    }

    return m_actions_done;
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::return_to_root()
{
    p_current_node = m_tree.get_root();
    m_state = m_root_state;
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::apply_root_action(
    const ActionT& action)
{
    m_root_state.apply_action(action);
    m_tree.set_root(m_root_state.key());
    return_to_root();

    m_actions_done.push_back(action);
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline bool Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::computation_resources()
{
    auto time = m_stopwatch();
    bool time_ok = m_config.max_time > 0 ? time < m_config.max_time : true;
    bool iterations_ok = m_config.max_iterations > 0 ? iteration_cnt < m_config.max_iterations : true;
    return time_ok && iterations_ok;
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
inline void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::init_counters()
{
    iteration_cnt = 0;
    m_stopwatch.reset_start();
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::reward_type inline Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::evaluate(const ActionT& action)
{
    return m_state.evaluate(action);
}

template <typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::reward_type inline Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::evaluate_terminal(const StateT& state) const
{
    return state.evaluate_terminal();
}

} // namespace mcts

#endif // MCTS_HPP_
