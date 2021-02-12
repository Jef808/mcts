// -*- c++ -*-
// examples/mctsttt.cpp

#include "mctsttt.hpp"

namespace ttt {

    /** Binds a token in an evaluation function. */
    double ttt::EvalFcn::operator()(const ttt::State& state, const ttt::Action& action) const
    {
        return [=]() {
            return std::invoke(&ttt::State::eval_terminal, state, tok);
        }();
    }

    void ttt::EvalFcn::setToken(Token _tok)
    {
        tok = _tok;
    }


    ttt::Agent::Agent(Token _tok, int _it, int _dpth, int _br)
        : _mcts(_it, _dpth, _br)
        , agent_token(_tok)
    {
        _mcts.setEvalPolicy(ttt::EvalFcn(agent_token));
    }

    Token ttt::Agent::get_token() const
    {
        return agent_token;
    }

    Action ttt::Agent::choose_action(const State& state)
    {
        return _mcts.get_best_action(state);
    }

}
