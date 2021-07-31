#include "mcts.h"
#include "policies.h"
#include "oware.h"
#include "oware_mcts.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

#include "agent_random.h"
#include "utils/stopwatch.h"
#include "utils/rand.h"


enum class Result {
Win = 1,
Draw = 0,
Loss = -1
};

inline Result negate(Result res)
{
    Result ret = Result::Draw;
    switch (res) {
        case Result::Win: {
            ret = Result::Loss;
            break;
        }
        case Result::Loss: {
            ret = Result::Win;
        }
        default:
            break;
    }
    return ret;
}

inline bool last_player(const Board& b)
{
    return !b.side_to_move();
}

inline Result last_player_result(Board::reward_type terminal_eval)
{
    return Result { -1 + (terminal_eval > 0.2) + (terminal_eval > 0.7) };
}

inline Result mcts_player_result(Board::reward_type terminal_eval,
                                 bool last_player,
                                 bool mcts_player)
{
    // The result of the `last_player`: the player who played the last
    // move of the game. (The `terminal_eval` score is from the point
    // of view of that player).
    Result last_player_result { -1 + (terminal_eval > 0.2) + (terminal_eval > 0.7) };

    // Basic logic but a bit of gymnastic
    Result mcts_res = (last_player_result == Result::Draw) ? Result::Draw : (mcts_player == last_player) ? last_player_result
        : negate(last_player_result);

    return mcts_res;
}

void progress_bar(int cnt, int n_games)
{
    int progress = int(80.0 * cnt / n_games);
    std::cout << '|'
              << std::string(progress, '@')
              << std::string(80 - progress, ' ')
              << '|'
              << std::endl;
}


using namespace oware;

template<typename Agent>
struct configure_mcts;

template<typename Agent>
std::ostream& operator<<(std::ostream&, const configure_mcts<Agent>&);

template <typename Agent>
struct configure_agent {
    int n_iterations = 200;
    int max_time = 0;

    void operator()(Agent& agent)
    {
        agent.set_max_iterations(n_iterations);
        agent.set_max_time(max_time);
    }
};

template <typename Agent>
struct configure_mcts : configure_agent<Agent> {
    using base = configure_agent<Agent>;
    using Backprop = typename Agent::BackpropagationStrategy;
    using NPlayers = typename Agent::NPlayers;

    int n_iterations = 4000;
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

    friend std::ostream& operator<<<>(std::ostream& _out, const configure_mcts<Agent>& conf);
};

using MctsAgent = mcts::Mcts<Board,
                             Board::action_type,
                             TimeCutoff_UCB_Func<30>,
                             oware::Oware_Playout_Func,
                             128>;
template<>
std::ostream& operator<<(std::ostream& _out, const configure_mcts<MctsAgent>& conf)
{
    return _out << "\n***********"
                << "\n\nWith: "
                << "\nn_iterations: " << conf.n_iterations
                << "\nmax_time: " << conf.max_time
                << "\nexpl_cst: " << conf.expl_cst
                << "\nBackpropagation by AVG_BEST_VALUE"
                << std::endl;
}

struct Basic_params
{
    int time;
    int iterations;
    double explo_constant;
};


int main()
{
    using action_type = Board::action_type;
    using reward_type = Board::reward_type;
    // using MctsAgent = mcts::Mcts<Board,
    //     action_type,
    //     TimeCutoff_UCB_Func<30>,
    //     oware::Oware_Playout_Func,
    //     128>;

    //std::fstream _out("logs/timebench.log", std::ios::app);


    // if (!_out)
    // {
    //     std::cerr << "FAILED TO OPEN LOG FILE..."
    //               << std::endl;

    //     return EXIT_FAILURE;
    // }

    //configure_mcts<MctsAgent> mcts_conf{ };

    bool rand0_player = 1;
    //bool mcts_player = 1;
    bool rand_player = 0;

    Board bk { 1 };

    utils::Stopwatch sw;

    constexpr static int n_games = 10;

    configure_agent<Agent_random<Board>> rand_conf{ };
    rand_conf.n_iterations = 5000;

    for (int i=0; i<5; ++i)
    {
        progress_bar(i, 5);

        std::vector<std::pair<Result, int>> results;
        std::vector<utils::Stopwatch::Discrete_duration> times;
        std::vector<size_t> n_nodes;

        for (int game = 0; game < n_games; ++game) {

            Board b = bk;

            // MctsAgent mcts(b, TimeCutoff_UCB_Func<30> {});
            // mcts_conf(mcts);

            Agent_random<Board> rand0 { b };

            Agent_random<Board> rand { b };
            rand_conf(rand); rand_conf(rand0);

            sw.reset_start();
            action_type action_buf{};

            while (!b.is_terminal()) {

                if (b.side_to_move() == rand0_player)
                {
                    action_buf = rand0.best_action();
                }
                else
                {
                    action_buf = rand.best_action();
                }
                // action_type action = (b.side_to_move() == mcts_player ?
                //                       mcts.best_action() :
                //                       rand.best_action());

                //mcts.apply_root_action(action);
                rand0.apply_root_action(action_buf);
                rand.apply_root_action(action_buf);
            }

            auto time = sw();
            times.push_back(time);
            //n_nodes.push_back(mcts.get_n_nodes());

            // Figure out the result from the point of view of the mcts agent
            // reward_type terminal_eval = Board::evaluate_terminal(b);
            // bool last_p = last_player(b);
            // Result mcts_res = mcts_player_result(terminal_eval, last_p, mcts_player);
            // auto [s1, s2] = b.final_score();
            // double mcts_score = !b.side_to_move() == mcts_player ? s1 : s2;

            // Store the result
            //results.push_back(std::make_pair(mcts_res, mcts_score));
        }

        // // Counters for the results
        // int wins = 0,
        //     losses = 0,
        //     draws = 0;

        // for (auto res : results) {
        //     switch (res.first) {
        //     case Result::Win: {
        //         ++wins;
        //         break;
        //     }
        //     case Result::Loss: {
        //         ++losses;
        //         break;
        //     }
        //     case Result::Draw: {
        //         ++draws;
        //         break;
        //     }
        //     }
        // }

        std::time_t time = std::time(nullptr);

        std::cout << "\n**********************\n\n\n"
             << std::ctime(&time)
             //<< rand_conf
             << rand_conf.n_iterations << " iterations"
             << std::endl;

        // double total_score = std::accumulate(
        //     results.begin(),
        //     results.end(), 0.0, [](double m, auto s) -> double
        //     {
        //         return m + s.second;
        //     });

        //double avg_score_ratio = total_score / (48.0 * results.size());

        int cnt = 0;
        for (auto r : results)
        {
            std::cout << "\nGame " << cnt + 1
                << ":\n"
                //<< (r.first == Result::Win ? "WON" :
                //    r.first == Result::Draw ? "DRAW" :
                //    "LOST")
                //<< std::setprecision(2)
                //<< "\nWith scores " << r.second
                //<< " to " << 48 - r.second
                //<< "\nNumber of nodes in tree: "
                //<< n_nodes[cnt]
                << "\nTIME TAKEN: "
                << times[cnt] << "ms"
                << std::endl;
            ++cnt;
        }

        // std::cout << "\nAVG SCORE RATIO "
        //     << avg_score_ratio
            std::cout << "\n******************"
                 << std::endl;
    }
}
