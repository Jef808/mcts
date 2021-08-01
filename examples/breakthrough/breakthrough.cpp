#include "bitboard.h"
#include "board.h"
#include "types.h"

#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "mcts.h"
#include "policies.h"
#include "minmax.h"
#include "utils/agent_random.h"

void progress_bar(int cnt, int n_games)
{
    int progress = int(80.0 * cnt / n_games);
    std::cout << '|'
               << std::string(progress, '@')
              << std::string(80 - progress, ' ')
              << '|'
              << std::endl;
}



using namespace BT;

template<size_t N>
struct TimeCutoff_UCB_Func
{
    auto operator()(double expl_cst, unsigned int n_parent_visits)
    {
        return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge)
            {
            double ret = (n_parent_visits < N
                          ? expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0))
                          : 0.00001);
            return ret + edge.total_val / (edge.n_visits + 1.0);
        };
    }
};


template <typename Agent>
struct configure_agent {
    using Backprop = typename Agent::BackpropagationStrategy;
    using NPlayers = typename Agent::NPlayers;

    int n_iterations = 500;
    double max_time = 0;
    double expl_cst = 1.0;

    void operator()(Agent& mcts)
    {
        mcts.set_max_iterations(n_iterations);
        mcts.set_max_time(max_time);
        mcts.set_exploration_constant(expl_cst);
        mcts.set_backpropagation_strategy(
            Backprop::avg_best_value);
        mcts.set_n_players(
            NPlayers::Two);
    }
};

template <>
struct configure_agent<Agent_random<Position>> {
    int n_iterations = 2000;
    int max_time = 0;

    void operator()(Agent_random<Position>& agent)
    {
        agent.set_max_iterations(n_iterations);
        agent.set_max_time(max_time);
    }
};

int main(int argc, char* argv[])
{
    using namespace mcts;

    using action_type = Position::action_type;
    using MctsAgent = Mcts<Position,
        action_type,
        TimeCutoff_UCB_Func<30>,
        policies::Default_Playout_Func<Position, action_type>,
        128>;
    using Minmax_agent = minmax::Agent;

    Position pos_bk {};


    Color p_minmax = Color::White;
    //Color p_mcts = Color::White;
    Color p_rand = Color::Black;
    //Color p_rand_opp = Color::Black;

    int n_games = 1;
    const int n_iters = 3000;
    std::vector<std::chrono::milliseconds> times;
    std::vector<bool> results;
    utils::Stopwatch sw{};
    auto time = std::chrono::milliseconds::zero();

    for (int i = 0; i < n_games; ++i) {

        progress_bar(i, n_games);

        Position pos = pos_bk;

        Move move_buf;
        //MctsAgent mcts{pos};

        Agent_random<Position> rand { pos };
        Minmax_agent minmax;

        //configure_agent<MctsAgent> conf1{};
        configure_agent<Agent_random<Position>> conf2 {};
        //conf1.n_iterations = n_iters;
        conf2.n_iterations = n_iters;
        //conf1.expl_cst = 0.7;
        //conf1(mcts);
        conf2(rand);

        int depth = 0;

        while (!pos.is_terminal())
        {
            if (pos.side_to_move() == p_minmax) {
                sw.reset_start();
                //move_buf = mcts.best_action();
                move_buf = minmax.best_action(pos);
                time += sw.get();
                //move_buf = rand.best_action();
            } else {
                move_buf = rand.best_action();
                //std::this_thread::sleep_for(std::chrono::milliseconds{400});
            }

            // std::cout << "\n\n*******************\n\n"
            //           << pos
            //           << "\n\nPlayer: "
            //           << (pos.side_to_move() == p_rand ? "RAND" : "RAND0")
            //           << "\nAction: "
            //           << move_buf << '\n'
            //           << "\n****************\n"
            //           << std::endl;

            // auto va = pos.valid_actions();
            // std::cout << "\nValid actions were:\n";

            // for (auto a : va) {
            //     std::cout << a << '\n';
            // }
            // std::cout << std::endl;

            //mcts.apply_root_action(move_buf);
            //mcts.apply_root_action(move_buf);
            rand.apply_root_action(move_buf);
            pos.apply_action(move_buf);
            ++depth;
        }

        bool res = (pos.winner(pos) == p_minmax ? 1 : 0);
        results.push_back(res);

        // std::cout << "\n\n*******************\n"
        //       << "TERMINAL:\n\n"
        //       << pos
        //       << "Winner: "
        //       << (Position::winner(pos) == mcts_player ? "MCTS" : "RAND")
        //       << "\n****************\n"
        //       << "Time taken with " << n_iters
        //       << " iterations: "
        //       << std::setprecision(4)
        //       << time.count()
        //       << " ms."
        //       << "\nDepth reached: "
        //       << depth
        //       << "Number of nodes created: "
        //       << mcts.get_n_nodes()
        //       << ", each of size "
        //       << sizeof(MctsAgent::node_type)
        //       << std::endl;

        // std::cout << "Input any character to continue..." << std::endl;
        // char _block{ };
        // std::cin >> _block; std::cin.ignore();
    }

    auto total_wins = std::count_if(results.begin(), results.end(), [](auto r) {
        return r;
    });
    std::cout << "\n\n*************\n"
              << "After "
              << n_games
              << " with "
              << n_iters
              << " iterations per game turn...\n"
              << " Average time taken: "
              << std::setprecision(6)
              << time.count() / double(n_games)
              << " ms."
              << "\nNumber of wins: "
              << total_wins
              << "\nFor a winning percentage of "
              << 100.0 * total_wins / n_games << " %."
              << std::endl;

    return 0;
}
