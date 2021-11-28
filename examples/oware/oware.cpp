#include "oware.h"

#include <algorithm>
#include <array>
#include <set>
#include <iostream>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#include "utils/rand.h"
#include "utils/zobrist.h"

////////////////////////////////////////////////////////////////////////////////
// Util functions for game mechanics
////////////////////////////////////////////////////////////////////////////////
namespace {

inline const auto& cur_player_holes(const Board& b)
{
    return b.holes(b.side_to_move());
}
inline const auto& other_player_holes(const Board& b)
{
    return b.holes(!b.side_to_move());
}
inline int cur_player_mancala(const Board& b)
{
    return b.mancala(b.side_to_move());
}
inline int other_player_mancala(const Board& b)
{
    return b.mancala(!b.side_to_move());
}
inline void distribute(std::array<int, 6>& holes,
    int& m,
    int& n_beads,
    bool cur_player)
{
    // Indices of holes are increasing if cur_player is the
    // first player, otherwise they are decreasing
    int pm = cur_player ? 1 : -1;

    // Last hole is on our right or on our left depending on
    // the player.
    int end = cur_player ? 6 : -1;

    while (n_beads > 0) {
        // Go to next hole
        m += pm;
        // Stop if we went beyond the last hole
        if (m == end)
            break;
        // Drop one bead
        ++holes[m];
        --n_beads;
    }
}

/** Drop a bead in a mancala. */
inline void feed_mancala(int& mancala, int n_beads = 1)
{
    mancala += n_beads;
}

/**
 * If the last bead was dropped in one of the current player's empty
 * holes, and the hole directy accross it is not empty, the current
 * player captures the beads from both holes.
 */
inline void capture_if_can(std::array<int, 6>& cur_player_holes,
    std::array<int, 6>& other_player_holes,
    int& cur_player_mancala,
    int pos)
{
    auto& last_hole = cur_player_holes[pos];
    auto& beads_accross = other_player_holes[pos];

    if (last_hole == 1 && beads_accross > 0) {
        cur_player_mancala += beads_accross + 1;
        last_hole = beads_accross = 0;
    }
}

/** Pickup the beads from a hole */
inline int pickup_beads(int& hole)
{
    int beads = hole;
    hole = 0;
    return beads;
}

/** Checks if all holes are empty. */
inline bool side_empty(const Board& b, bool player)
{
    const auto& holes = b.holes(player);
    return std::all_of(holes.begin(), holes.end(), [](int cnt) {
        return cnt == 0;
    });
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
// Constructor and accessors
////////////////////////////////////////////////////////////////////////////////
Board::Board()
    : player2 { 4, 4, 4, 4, 4, 4 }
    , player1 { 4, 4, 4, 4, 4, 4 }
    , man_player2 { 0 }
    , man_player1 { 0 }
    , m_player { 1 }
{
}

const std::array<int, 6>& Board::holes(bool player) const
{
    return player ? player1 : player2;
}
int Board::mancala(bool player) const
{
    return player ? man_player1 : man_player2;
}
bool Board::side_to_move() const
{
    return m_player;
}

////////////////////////////////////////////////////////////////////////////////
// Valid actions and evaluation methods
////////////////////////////////////////////////////////////////////////////////
std::vector<int> Board::valid_actions() const
{
    std::vector<int> ret;

    if (is_terminal())
        return ret;

    const auto& cur_player_holes = m_player ? player1 : player2;

    for (auto [hole_ndx, hole_it] = std::make_pair(0, cur_player_holes.begin());
         hole_ndx < 6;
         ++hole_ndx, ++hole_it) {
        if (*hole_it > 0) {
            ret.push_back(hole_ndx);
        }
    }

    return ret;
}

bool Board::is_terminal() const
{
    return side_empty(*this, 0) || side_empty(*this, 1);
}

/**
 * Return [score1, score2]
*/
std::pair<int, int> Board::final_score() const
{
    int player1_score = std::accumulate(player1.begin(), player1.end(), man_player1);
    int player2_score = std::accumulate(player2.begin(), player2.end(), man_player2);

    return std::make_pair(player1_score, player2_score);
}

double Board::evaluate(int action) const
{
    return 0.0;
}

/**
 * Return 1.0 if the player who played the last move won, 0.0 if they lost,
 * 0.5 if the game is drawn.
*/
double Board::evaluate_terminal(const Board& b)
{
    auto [p1_score, p2_score] = b.final_score();

    if (p1_score == p2_score)
        return 0.5;

    int score_diff = b.side_to_move() == 0 ? p1_score - p2_score : p2_score - p1_score;

    return score_diff > 0;
}

////////////////////////////////////////////////////////////////////////////////
// The 'actions' used to simulate the game
////////////////////////////////////////////////////////////////////////////////
inline bool is_valid(int action)
{
    return action > -1 && action < 6;
}

bool Board::is_trivial(int action) const
{
    return is_valid(action)
        && cur_player_holes(*this)[action] == 0;
}

/**
 * Apply a whole game action (pick up beads and distribute them).
 *
 * The boolean returned indicates if the action was succesfully applied.
 * (So a return value of `false` indicates the state hasn't changed. This
 * is to be consistent with the mcts interface and should be fixed there.)
 */
bool Board::apply_action(int action)
{
    if (is_trivial(action))
        return false;

    bool player = m_player;
    auto& player_holes = player ? player1 : player2;
    auto& other_player_holes = player ? player2 : player1;
    auto& player_mancala = player ? man_player1 : man_player2;

    int n_beads = pickup_beads(player_holes[action]);

    while (n_beads > 0)
    {
        distribute(player_holes, action, n_beads, player);

        // If we distributed all beads before the player's mancala
        if (n_beads == 0)
        {
            capture_if_can(
                player_holes,
                other_player_holes,
                player_mancala,
                action);
            break;
        }

        feed_mancala(player_mancala);
        --n_beads;

        // If that last bead dropped the player's mancala was the
        // last one, AND the game is not terminal,
        // the current player gets to play again.
        if (n_beads == 0)
        {
            if (!is_terminal())
                return true;
            break;
        }

        distribute(other_player_holes, action, n_beads, !player);
    }

    // Switch the bool indicating whose turn it is.
    m_player = !m_player;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Random util
////////////////////////////////////////////////////////////////////////////////
namespace {

Rand::Util<int> rand_util {};

} // namespace

int Board::apply_random_action()
{
    auto _valid_actions = valid_actions();
    if (_valid_actions.empty())
        return -1;

    auto chosen = rand_util.choose(_valid_actions);
    apply_action(chosen);
    return chosen;
}

////////////////////////////////////////////////////////////////////////////////
// Hashing of boards
////////////////////////////////////////////////////////////////////////////////
namespace {

/**
     * Keys are arranged as follow:
     *
     * Then for the first hole of player0, have 48 random
     * keys at indices 0, 1..., 47.
     * Then Indices 48, 49, ..., 95 are for the first hole
     * of player 1, etc...
     *
     * Finally, the last two series of 48 entries are for the mancalas,
     * and we have two extra keys indicating the player whose turn it
     * is to play.
     */
constexpr int n_keys = (12 + 2) * 48;

/**
     * A functor taking in building blocks of boards
     * (the holes with their bead counts), then
     * returning the KTable index for the corresponding key.
     *
     * Then the whole key of a board is gotten by xoring the entries
     * found in the KTable for all.
     *
     * We also reserve the first bit of the key to indicate whose turn it is.
     */
struct Hash_fun {
    // The indices for the hole keys
    constexpr size_t operator()(size_t hole_ndx, size_t hole_cnt, bool player)
    {
        return 48 * (int(player) + 2 * hole_ndx) + hole_cnt;
    }

    // The index for the mancala keys
    constexpr size_t operator()(size_t mancala_cnt, bool player)
    {
        return (12 + int(player)) * 48 + mancala_cnt;
    }
};

::zobrist::KeyTable<Hash_fun, int, n_keys> KTable(1);

// Contribution from a hole
inline Board::key_type key_hole(const Board& b, size_t hole_ndx, bool player)
{
    return KTable(hole_ndx, b.holes(player)[hole_ndx], player);
}

// Contibution from a mancala
inline Board::key_type key_mancala(const Board& b, bool player)
{
    return KTable(b.mancala(player), player);
}

inline Board::key_type key_player(const Board& b)
{
    return int(b.side_to_move());
}

inline void test_key_mancala_ndx()
{
    Hash_fun get_ndx{ };
    for (bool p : { 0, 1 })
    {
        for (int n=0; n<48; ++n)
        {
            size_t ndx = get_ndx(n, p);
            std::cerr << "Player " << p
                      << ", mancala count: " << n
                      << "\nGives ndx = " << ndx
                      << std::endl;
        }
    }
}

inline void test_key_hole_ndx()
{
    Hash_fun get_ndx{ };

    for (bool p : { 0, 1 } )
    {
        for (int h=0; h<6; ++h)
        {
            for (int n=0; n<48; ++n)
            {
                size_t ndx = get_ndx(h, n, p);
                std::cerr << "Player " << p
                          << ", hole " << h
                          << ", count " << n
                          << "\nGives ndx = " << ndx
                          << std::endl;
                if (n % 12 == 0)
                {
                    std::cerr << "Input any char to continue..." << std::endl;
                    char _block{ };
                    std::cin >> _block; std::cin.ignore();
                }
            }
        }
    }
}

inline bool test_key_table()
{
    std::set<Board::key_type> distinct;
    for (const auto& k : KTable)
    {
        auto [it, inserted] = distinct.insert(k);
    }
    return distinct.size() == KTable.size();
}

} // namespace

bool Board::test_init() const
{
    bool kt = test_key_table();
    if (!kt)
    {
        std::cerr << "TEST_KEY_TABLE() failed..." << std::endl;
    }
    //test_key_mancala_ndx();
    return kt;
}


Board::key_type Board::key() const
{
    auto ret = key_player(*this);
    for (auto player : { 0, 1 }) {
        ret ^= key_mancala(*this, player);
        for (size_t hole_ndx = 0; hole_ndx < 6; ++hole_ndx) {
            ret ^= key_hole(*this, hole_ndx, player);
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Viewing utils
////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& _out, const Board& b)
{
    int spaces = b.man_player2 < 10 ? 2 : 1;
    _out << '\n'
         << b.man_player2
         << (b.man_player2 < 10 ? "  " : " ")
         << "| ";

    for (int i = 0; i < 6; ++i) {
        _out << b.player2[i]
             << (b.player2[i] < 10 ? "  " : " ");
    }
    _out << "\n     ";
    for (int i = 0; i < 6; ++i) {
        _out << b.player1[i]
             << (b.player1[i] < 10 ? "  " : " ");
    }
    _out << '|'
         << (b.man_player1 < 10 ? "  " : " ")
         << b.man_player1;

    _out << "\n     " << std::string(18, '-');
    _out << "\n     ";
    for (int i = 1; i < 7; ++i)
        _out << i << "  ";
    return _out << '\n';
}

bool Board::operator!=(const Board& other) const
{
    if (man_player1 != other.man_player1 || man_player2 != other.man_player2)
        return true;

    for (int i = 0; i < 6; ++i) {
        if (player1[i] != other.player1[i] || player2[i] != other.player2[i])
            return true;
    }
    return false;
}
