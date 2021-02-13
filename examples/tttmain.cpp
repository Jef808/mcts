// -*- c++ -*-

#include "mctsttt.hpp"
#include "ttt.hpp"

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

using namespace mcts;

Action get_next_action(const State& state, ttt::Agent & agent)
{
    Token tok = state.get_next_player();
    auto action = Action();
    if (agent.get_token() == tok) {
        action = agent.choose_action(state);
    } else {
        action = get_human_action(state, tok);
    }
    return action;
}


int main()
{
    Token player_token = get_player_token();
    Token agent_token = (player_token == Token::X ? Token::O : Token::X);

    auto agent = ttt::Agent(agent_token, 5000, 100, sqrt(2));

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
