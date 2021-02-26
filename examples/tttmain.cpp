// -*- c++ -*-

#include "ttt.hpp"
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <variant>
//#include "../../tikztreeold/dfs.hpp"
#include "../mcts.hpp"

//#include "mctsttt.hpp"

using namespace ttt;

bool is_valid(const State& state, const Action& action)
{
    auto valid_actions = state.get_valid_actions();
    return std::find(cbegin(valid_actions), cend(valid_actions), action) != cend(valid_actions);
}

Action get_human_action(const State& state, Token human_token)
{
    auto action = Action(-1, human_token);
    int buf;
    while (!is_valid(state, action)) {
        std::cout << "Choose your move..." << std::endl;
        std::cin >> buf;
        std::cin.ignore();
        action.ndx = buf;
    }
    return action;
}

Action get_random_action(const State& state, Token tok)
{
    auto valid_actions = state.get_valid_actions();
    int ndx = rand() % valid_actions.size();

    return Action(ndx, tok);
}

Token get_player_token()
{
    std::string ans = "";
    while (ans != "X" && ans != "O") {
        std::cout << "What side do you want to play? (X or O)" << std::endl;
        std::cin >> ans;
        std::cin.ignore();
    }
    return ans == "X" ? Token::X : Token::O;
}

Token other_player(Token t)
{
    return t == Token::X ? Token::O : Token::X;
}

template <Token t>
struct EvalFunctor {

    EvalFunctor() { }

    double operator()(const State& _state, const Action& _action)
    {
        auto winner = _state.get_winner();

        if (winner == t) {
            return 1.0;
        }
        if (winner == Token::EMPTY) {
            return 0.5;
        }
        return 0.0;

        // switch(_state.get_winner())
        // {
        // case Token::EMPTY:
        //     return 0.5;
        // case t:
        //     return 1.0;
        // default:
        //     return 0.0;
        // }
    }
};

template<Token t>
struct UCT1_WithTokens {

    using node_t = mcts::Node<State, Action>;

    UCT1_WithTokens(double _b = sqrt(2))
        : BR_CST(_b)
        , agent_token(t)
    {
    }

    node_t* operator()(const node_t& node)
    {
        if (node.get_parent_action().token == agent_token)
        {
            return max_node(node);
        }
        else
        {
            return min_node(node);
        }
    }

private:
    double BR_CST;
    Token agent_token;

    double uct1_val(const node_t& child)
    {
        return child.get_avg_value() + BR_CST * sqrt(log(child.get_parent()->get_n_visits()) / child.get_n_visits());
    }

    node_t* max_node(const node_t& node)
    {
        return std::max_element(begin(node.get_children()), end(node.get_children()),
            [&, c = BR_CST](const auto& a, const auto& b) {
                // if (a->get_n_visits() < 5)
                // {
                //     return false;
                // }
                return uct1_val(*a) < uct1_val(*b);
            })
            ->get();
    }
    node_t* min_node(const node_t& node)
    {
        return std::min_element(begin(node.get_children()), end(node.get_children()),
            [&, c = BR_CST](const auto& a, const auto& b) {
                // if (a->get_n_visits() < 5)
                // {
                //     return true;
                // }
                return uct1_val(*a) < uct1_val(*b);
            })
            ->get();
    }
};

template <Token t>
class Agent { // : mcts::Agent<State, Action, EvalFunctor<t>> {
    using agent_t = mcts::Agent<State, Action, EvalFunctor<t>, UCT1_WithTokens<t>>;
    //using node_t = mcts::Node<State, Action>;
public:
    Agent(int _iter = 10000, int _depth = 50, int _br = 1)
        : agent_(_iter, _depth, _br)
    {
    }
    Action action(const State& state) const
    {
        // Run n_iterations steps of the MCTS algorithm.
        return agent_.get_best_action(state);
    }

private:
    agent_t agent_;
};

class AgentV {
    using Agent_T = std::variant<Agent<Token::X>, Agent<Token::O>>;
    Agent_T agent_;
    Token token_;

public:
        AgentV(Token _token, int _iter, int _depth, int _br)
        : token_(_token)
        , agent_(init_agent(_token, _iter, _depth, _br))
    {
    }
    Action get_action(const State& state)
    {
        Action action;
        if (token_ == Token::X) {
            action = std::get<Agent<Token::X>>(agent_).action(state);
        } else {
            action = std::get<Agent<Token::O>>(agent_).action(state);
        }
        return action;
    }

private:
    Agent_T init_agent(Token _token, int _iter, int _depth, int _br)
    {
        Agent_T agent;

        if (_token == Token::X) {
            agent = Agent<Token::X>(_iter, _depth, _br);
        } else {
            agent = Agent<Token::O>(_iter, _depth, _br);
        }
        return agent;
    }
};

int main()
{
    auto state = State();
    auto action = Action();
    auto player_token = get_player_token();
    auto agent_token = other_player(player_token);

    AgentV agent(agent_token, 30000, 50, 1); // = Agent<Token::X>();

    auto nex_player = Token::X;

    while (!state.is_terminal()) {
        action = nex_player == player_token ? get_human_action(state, player_token) : agent.get_action(state);
        state.apply_action(action);
        std::cout << state << std::endl;
        nex_player = other_player(nex_player);
    }

    auto winner = state.get_winner();
    if (winner == Token::EMPTY) {
        std::cout << "Game is a draw!" << std::endl;
    } else {
        std::cout << winner << " wins!" << std::endl;
    }

    return 0;
}
