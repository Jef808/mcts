#ifndef __NODE_H_
#define __NODE_H_

/**
 * Requireemnts:
 *
 * State_T admits methods get_valid_actions(), is_terminal(), clone),
 * a (preferably mutable) apply_action()
*/
#include <algorithm>
#include <memory>
#include <vector>

namespace mcts {

/** Wrapper class around a state to build a search tree */
template <typename State_T, typename Action_T>
class Node {
public:
    using Node_T = Node<State_T, Action_T>;
    using node_sptr = std::shared_ptr<Node_T>;
    using node_uptr = std::unique_ptr<Node_T>;

    // Node(const State_T& _state)
    //     : state(_state)
    //     , children()
    //     , valid_actions(_state.get_valid_actions())
    //     , parent(nullptr)
    //     , parent_action(Action_T())
    //     , n_visits(0)
    //     , avg_value(0)
    // {}
    Node(const State_T& _state, Node_T* const _parent=nullptr, Action_T _parent_action=Action_T())
        : state(_state)
        , children()
        , valid_actions(_state.get_valid_actions())
        , parent(_parent)
        , parent_action(_parent_action)
        , n_visits(0)
        , avg_value(0)
    {}
    /** Update the average value and number of visits. */
    void update_stats(double val)
    {
        avg_value = (avg_value * n_visits + val) / (++n_visits);
    }
    /** Add a child corresponding to the given action. */
    Node_T& add_child(const Action_T& action) const
    {
        auto child_state = State_T(state).apply_action(action);
        return *children.emplace_back(std::make_shared<Node_T>(child_state, const_cast<Node_T* const>(this), action));
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
    Node_T* const get_parent() const
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
private:
    State_T state;
    std::vector<Action_T> valid_actions;
    mutable std::vector<node_sptr> children;
    Node_T* const parent;
    Action_T parent_action;
    int n_visits;
    double avg_value;
};
} // namespace mcts

#endif // __NODE_H_
