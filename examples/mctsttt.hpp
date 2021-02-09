#ifndef __MCTSTTT_H_
#define __MCTSTTT_H_

#include "../../tikztreeold/dfs.hpp"
#include "ttt.hpp"
#include "../mcts.hpp"


namespace ttt {

    /** Extends the interface class for the particularities of ttt */
    class Agent : public mcts::Agent<ttt::State, ttt::Action, ttt::EvalFcn, mcts::policies::RandomRollout>
    {
        using agent_mcts = mcts::Agent<ttt::State, ttt::Action, ttt::EvalFcn, mcts::policies::RandomRollout>;
        using node_t = mcts::Node<ttt::State, ttt::Action>;
        Token agent_token;
        EvalFcn eval;

    public:

        Agent(Token tok, int _max_iterations = 1000, int _max_rollout_depth = 100, double br_fact = sqrt(2));

        double evaluate(const State& state, const Action& action) const;
        Token get_token() const;
    };

    class tikz_ttt : public tikz::TikzTree
    {
        using Inode = mcts::Node<ttt::State, ttt::Action>;
    };

}

#endif // __MCTSTTT_H_
