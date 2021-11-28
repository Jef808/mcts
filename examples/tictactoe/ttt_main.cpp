//#include "mcts.h"
#include "utils/stopwatch.h"
#include "bitboard.h"
#include "tictactoe.h"
#include "agent_random.h"

#include <array>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ttt {

void view_bitboards(std::ostream& out)
{
    for (int i=0; i<9; ++i) {
        out << "Square " << i
            << ": " << std::bitset<32>(SquareBB[i])
            << "\n";
    }

    out << '\n';

    for (int i=0; i<8; ++i) {
        out << "Line   " << i
            << ": " << std::bitset<32>(LineBB[i])
            << "\n";
    }

    out << '\n';

    for (int i=0; i<3; ++i) {
        out << "Token  " << i
            << ": " << std::bitset<32>(TokenBB[i])
            << "\n";
    }

    out << '\n';

    for (int i=0; i<2; ++i) {
        out << "Player " << i << '\n';
        for (int j=0; j<9; ++j)
            out << ": " << std::bitset<32>(MoveBB[i][j])
            << "\n";
    }

    out << std::endl;
}

}  //namespace ttt


template<typename ActionT>
inline bool is_valid(const ActionT& action)
{
    return action > -1 && action < 10;
}

template<typename Move>
inline Move input_human_move()
{
    std::cout << "\nChoose your next move...\n" << std::endl;
    int move_buf{};
    while (1)
    {
        std::cin >> move_buf; std::cin.ignore();

        if (is_valid(move_buf-1))
            break;

        std::cout << "Invalid move: " << move_buf << std::endl;
    }
    return Move(move_buf-1);
}

template<typename S, typename Move>
inline void play_human_move(S& s)
{
    Move move{};
    bool done = false;

    while (!done)
    {
        move = input_human_move<Move>();
        done = !s.is_trivial(move);
    }

    s.apply_action(move);
}

template<typename S>
inline void display_results(const S& s)
{
    auto [my_score, opp_score] = s.final_score();

    auto winner_str = my_score > opp_score ? "I win!\n" :
                           my_score == opp_score ? "Draw!\n" :
                                                       "I lose!\n";
    std::cout << s
              << winner_str
              << "My score: " << my_score
              << "... Opponent score: " << opp_score << std::endl;
}

using namespace ttt;


//using Agent = Agent_random<State>;
//using Mcts = mcts::Mcts<State, State::action_type>;

int main()
{
    int n_iters = 3000;


    Player p_mcts = Player::O;
    //Mcts mcts{_s};
    //mcts.set_max_iterations(n_iters);
    //mcts.set_max_time(0);

    //Agent agent{_s};
    //agent.set_max_iterations(10);
    //agent.set_max_time(0);

    State::action_type action{};
    std::vector<Token> results;


    int n_games = 5000000;
    results.reserve(n_games);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    for (int i=0; i<n_games; ++i) {
        State _s{};
        while (!_s.is_terminal()) {
            action = _s.apply_random_action();
        }
        results.push_back(_s.is_draw() ? Token::None : token_of(_s.winner()));
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time = std::chrono::duration_cast<std::chrono::duration<double>>(
            end - start
    );

    int n_x_wins = std::count(results.begin(), results.end(), Token::X);
    std::cout << std::setprecision(4)
        << "Avg time per game with bitboards: "
        << time.count() / n_games
        << "\nX wins "
        << n_x_wins
        << " times."
        << std::endl;

    results.clear();
    start = std::chrono::steady_clock::now();

    std::cerr << "State_normal games:" << std::endl;

    for (int i=0; i<n_games; ++i) {
        State_normal _s{};
        while (!_s.is_terminal()) {
            action = _s.apply_random_action();
        }

        results.push_back(_s.is_draw() ? Token::None : token_of(_s.winner()));
    }

    end = std::chrono::steady_clock::now();
    time = std::chrono::duration_cast<std::chrono::duration<double>>(
            end - start
    );

    n_x_wins = std::count(results.begin(), results.end(), Token::X);
    std::cout << "Avg time per game without bitboards: "
        << time.count() / n_games
        << "\nX wins "
        << n_x_wins
        << " times."
        << std::endl;

    return EXIT_SUCCESS;

    // while (!_s.is_terminal())
    // {
    //     //std::cout << _s << std::endl;

    //     if (_s.side_to_move() == p_mcts)
    //     {
    //         sw.reset_start();
    //         action = mcts.best_action();
    //         auto time = sw.get();
    //         std::cerr << "\nMCTS chooses  " << action << std::endl;
    //         // std::cerr << "\nTime taken: "
    //         //           << std::setprecision(4) << time.count() << std::endl;
    //         //action = input_human_move<Move>();
    //     }
    //     else {
    //         action = agent.best_action();
    //         std::cerr << "\nAgent chooses  " << action << std::endl;
    //     }

    //     mcts.apply_root_action(action);
    //     agent.apply_root_action(action);
    //     _s.apply_action(action);
    // }

    // std::cout << "\n*********\nTERMINAL\n"
    //           << _s;

    // if (_s.is_draw())
    //     svtd::cout << "\nGame is drawn!";
    // else
    //     std::cout << "\n" << token_of(_s.winner()) << " wins!";

    // std::cout << std::endl;


    return 0;
}
