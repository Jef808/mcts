#ifndef __AGENT_RANDOM_H_
#define __AGENT_RANDOM_H_

#include "utils/stopwatch.h"

#include <iostream>
#include <chrono>


/**
 * A simple random agent with similar interface to the
 * mcts agent for benchmarking and testing purposes.
 */
template<typename Board>
class Agent_random
{
public:
    using reward_type = typename Board::reward_type;
    using action_type = typename Board::action_type;
    using player_type = typename Board::player_type;

    Agent_random(const Board& b) :
        m_state(b), m_root_state(b)
    {
    }

    void set_max_iterations(int n)
    {
        max_iters = n;
    }
    void set_max_time(int n)
    {
        max_time = n;
    }
    bool playout(action_type a)
    {
        Board b = m_state;
        player_type player = b.side_to_move();

        b.apply_action(a);

        while (!b.is_terminal())
        {
            b.apply_random_action();
        }

        auto score = Board::evaluate_terminal(b);
        if (~b.side_to_move() != player)
            return 1.0 - score;

        return score;
    }

    void init_counters()
    {
        cnt_iters = 0;
        m_stopwatch.reset_start();
    }

    action_type best_action()
    {
        auto valid_actions = m_state.valid_actions();

        int action_nb = 0;
        action_type action{ };

        m_root_evals.clear();
        m_root_evals.resize(valid_actions.size());
        m_root_visits.clear();
        m_root_visits.resize(valid_actions.size());

        init_counters();

        while (computation_resources())
        {
            action = valid_actions[action_nb];
            m_root_evals[action_nb] += playout(action);

            action_nb = ++action_nb % m_root_evals.size();
            ++cnt_iters;
            ++m_root_visits[action_nb];
        }

        int best_ndx = 0;
        for (int i=0; i<m_root_evals.size(); ++i)
        {
            int val = m_root_evals[i];
            if (val * m_root_visits[best_ndx] > m_root_evals[best_ndx] * m_root_visits[i])
            {
                best_ndx = i;
            }
        }
        return valid_actions[best_ndx];
    }
    bool computation_resources() const
    {
        bool iters_ok = max_iters > 0 ? cnt_iters < max_iters : true;
        bool time_ok = max_time > 0 ? m_stopwatch() < max_time : true;
        return iters_ok && time_ok;
    }
    std::vector<std::pair<double, int>> root_moves_eval() const
    {
        auto va = m_state.valid_actions_data();
        std::vector<std::pair<double, int>> ret;
        for (auto i = 0; i<va.size(); ++i)
        {
            int n_visits = m_root_visits[i];
            double val = m_root_evals[i] / (n_visits + 1.0);
            ret.push_back(std::pair{ val, n_visits });
        }

        return ret;
    }
    std::chrono::milliseconds::rep time_elapsed() const
    {
        return m_stopwatch();
    }
    int get_iterations_cnt() const
    {
        return cnt_iters;
    }
    void apply_root_action(const action_type& action)
    {
        m_state.apply_action(action);
    }
    const Board& state() const
    {
        return m_state;
    }
    const Board& root_state() const
    {
        return m_root_state;
    }


private:
    Board m_root_state;
    Board m_state;
    std::vector<action_type> m_actions_done;
    int cnt_iters;
    utils::Stopwatch m_stopwatch;
    int max_iters;
    int max_time;
    std::vector<int> m_root_evals;
    std::vector<int> m_root_visits;
};


#endif
