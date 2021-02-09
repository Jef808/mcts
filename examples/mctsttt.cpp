#include "mctsttt.hpp"

namespace ttt {


    ttt::Agent::Agent(Token tok, int _max_it, int _max_rol, double br)
        : Agent::agent_mcts { _max_it, _max_rol, br }
        , agent_token(tok)
        , eval(tok)
    {
        eval = EvalFcn(tok);
    }

    double ttt::Agent::evaluate(const State& state, const Action& parent_action) const
    {
        return eval(state, parent_action);
    }



}
