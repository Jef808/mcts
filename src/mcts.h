// As things are, StateT has to implement the following methods:
//
// - is_trivial(const ActionT& action) determining if an action is trivial.
// - evaluate(const ActionT& action)
// - evaluate_terminal()
// - valid_actions_data() returning a vector containing all valid actions
// - apply_random_action()
// - apply_action(const ActionT& action)
// - key()

#ifndef __MCTS_H_
#define __MCTS_H_

#include "mcts_tree.h"
#include "policies.h"

#include <iostream>

#include "utils/stopwatch.h"


namespace mcts {

template <
    typename StateT,
    typename ActionT,
    typename UCB_Functor = policies::Default_UCB_Func,
    typename Playout_Functor = typename policies::Default_Playout_Func<StateT, ActionT>,
    size_t MAX_DEPTH = 128
    >
class Mcts;

namespace display {
    template <typename _StateT,
              typename _ActionT,
              size_t _Depth>
    class Mcts_view;
} // namespace display

// Config Parameters
struct Config {
    double exploration_constant = 0.7;
    int max_iterations = 1000;
    int max_time = 10000;
    /** The number of simulations to run when initializing an edge. */
    int n_rollouts = 5;
};

template <
    typename StateT,
    typename ActionT,
    typename UCB_Functor,
    typename Playout_Functor,
    size_t MAX_DEPTH>
class Mcts
{
    friend class display::Mcts_view<StateT, ActionT, MAX_DEPTH>;
public:
    // Some typedefs for hygiene and forward declaration of the enums involved in
    // the configuration and choice of strategy.
    enum class ActionSelection;
    enum class BackpropagationStrategy;
    using reward_type = typename StateT::reward_type;
    using player_type = typename StateT::player_type;
    using ActionSequence = typename std::vector<ActionT>;
    enum class NPlayers { One, Two };
   /**
   * Constructor storing a reference to a state.
  */
    Mcts(StateT& state,
         UCB_Functor = UCB_Functor {});

    /**
   * Run the algorithm and generate the best playout it can.
  */
    ActionSequence
        best_action_sequence(ActionSelection = ActionSelection::by_best_value);

    /**
   * Run the algorithm but only return one action.
  */
    ActionT
        best_action(ActionSelection = ActionSelection::by_n_visits);

    /**
   * Run the algorithm until the `computation_resources()` returns false.
  */
    void run();

    /**
     * Sends a representation of the current tree in a json format
    */
    void display_tree(std::ostream&, int depth=0);

    /**
     * Apply a move to the root state, pruning all other root edges
     * @Note The action is pushed at the back of `m_actions_done`.
    */
    void apply_root_action(const ActionT&);

    /**
     * Return the time elapsed in milliseconds since the construction
     * of the agent or the last call to `init_counters()`.
    */
    std::chrono::milliseconds::rep time_elapsed() const
    {
        return m_stopwatch();
    }

    std::vector<std::pair<double, int>> root_moves_eval() const
    {
        node_pointer p_root = m_tree.get_root();
        std::vector<std::pair<double, int>> ret;
        std::transform(p_current_node->children.begin(),
                       p_current_node->children.end(),
                       std::back_inserter(ret),
                       [](const auto& e) {
                           return std::make_pair(e.total_val / (e.n_visits + 1.0), e.n_visits);
                       });
        return ret;
    }

    const StateT& state() const
    {
        return m_state;
    }
    const StateT& root_state() const
    {
        return m_root_state;
    }
    const size_t n_nodes() const
    {
        return m_tree.size();
    }

private:
    using Tree = MctsTree<StateT, ActionT, MAX_DEPTH>;
    //using node_type = typename Tree::Node*;
    using node_pointer = typename Tree::Node*;
    using edge_type = typename Tree::Edge;
    using edge_pointer = typename Tree::Edge*;

    StateT m_state;
    //StateT& m_state;
    Tree m_tree;
    node_pointer p_current_node;
    StateT m_root_state;
    UCB_Functor UCB_Func;

    Config m_config;
    BackpropagationStrategy backpropagation_strategy = BackpropagationStrategy::avg_value;
    ActionSequence m_actions_done;
    NPlayers n_players = NPlayers::Two;
    int iteration_cnt;
    ::utils::Stopwatch m_stopwatch;
public:
    using node_type = typename Tree::Node;
private:
    /**
   * Complete a full cycle of the algorithm.
  */
    void step();

    /**
   * Select the best edge from the current node according to the given method.
  */
    edge_pointer get_best_edge(ActionSelection);

    /**
   * The *Selection* phase of the algorithm.
   */
    edge_pointer Select_next_edge();

    /**
   * Traverse the tree to the next leaf to be expanded, using the ucb criterion
   * to select an edge at each node in the process.
  */
    void select_leaf();

    /**
   * Play random actions from the current state starting at the given action, until
   * we hit a terminal state. Return the total score.
   *
   * Optionally, specify how many simulations to run and return the average score.
  */
    reward_type simulate_playout(const ActionT&, int = 1);

    /**
     * For when the current node is a leaf, run `simulate_playout` on all the state's
     * valid actions and populate the current node with children edges corresponding
     * to those actions.
     *
     * @Note This increments the node's number of visits by 1 (and only does that when the
     * node had been visited before but is terminal).
   */
    void expand_current_node();

    /**
     * After expanding the leaf node, update the statistics of all edges connecting it
     * to the root with the results obtained from the simulated playouts, according to
     * the set BackpropagationStrategy.
   */
    void backpropagate();

    /**
     * Resets `m_current_node` with a reference to the root node, and reset the
     * state with the data from `m_root_state`.
   */
    void return_to_root();


    /**
     * Apply the edge's action to the state and update `m_current_node`.
   */
    void traverse_edge(edge_pointer);

    /**
   * Using the given selection method, return the best path from root to leaf according to
   * the statistics collected thus far.
   *
   * @Note If the sequence reaches unvisited nodes, `best_traversal` completes it with
   * random choices of edges.
  */
    ActionSequence best_traversal(ActionSelection);

    /**
   * The evaluation of the states.
  */
    reward_type evaluate(const ActionT&);

    /**
   * Evaluation of terminal states.
  */
    reward_type evaluate_terminal(const StateT&) const;

    reward_type evaluate_terminal() const
    {
        return StateT::evaluate_terminal(m_state);
    }

    /**
    * Return true if the agent can continue with the algorithm.
   */
    bool computation_resources();

    /**
    * Initialize the counters and time to 0.
   */
    void init_counters();

public:
    // Strategy options
    enum class BackpropagationStrategy {
        avg_value,
        avg_best_value,
        best_value
    };
    enum class ActionSelection {
        by_ucb,
        by_n_visits,
        by_avg_value,
        by_best_value
    };

    // Configuration options
    void set_exploration_constant(double c)
    {
        m_config.exploration_constant = c;
    }
    void set_backpropagation_strategy(BackpropagationStrategy strat)
    {
        backpropagation_strategy = strat;
    }
    void set_max_iterations(int n)
    {
        m_config.max_iterations = n;
        m_tree.reserve(n);
    }
    void set_max_time(int t)
    {
        m_config.max_time = t;
    }
    void set_n_players(NPlayers np)
    {
        n_players = np;
    }
    void set_n_rollouts(int n)
    {
        m_config.n_rollouts = n;
    }
    unsigned int get_iterations_cnt() const
    {
        return iteration_cnt;
    }
    size_t get_n_nodes() const
    {
        return m_tree.size();
    }
};

} // namespace mcts

#include "mcts.hpp"

#endif
