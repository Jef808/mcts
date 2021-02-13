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
    EvalFcn();
    void SetToken(Token);
    double operator()(const State&, const Action&) const;
    EvalFcn* clone() const;
    ~EvalFcn() { }
private:
    Token token;
    EvalFcn(const EvalFcn&);
    EvalFcn& operator=(const EvalFcn& other);
};

class EvalFcnHandler
{
public:
    EvalFcnHandler();
    EvalFcn* operator()();
    void setToken(Token);
private:
    EvalFcn* evalInstance;
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
    EvalFcnHandler handler;
    Token agent_token;

    mcts::Agent <
        ttt::State,
        ttt::Action,
        decltype(handler()),
        mcts::policies::RandomRollout> _mcts;
};


}


#endif // __MCTSTTT_H_
