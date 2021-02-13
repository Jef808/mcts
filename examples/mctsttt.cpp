// -*- c++ -*-
// examples/mctsttt.cpp

#include "mctsttt.hpp"

namespace ttt {

ttt::EvalFcn::EvalFcn()
    : token()
{
}
void ttt::EvalFcn::SetToken(Token _tok)
{
    token = _tok;
}
double ttt::EvalFcn::operator()(const ttt::State& _state, const ttt::Action& _action) const
{
    return _state.eval_terminal(token);
}
ttt::EvalFcn* ttt::EvalFcn::clone() const
{
    return new EvalFcn(*this);
}

ttt::EvalFcn::EvalFcn(const EvalFcn& other)
{
    token = other.token;
}
ttt::EvalFcn& ttt::EvalFcn::operator=(const EvalFcn& other)
{
    token = other.token;
    return *this;
}

ttt::EvalFcnHandler::EvalFcnHandler()
    : evalInstance(new EvalFcn())
{
}

ttt::EvalFcn* ttt::EvalFcnHandler::operator()()
{

    return evalInstance->clone();
}
void ttt::EvalFcnHandler::setToken(Token t)
{
    evalInstance->SetToken(t);
}

ttt::Agent::Agent(Token _tok, int _it, int _dpth, int _br)
    : _mcts(_it, _dpth, _br)
    , agent_token(_tok)
    , handler()
{
    handler.setToken(_tok);
}

Token ttt::Agent::get_token() const
{
    return agent_token;
}

ttt::Action ttt::Agent::choose_action(const ttt::State& state)
{
    return _mcts.get_best_action(state);
}

}
