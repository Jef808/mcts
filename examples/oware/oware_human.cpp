#include "oware.h"

#include <chrono>
#include <string>
#include <thread>
#include <iostream>

inline bool is_valid(int action)
{
    return action > -1 && action < 6;
}

inline int input_human_move()
{
    std::cout << "\nChoose your next move...\n" << std::endl;
    int move = 0;
    while (1)
    {
        std::cin >> move; std::cin.ignore();

        if (is_valid(move-1))
            break;

        std::cout << "Invalid move: " << move << std::endl;
    }
    return move-1;
}

inline void display_results(const Board& b)
{
    auto [my_score, opp_score] = b.final_score();

    auto winner_str = my_score > opp_score ? "I win!\n" :
                           my_score == opp_score ? "Draw!\n" :
                                                       "I lose!\n";
    std::cout << b
              << winner_str
              << "My score: " << my_score
              << "... Opponent score: " << opp_score << std::endl;
}

inline void play_human_move(Board& b)
{
    std::cout << b;
    int move = 0;
    bool done = false;

    while (!done)
    {
        move = input_human_move();
        done = !b.is_trivial(move);
    }

    b.apply_action(move);
}


struct Agent_random
{
    using action_type = Board::action_type;

    Agent_random() = default;

    action_type operator()(Board& b)
    {
        return b.apply_random_action();
    }
};

Agent_random s_agent { };

inline void play_agent_move(Board& b)
{
    std::cout << b;
    Board::action_type action = s_agent(b);
    std::cout << "\nOppponent chooses move " << action + 1 << '\n';
}

int main()
{
    using namespace std::literals::chrono_literals;

    Agent_random agent{};

    const bool human_player = 1;
    bool human_plays_first = 1;
    Board b(human_plays_first);
    int move { };

    while (!b.is_terminal())
    {
        if (b.side_to_move() == human_player)
        {
            play_human_move(b);
        }
        else
        {
            play_agent_move(b);
        }
    }

    display_results(b);

    return EXIT_SUCCESS;
}
