#include "ttt.hpp"
#include "mcts.hpp"
#include <array>
#include <iostream>
#include <string>

namespace ttt {

size_t State::n_empty_cells() const
{
    return std::count_if(begin(grid), end(grid), [](const Token t) { return t == Token::EMPTY; });
}

Token State::get_next_player() const
{
    return n_empty_cells() & 1 ? Token::X : Token::O;
}

std::vector<Action> State::get_valid_actions() const
{
    std::vector<Action> ret;
    std::for_each(cbegin(grid), cend(grid), [ndx = 0, nex_player = get_next_player(), &ret](const auto& t) mutable {
        if (t == Token::EMPTY) {
            ret.emplace_back(ndx, nex_player);
        }
        ++ndx;
    });
    return ret;
}

State& State::apply_action(const Action& action)
{
    grid[action.ndx] = action.token;
    return *this;
}

bool State::is_terminal() const
{
    return n_empty_cells() == 0 || get_winner() != Token::EMPTY;
}

bool State::is_draw() const
{
    return n_empty_cells() == 0 && get_winner() == Token::EMPTY;
}

State::token_line_t State::get_tokens(const line_t& line) const
{
    static token_line_t ret { Token::EMPTY };
    std::copy_if(cbegin(grid), cend(grid), begin(ret), [it = cbegin(line), grid_ndx = -1](const auto& t) mutable {
        if (*it == ++grid_ndx) {
            ++it;
            return true;
        }
        return false;
    });
    return ret;
}

auto three_in_row = [](const State::token_line_t& line, Token tok) {
    return std::all_of(cbegin(line), cend(line), [&tok](const auto& t) { return t == tok; });
};

Token State::get_winner() const
{
    for (const auto& win_line : WIN_COMBIN) {
        auto token_line = get_tokens(win_line);
        for (auto& t : { Token::X, Token::O }) {
            if (three_in_row(token_line, t)) {
                return t;
            }
        }
    }
    return Token::EMPTY;
}

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

template <class EvalFcn>
using mcts_agent_t = mcts::Agent<State, Action, EvalFcn, mcts::policies::RandomRollout>;
template <class EvalFcn>
class TTTAgent : public mcts_agent_t<EvalFcn> {
public:
    const Token agent_token;
    TTTAgent(const Token& tok, int _max_iter = 1000, int _max_roll = 100, double _br_fact = sqrt(2))
        : agent_token(tok)
        , mcts_agent_t<EvalFcn>()
    {}
};

template <class Agent_T>
Action get_next_action(const State& state, const Agent_T& agent)
{
    Token tok = state.get_next_player();
    auto action = Action();
    if (agent.agent_token == tok) {
        action = agent.get_best_action(state);
    } else {
        action = get_human_action(state, tok);
    }
    return action;
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

} // namespace ttt
using namespace ttt;

int main()
{
    Token player_token = get_player_token();
    static Token agent_token = (player_token == Token::X ? Token::O : Token::X);

    struct EvalFcn {
        EvalFcn() { }
        auto operator()(const State& state, const Action& action)
        {
            auto winner = state.get_winner();
            if (winner == agent_token) {
                return 1;
            } else if (winner != Token::EMPTY) {
                return -1;
            }
        return 0;
        }
    };

    auto agent = TTTAgent<EvalFcn>(agent_token, 5000, 100, sqrt(2));

    auto state = State();
    auto action = Action();

    while (!state.is_terminal()) {

        action = get_next_action(state, agent);
        state.apply_action(action);
        std::cout << state << std::endl;

        if (state.is_terminal()) {
            break;
        }
        action = get_next_action(state, agent);
        state.apply_action(action);
        std::cout << state << std::endl;
    }

    Token winner = state.get_winner();
    if (winner == Token::EMPTY) {
        std::cout << "Game is a draw!" << std::endl;
    } else {
        std::cout << winner << " wins!" << std::endl;
    }

    return 0;
}
