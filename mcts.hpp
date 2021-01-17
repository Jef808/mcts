#ifndef __AGENT_H_
#define __AGENT_H_

/**
 * Requirements:
 * State_T implements : is_terminal(), apply_action()
 * Action_T implements: binary operator==
 *
 * EvaluationPolicy<State, Action>() : const Node_T& --> Reward
 * RolloutPolicy<State, Action>()    : unique_ptr<Node_T> --> unique_ptr<Node_T>
 */
#include "node.hpp"
#include "policies.hpp"
#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <math.h>
#include <memory>
#include <random>
#include <string>

namespace mcts {

template <
    class State_T,
    class Action_T,
    typename EvaluationPolicy,
    template <class, class> class RolloutPolicy
    >
class Agent {
public:
    using Node_T = Node<State_T, Action_T>;
    using node_sptr = std::shared_ptr<Node_T>;
    using node_uptr = std::unique_ptr<Node_T>;
    typedef RolloutPolicy<State_T, Action_T> Rollout;

    Agent(int _max_iterations = 1000, int _max_rollout_depth = 100, double br_fact = sqrt(2))
        : max_iterations(_max_iterations)
        , max_rollout_depth(_max_rollout_depth)
        , BR_FACT(br_fact)
    {}
    Action_T get_best_action(const State_T& state) const
    {
        // Run n_iterations steps of the MCTS algorithm.
        auto root = Node_T(state);
        for (int i = 0; i < max_iterations; ++i) {
            step(root);
        }
        // DEBUG
        for (const auto& child : root.get_children()) {
            std::cerr << "Action " << child->get_parent_action() << " has:" << std::endl;
            std::cerr << "Value : " << child->get_avg_value() << " and n_visits : " << child->get_n_visits() << std::endl;
        }
        // Get the most visited child.
        return most_visited_action(root);
        // Get the highest avg value child.
        //return best_avg_value_child(root)->get_parent_action();
    }
private:
    int max_iterations;
    int max_rollout_depth;
    double BR_FACT;

    Action_T most_visited_action(const Node_T& node) const
    {
        return (*std::max_element(cbegin(node.get_children()), cend(node.get_children()),
            [](const auto a, const auto b) {
                return a->get_n_visits() < b->get_n_visits();
            }))
            ->get_parent_action();
    }
    Action_T best_avg_value_child(const Node_T& node) const
    {
        return (*std::max_element(cbegin(node.get_children()), cend(node.get_children()),
            [](const auto& a, const auto& b) {
                return a->get_avg_value() < b->get_avg_value();
            }))
            ->get_parent_action();
    }
    /** Select the (immediate) child of node with best uct score. */
    Node_T* best_uct_child(const Node_T& node) const
    {
        auto uct1_value = [c = BR_FACT](const Node_T& node) {
            return node.get_avg_value() + c * sqrt(log(node.get_parent()->get_n_visits()) / node.get_n_visits());
        };

        return std::max_element(cbegin(node.get_children()), cend(node.get_children()),
            [&uct1_value](const auto& a, const auto& b) {
                return uct1_value(*a) < uct1_value(*b);
            })
            ->get();
    }
    /** Choose (and add) an unexplored child for the next rollout. */
    Node_T& expand(const Node_T& node) const
    {
        //assert(!node.is_terminal() && !node.is_fully_expanded());
        Action_T action;
        if (node.get_children().empty()) {
            return node.add_child(node.get_valid_actions().back());
        }
        auto action_unexplored = [&node](const Action_T& a) {
            return std::find_if(cbegin(node.get_children()), cend(node.get_children()), [&a](const auto& child) {
                return child->get_parent_action() == a;
            }) == cend(node.get_children());
        };
        action = *std::find_if(cbegin(node.get_valid_actions()), cend(node.get_valid_actions()), action_unexplored);

        return node.add_child(action);
    }
    // /** Return a random action from the valid actions. */
    // const Action_T choose_random_action(const State_T& state) const
    // {
    //     auto actions = state.get_valid_actions();
    //     return actions[rand() % actions.size()];
    // }
    /** Interface for evaluating a state during rollout */
    double evaluate(const Node_T& node) const
    {
        return EvaluationPolicy()(node.get_state(), node.get_parent_action());
    }
    /** Simulate a complete playout and return a reward */
    double rollout(const Node_T& node) const
    {
        auto rnode = Node_T(node);
        auto depth = 0;
        while (!rnode.is_terminal() && depth < max_rollout_depth) {
            rnode.apply_action_rollout(Rollout()(rnode));
            ++depth;
        }
        return evaluate(rnode);
    }
    /** Update the statistics of every node sitting on top */
    void backpropagate(Node_T* const node, double rollout_score) const
    {
        if (node == nullptr) {
            return;
        }
        node->update_stats(rollout_score);
        backpropagate(node->get_parent(), rollout_score);
    }
    /** A full step of the MCTS algorithm. */
    void step(Node_T& root) const
    {
        // Selection
        Node_T* node = &root;

        while (!node->is_terminal() && node->is_fully_expanded()) {
            //std::cerr << "Selecting best uct child" << std::endl;
            node = best_uct_child(*node);
        }

        //std::cerr << "Selected node : " << node->get_state() << std::endl;
        //std::cerr << "It has " << node->get_children().size() << " children" << std::endl;
        // Expansion
        if (node->is_terminal()) {
            return;
        }

        // At this point, not terminal and not fully expanded...
        Node_T& child_node = expand(*node);
        //std::cerr << "Expanded node : " << node->get_state() << std::endl;
        // Rollout
        auto reward = rollout(child_node);
        //std::cerr << "Reward : " << reward << std::endl;
        // Backpropagate
        backpropagate(node, reward);
    }
};
}

#endif // __AGENT_H_
