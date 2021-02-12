// -*- c++ -*-
// examples/mctsttt.hpp


#ifndef __MCTSTTT_H_
#define __MCTSTTT_H_

#include <type_traits>
#include "ttt.hpp"
#include "../mcts.hpp"

namespace ttt {

class EvalFcn
{
public:
    EvalFcn(Token _tok = Token::EMPTY)
        : tok()
    {}
    double operator()(const ttt::State& state, const ttt::Action& action) const;
    void setToken(Token _tok);

private:
    ttt::Token tok;
};

class Agent
{

public:
    Agent(Token token,
        int _max_iter = 1000,
        int _max_depth = 100,
        int _branch = 2);

    Token get_token() const;
    Action choose_action(const State& state);

private:
    ttt::Token agent_token;
    mcts::Agent<
        State,
        Action,
        EvalFcn,
        ::mcts::policies::RandomRollout> _mcts;
};


}




#endif // __MCTSTTT_H_
