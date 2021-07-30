#include <iostream>
#include <random>

#include "board.h"
#include "bitboard.h"
#include "types.h"

using namespace BT;

std::ostream& operator<<(std::ostream& out, Move m) {
    return out << from_sq(m) << to_sq(m);
}



int main(int argc, char *argv[])
{
    std::cout << "Initializing the position..." << std::endl;

    Position pos { };

    std::cout << "\nDone! Position is\n"
              << pos
              << std::endl;

    auto valid_actions = pos.valid_actions();

    std::cout << "\nValid actions:\n"
              << std::endl;

    for (auto m : valid_actions) {
        std::cout << m << '\n';
    }

    Move chosen = valid_actions[rand() % valid_actions.size()];
    std::cout << "Chose move " << chosen << '\n';

    pos.apply_action(chosen);

    std::cout << "After applying action, state is\n"
              << pos
              << std::endl;

    std::cout << "Random playout:\n";
    while (!pos.is_terminal()) {
        valid_actions = pos.valid_actions();
        chosen = valid_actions[rand() % valid_actions.size()];
        std::cout << "\n***********\n"
                  << pos
                  << "\nAction: "
                  << chosen << '\n';
        pos.apply_action(chosen);
        std::cout << pos
                  << "\n************"
                  << std::endl;
    }

    Color winner = Position::winner(pos);

    std::cout << "\n*************"
              << "\nTERMINAL:\n\n"
              << pos
              << "\n\n"
              << std::endl;

    std::cout << "Winner is: "
              << (winner == Color::White ? "White" : "Black")
              << std::endl;

    return 0;
}
