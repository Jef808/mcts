#ifndef __NODE_H_
#define __NODE_H_

/**
 * Requireemnts:
 *
 * State_T admits methods get_valid_actions(), is_terminal(), clone),
 * a (preferably mutable) apply_action()
*/
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
//#include "../tikztreeold/dfs.hpp"

namespace mcts {


/** Wrapper class around a state to build a search tree */
template <typename State_T, typename Action_T>
class Node {
public:
    using Node_T = Node<State_T, Action_T>;
    using node_sptr = std::shared_ptr<Node>;
    using node_uptr = std::unique_ptr<Node>;

    // Node(const State_T& _state)
    //     : state(_state)
    //     , children()
    //     , valid_actions(_state.get_valid_actions())
    //     , parent(nullptr)
    //     , parent_action(Action_T())
    //     , n_visits(0)
    //     , avg_value(0)
    // {}
    Node(const State_T& _state, Node* const _parent = nullptr, Action_T _parent_action = Action_T())
        : state(_state)
        , children()
        , valid_actions(_state.get_valid_actions())
        , parent(_parent)
        , parent_action(_parent_action)
        , n_visits(1)
        , avg_value(0)
    {
    }
    // Node(const Node& other)
    //     : state(other.state)
    //     , valid_actions(other.valid_actions)
    //     , children(other.children)
    //     , parent(other.parent)
    //     , parent_action(other.parent_action)
    //     , n_visits(other.n_visits)
    //     , avg_value(other.avg_value)
    // {
    // }
    // Node& operator=(const Node& other)
    // {
    //     if (&other == this)
    //     {
    //         return *this;
    //     }
    //     state = other.state;
    //     valid_actions = other.valid_actions;
    //     children.clear();
    //     std::copy(begin(other.children), end(other.children), begin(children));
    //     parent = const_cast<Node* const>(&other);
    //     parent_action = other.parent_action;
    //     n_visits = other.n_visits;
    //     avg_value = other.avg_value;
    // }
    // ~Node()
    // {
    //     children.clear();
    //     valid_actions.clear();
    //     //parent = nullptr;
    //     parent_action = Action_T();
    //     n_visits = 1;
    //     avg_value = 0;
    // }

    // Node* clone() const
    // {
    //     return new Node(*this);
    // }
    /** Update the average value and number of visits. */
    void update_stats(double val)
    {
        avg_value = (avg_value * n_visits + val) / (++n_visits);
    }
    /** Add a child corresponding to the given action. */
    Node& add_child(const Action_T& action) const
    {
        auto child_state = State_T(state);
        child_state.apply_action(action);
        return *children.emplace_back(std::make_shared<Node>(child_state.apply_action(action), const_cast<Node* const>(this), action));
    }
    /**
     * Apply an action to the state and update the valid actions vector.
     * It is assumed that `children` and `parent` are unspecified.
    */
    void apply_action_rollout(const Action_T& action)
    {
        state.apply_action(action);
        valid_actions = state.get_valid_actions();
    }
    /** Check if there are any valid actions from current state. */
    bool is_terminal() const
    {
        return state.is_terminal();
    }
    /** Check if every valid action has been expanded into a child node. */
    bool is_fully_expanded() const
    {
        return valid_actions.size() == children.size();
    }
    const State_T& get_state() const
    {
        return state;
    }
    Node* const get_parent() const
    {
        return parent;
    }
    const std::vector<Action_T>& get_valid_actions() const
    {
        return valid_actions;
    }
    /** The action that lead to the current state. */
    const Action_T& get_parent_action() const
    {
        return parent_action;
    }
    const std::vector<node_sptr>& get_children() const
    {
        return children;
    }
    int get_n_visits() const
    {
        return n_visits;
    }
    double get_avg_value() const
    {
        return avg_value;
    }
    operator std::string() const
    {
        return std::string(state);
    }

private:

    State_T state;
    std::vector<Action_T> valid_actions;
    mutable std::vector<node_sptr> children;
    Node* const parent;
    Action_T parent_action;
    int n_visits;
    double avg_value;
};
} // namespace mcts

#endif // __NODE_H_
