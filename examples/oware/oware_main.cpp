#include "mcts.h"
#include "policies.h"
#include "oware.h"
#include "oware_mcts.h"

#include <chrono>
#include <iostream>

#include "utils/agent_random.h"
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
    return Result { -1 + (terminal_eval > 0.49999) + (terminal_eval > 0.5000001) };
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



template<typename Agent>
struct configure_agent {
    using Backprop = typename Agent::BackpropagationStrategy;
    using NPlayers = typename Agent::NPlayers;

    int n_iterations = 2000;
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
struct configure_agent<Agent_random<Board>> {
    int n_iterations = 500;
    int max_time = 0;

    void operator()(Agent_random<Board>& agent)
    {
        agent.set_max_iterations(n_iterations);
        agent.set_max_time(max_time);
    }
};

template<typename Agent>
std::ostream& operator<<(std::ostream& _out, const configure_agent<Agent>& conf)
{
    return _out << "\nn_iterations: " << conf.n_iterations
                << "\nmax_time: " << conf.max_time
                << "\nexpl_cst: " << conf.expl_cst
                << "\nBackpropagation by AVG_BEST_VALUE"
                << std::endl;
}

template<>
std::ostream& operator<<(std::ostream& _out, const configure_agent<Agent_random<Board>>& conf)
{
    return _out << "\nn_iterations: " << conf.n_iterations
                << "\nmax_time: " << conf.max_time
                << std::endl;
}

template<typename Agent1, typename Agent2>
void make_play(int n_games,
               configure_agent<Agent1>& conf1,
               configure_agent<Agent2>& conf2)
{
    using namespace oware;

    using action_type = Board::action_type;
    using reward_type = Board::reward_type;

    bool agent1_player = 1;
    bool agent2_player = 0;
    std::vector<Result> agent1_results;
    utils::Stopwatch sw1, sw2;
    std::chrono::milliseconds time1, time2;
    time1 = time2 = std::chrono::milliseconds::zero();

    for (int game = 0; game < 2 * n_games; ++game) {
        progress_bar(game, 2 * n_games);

        // On odd game numbers, agent2 goes first and vice-versa.
        Board b { };

        Agent1 agent1(b);
        conf1(agent1);

        Agent2 agent2(b);
        conf2(agent2);

        action_type action_buf{};

        while (!b.is_terminal())
        {
            if (b.side_to_move() == agent1_player)
            {
                sw1.reset_start();
                action_buf = agent1.best_action();
                time1 += sw1.get();
            }
            else
            {
                sw2.reset_start();
                action_buf = agent2.best_action();
                time2 += sw2.get();
            }

#ifdef DEBUG_OWARE
            std::cerr << b
                      << "\nPlayer: " << b.side_to_move()
                      << "\nChosen action: " << action_buf
                      << std::endl;
#endif
            agent1.apply_root_action(action_buf);
            agent2.apply_root_action(action_buf);
            b.apply_action(action_buf);
        }

        // Figure out the result from the point of view of the mcts agent
        reward_type terminal_eval = Board::evaluate_terminal(b);

        Result last_p_result = last_player_result(terminal_eval);

        Result agent1_result = (
            agent1_player == last_player(b) ?
            last_p_result :
            negate(last_p_result)
         );

        // Store the result
        agent1_results.push_back(agent1_result);
    }

    // Counters for the results
    int wins = 0,
        losses = 0,
        draws = 0;

    for (auto res : agent1_results) {
        switch (res) {
            case Result::Win: {
                ++wins;
                break;
            }
            case Result::Loss: {
                ++losses;
                break;
            }
            case Result::Draw: {
                ++draws;
                break;
            }
        }
    }

    std::cerr << "Configurations: "
              << "\nAgent1:\n"
              << conf1
              << "\nAgent2:\n"
              << conf2;

    std::cerr << "\n\nResults after " << 2 * n_games << " games..."
              << "\nAgent1 Wins: " << wins
              << "\nAgent2 Wins: " << losses
              << "\nDraws: " << draws
              << "\nAgent1 winning percentage: "
              << std::setprecision(2)
              << 100.0 * wins / (2.0 * n_games)
              << " %."
              << "\nAverage time per game taken for agent1: "
              << std::setprecision(4)
              << time1.count() / (2.0 * n_games) << "ms"
              << "\nAverage time per game taken for agent2: "
              << std::setprecision(4)
              << time2.count() / (2.0 * n_games) << "ms"
              << std::endl;
}


int main()
{
    using namespace oware;

    constexpr size_t N = 30;

    using action_type = Board::action_type;
    using DefaultMctsAgent = mcts::Mcts<Board,
        action_type,
        policies::Default_UCB_Func,
        policies::Default_Playout_Func<Board, action_type>,
        128>;
    using DefaultMctsAgent_TimeCutoff = mcts::Mcts<Board,
        action_type,
        TimeCutoff_UCB_Func<N>,
        policies::Default_Playout_Func<Board, action_type>,
        128>;
    using OwareMctsAgent = mcts::Mcts<Board,
        action_type,
        TimeCutoff_UCB_Func<N>,
        oware::Oware_Playout_Func,
        128>;
    using OwareMctsAgent_Weighted = mcts::Mcts<Board,
        action_type,
        TimeCutoff_UCB_Func<30>,
        oware::Oware_Weighted_Playout_Func,
        128>;
    using OwareMctsAgent_Weighted50 = mcts::Mcts<Board,
        action_type,
        TimeCutoff_UCB_Func<50>,
        oware::Oware_Weighted_Playout_Func,
        128>;

    std::vector<std::string> agents {
        "RandomAgent",                                               // 0
        "DefaultMctsAgent",                                          // 1
        "DefaultMctsAgent_TimeCutoff<" + std::to_string(N) + ">",    // 2
        "OwareMctsAgent",                                            // 3
        "OwareMctsAgent_Weighted",                                   // 4
        "OwareMctsAgent_Weighted50"                                  // 5
        };

    using Agent0 = Agent_random<Board>;
    using Agent1 = DefaultMctsAgent;
    using Agent3 = OwareMctsAgent;
    using Agent4 = OwareMctsAgent_Weighted;
    using Agent5 = OwareMctsAgent_Weighted50;


    int n_games = 10;
    configure_agent<Agent1> conf1{};
    configure_agent<Agent0> conf2{};

    conf1.n_iterations = 5000;
    conf2.n_iterations = 12;
    conf1.max_time = conf2.max_time = 0;

    std::cout << "\n********** "
              << agents[1] << " vs " << agents[0]
              << " **********\n\n"
              << std::endl;

    make_play<Agent1, Agent0>(n_games, conf1, conf2);
    //make_play<OwareMctsAgent1, DefaultMctsAgent>(n_games);

    return EXIT_SUCCESS;
}
