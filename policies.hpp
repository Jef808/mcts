#ifndef __DEFAULTPOLICIES_H_
#define __DEFAULTPOLICIES_H_

#include "node.hpp"
#include <algorithm>
#include <cstdlib>
#include <math.h>
#include <memory>
#include <vector>

namespace mcts {

namespace policies {

    template <typename State_T, typename Action_T>
    struct RandomRollout {
        using Node_T = Node<State_T, Action_T>;
        using Node_uptr = std::unique_ptr<Node_T>;

        RandomRollout() { }
        Action_T operator()(const Node_T& node)
        {
            auto valid_actions = node.get_valid_actions();
            auto action_ndx = rand() % valid_actions.size();

            return valid_actions[action_ndx];
        }
    };
    /**
    * The way we select nodes to traverse the tree.
    *
    * Select the child maximizing
    * avg_value-to-date + c ( ln( n_visits/n_itertions  )  ),
    * where c would be the branching factor in a perfect world.
    */
    template <typename State_T, typename Action_T>
    struct UCT1 {
        using Node_T = Node<State_T, Action_T>;

        UCT1(double b_fact = sqrt(2))
            : BR_FACT(b_fact)
        {}
        Node_T* operator()(Node_T* node)
        {
            return *std::max_element(begin(node->get_children()), end(node->get_children()),
                [c = BR_FACT](auto* child) {
                    return child->get_avg_value() + c * sqrt(log(child->get_parent()->get_n_visits()) / child->get_n_visits());
                });
        }
    private:
        double BR_FACT;
    };
} // policies
} // mcts
#endif // __DEFAULTPOLICIES_H_
