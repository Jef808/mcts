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
    typename SelectionPolicy,
    template <class, class> class RolloutPolicy = policies::RandomRollout
    >
class Agent {
    using Node_T = Node<State_T, Action_T>;
    using node_sptr = std::shared_ptr<Node_T>;
    using node_uptr = std::unique_ptr<Node_T>;
     //using agent_t = Agent<State_T, Action_T, EvaluationPolicy, RolloutPolicy>;
    typedef RolloutPolicy<State_T, Action_T> Rollout;

public:

    Agent(int _max_iterations = 10000, int _max_rollout_depth = 100, double br_fact = sqrt(2))
        : max_iterations(_max_iterations)
        , max_rollout_depth(_max_rollout_depth)
        , BR_FACT(br_fact)
    {
    }

    Action_T get_best_action(const State_T& state) const
    {
        Node_T root{state};
        // Run n_iterations steps of the MCTS algorithm.


        int n_nodes_expanded = 0;


        for (int i = 0; i < max_iterations; ++i) {
            step(root, n_nodes_expanded);
            // if (i % 1000 == 0)
            // {
            //     root.print_tree("tikz/tikztree_" + std::to_string(i) + ".tex", 15, 10);
            // }
        }
        // Get the most visited child.
        //return most_visited_action(root);
        //delete root;
        //return best_action;

        // Get the child with best avg score.

        std::cerr << "Number of nodes expanded : " << n_nodes_expanded << std::endl;
        auto candidates = root.get_children();

        std::sort(candidates.begin(), candidates.end(),
                                [](const auto& a, const auto& b)
                                {
                                    return a->get_avg_value() < b->get_avg_value();
                                });

        std::cerr << "The actions have the following stats:" << std::endl;

        for (const auto& cand : candidates)
        {
            std::cerr << cand->get_parent_action() << " avg value : " << cand->get_avg_value() << " number visits : " << cand->get_n_visits() << std::endl;

            std::cerr << "Second term in UCT formula without the constant: " << sqrt(log(cand->get_parent()->get_n_visits()) / cand->get_n_visits()) << std::endl;
        }
        std::cerr << std::endl;

        auto best_avg_value = candidates.back()->get_parent_action();
        return best_avg_value;

        // std::sort(candidates.begin(), candidates.end(),
        //           [](const auto& a, const auto& b)
        //           {
        //               return a->get_n_visits() < b->get_n_visits();
        //           });

        //return candidates.back()->get_parent_action();

    }

protected:
    /** A full step of the MCTS algorithm. */
    void step(Node_T& root, int& n_nodes_expanded) const
    {
        // Selection (might not change node = &root if it has no children)
        Node_T* node = select_leaf(root);

        // Expansion
        if (node->is_terminal()) {
            backpropagate(node, evaluate(*node));
            return;
        }

        // At this point, not terminal and not fully expanded...
            // Node_T* child_node = expand(*node);
            node = expand(*node);

            ++n_nodes_expanded;

        // Rollout
        double reward = rollout(*node);
        // Backpropagate
        backpropagate(node, reward);
    }

private:
    int max_iterations;
    int max_rollout_depth;
    double BR_FACT;

    Action_T most_visited_action(const Node_T& node) const
    {
        return (*std::max_element(begin(node.get_children()), end(node.get_children()),
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
        return SelectionPolicy()(node);
        // auto uct1_value = [c = BR_FACT](const Node_T& node) {
        //     return node.get_avg_value() + c * sqrt(log(node.get_parent()->get_n_visits()) / node.get_n_visits());
        // };



        // return std::max_element(begin(node.get_children()), end(node.get_children()),
        //                                         [&uct1_value](const auto& a, const auto& b) {
        //                                             if (a->get_n_visits() < 2)
        //                                             {
        //                                                 return false;
        //                                             }
        //                                             if (!a->is_terminal() && a->get_n_visits() < 20)
        //                                             {
        //                                                 return false;
        //                                             }
        //                                             return uct1_value(*a) < uct1_value(*b);
        //                                         })
        //                     ->get();
    }

    Node_T* select_leaf(Node_T& root) const
    {
        Node_T* node = &root;

        while (true)
        {
            if (!node->is_fully_expanded())
            {
                return node;
            }
            if (node->get_n_visits() < 30)
            {
                return node;
            }
            // if (node->get_children().empty())
            // {
            //      return node;
            // }
            if (node->is_terminal())
            {
                return node;
            }
            node = best_uct_child(*node);
        }
    }
    /** Choose (and add) an unexplored child for the next rollout. */
    Node_T* expand(Node_T& node) const
    {
        if (node.is_terminal() || node.is_fully_expanded())
        {
            return &node;
        };

        Action_T child_action{};

        if (node.get_children().empty() && !node.is_terminal())
        {
            child_action = node.get_valid_actions()[rand() % node.get_valid_actions().size()];
        }
        else
        {
            for (const auto& action : node.get_valid_actions())
            {
                bool is_seen = std::any_of(begin(node.get_children()), end(node.get_children()), [a = action](const auto& child) {
                    return child->get_parent_action() == a;
                });
                if (!is_seen)
                {
                    child_action = action;
                    break;
                }
            }
        }
        return &(node.add_child(child_action));
    }
    /** Interface for evaluating a state during rollout */
    double evaluate(const Node_T& node) const
    {
        return EvaluationPolicy() (node.get_state(), node.get_parent_action());
    }
    /** Simulate a complete playout and return a reward */
    double rollout(const Node_T& node) const
    {
        Node_T rollout_node{node};
        auto depth = 0;
        while (!rollout_node.is_terminal() && depth < max_rollout_depth) {
            rollout_node.apply_action_rollout(Rollout()(rollout_node));
            ++depth;
        }
        auto reward = evaluate(rollout_node);
        //delete rollout_node;
        return reward;
    }
    /** Update the statistics of every node sitting on top */
    void backpropagate(Node_T* node, double rollout_score) const
    {
        if (node == nullptr) {
            return;
        }
        node->update_stats(rollout_score);
        backpropagate(node->get_parent(), rollout_score);
    }
};
}

#endif // __AGENT_H_
