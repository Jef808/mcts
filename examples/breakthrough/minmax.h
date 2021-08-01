#ifndef MINMAX_H_
#define MINMAX_H_

#include "board.h"

#include <algorithm>

namespace minmax {

struct Default_ActionCmp {
    using action_type = BT::Position::action_type;

    double operator()(const action_type& a, const action_type& b) {
        return 0.5;
    }
};

template<typename ActionCmp = Default_ActionCmp>
class Agent {
public:
    using reward_type = double;
    using state_type = BT::Position;
    using action_type = BT::Position::action_type;

    Agent() = default;

    action_type best_action(const state_type& state)
    {
        if (state.is_terminal())
            return action_type::Null;

        auto actions = state.valid_actions();

        return *std::max_element(actions.begin(),
                                 actions.end(),
                                 [*this, &state](const auto& a, const auto& b)
        {
            return evaluate(state, a) < evaluate(state, b);
        });
    }

    void set_beam_width(int w) {
        beam_width = w;
    }
private:
    int beam_width;

    reward_type evaluate(const state_type& state, const action_type& a) const
    {
        state_type _state = state;
        _state.apply_action(a);

        if (_state.is_terminal())
        {
            // 1 if the last action was a win, 0 if it was a loss,
            // or something similar.
            return state_type::evaluate_terminal(_state);
        }

        auto actions = _state.valid_actions();

        /**
         * Select only a subset of the actions depending on the beam_width
         * and ActionCmp parameters.
        */
        std::sort(actions.begin(), actions.end(), [](const auto& a, const auto& b){
            return ActionCmp(a, b) > 0.55 ? true: false;
        });

        if (beam_width > 0 && actions.size() > beam_width) {
            actions.resize(beam_width);
        }


        std::vector<reward_type> evaluations;

        std::transform(actions.begin(),
                       actions.end(),
                       std::back_inserter(evaluations),
                       [*this, &_state](const auto& b)
        {
            return evaluate(_state, b);
        });

        return 1 - *std::max(evaluations.begin(), evaluations.end());
    }
};







} // namespace minmax

#endif // MINMAX_H_
