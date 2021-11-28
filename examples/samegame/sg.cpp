#include "sg.h"
#include "display.h"
#include "dsu.h"
#include "utils/rand.h"
#include "utils/zobrist.h"

#include <chrono>
#include <deque>
#include <iostream>
#include <set>
#include <thread>


namespace sg::clusters {

namespace {

/**
 * Data structure to partition the grid into clusters by colors
 */
DSU<sg::Cluster, sg::MAX_CELLS> grid_dsu{};

/**
 * Utility class initializing a random number generator and implementing
 * the methods we need for the random actions.
 */
Rand::Util<Cell> rand_util{};

//************************************** Grid manipulations **********************************/

///TODO Remove the 'empty cel ls' stuff, or do something with it
/**
 * Populate the disjoint data structure grid_dsu with all adjacent clusters
 * of cells sharing a same color.
 */
void generate_clusters(const Grid& _grid)
{
  grid_dsu.reset();

  auto n_empty_rows = _grid.n_empty_rows;
  bool row_empty = true;
  bool found_empty_cell = false;
  Cell empty_cell = CELL_NONE;

  auto row = HEIGHT - 1;
  // Iterate from bottom row upwards so we can stop at the first empty row.
  while (row > n_empty_rows)
  {
    row_empty = true;
    // Iterate through all columns except last
    for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell)
    {
      if (_grid[cell] == Color::Empty)
        continue;

      // compare up
      if (_grid[cell] == _grid[cell - WIDTH])
        grid_dsu.unite(cell, cell - WIDTH);

      // compare right
      if (_grid[cell] == _grid[cell + 1])
        grid_dsu.unite(cell, cell + 1);
    }
    Cell _cell = (row + 1) * WIDTH - 1;
    // Last column
    // If the row was empty, we are done.
    if ((row_empty = row_empty && _grid[_cell] == Color::Empty))
    {
      _grid.n_empty_rows = row;
      return;
    }
    // Compare up
    if (_grid[_cell] == _grid[_cell - WIDTH])
    {
      grid_dsu.unite(_cell, _cell - WIDTH);
    }
    --row;
  }

  row_empty = true;
  // The upmost non-empty row: only compare right
  for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell)
  {
    if (_grid[cell] == Color::Empty)
      continue;

    if (_grid[cell] == _grid[cell + 1])
      grid_dsu.unite(cell, cell + 1);
  }
  if ((row_empty = row_empty && _grid[CELL_UPPER_RIGHT] == Color::Empty))
    ++_grid.n_empty_rows;
}

/**
 * Make cells drop down if they lie above empty cells.
 */
void pull_cells_down(Grid& _grid)
{
  // For all columns
  for (int i = 0; i < WIDTH; ++i)
  {
    // stack the non-zero colors going up
    std::array<Color, HEIGHT> new_column{Color::Empty};
    int new_height = 0;

    for (int j = 0; j < HEIGHT; ++j)
    {
      auto bottom_color = _grid[i + (HEIGHT - 1 - j) * WIDTH];
      if (bottom_color != Color::Empty)
      {
        new_column[new_height] = bottom_color;
        ++new_height;
      }
    }
    // pop back the value (including padding with 0)
    for (int j = 0; j < HEIGHT; ++j)
      _grid[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
  }
}

/**
 * Stack the non-empty columns towards the left, leaving empty columns
 * only at the right side of the grid.
 */
void pull_cells_left(Grid& _grid)
{
  int i = 0;
  std::deque<int> zero_col;

  while (i < WIDTH)
  {
    if (zero_col.empty())
    {
      // Look for empty column
      while (i < WIDTH - 1 && _grid[i + (HEIGHT - 1) * WIDTH] != Color::Empty)
        ++i;
      zero_col.push_back(i);
      ++i;
    }
    else
    {
      int x = zero_col.front();
      zero_col.pop_front();
      // Look for non-empty column
      while (i < WIDTH && _grid[i + (HEIGHT - 1) * WIDTH] == Color::Empty)
      {
        zero_col.push_back(i);
        ++i;
      }
      if (i == WIDTH)
        break;
      // Swap the non-empty column with the first empty one
      for (int j = 0; j < HEIGHT; ++j)
        std::swap(_grid[x + j * WIDTH], _grid[i + j * WIDTH]);
      zero_col.push_back(i);
      ++i;
    }
  }
}

/**
 * Empty the cells of the cluster to which the given cell belongs, if
 * that cluster is valid.
 *
 * @Return The cluster descriptor of the given cell.
 */
ClusterData kill_cluster(Grid& _grid, const Cell _cell)
{
  const Color color = _grid[_cell];
  ClusterData cd{_cell, color, 0};

  if (_cell == CELL_NONE || color == Color::Empty)
    return cd;

  std::array<Cell, sg::MAX_CELLS> queue;
  queue[0] = _cell;
  int ndx_back = 1;
  Cell cur = CELL_NONE;

  _grid[_cell] = Color::Empty;
  ++cd.size;

  while (!(ndx_back == 0))
  {
    cur = queue[--ndx_back];

    // We remove the cells adjacent to `cur` with the target color
    // Look right
    if (cur % WIDTH < WIDTH - 1 && _grid[cur + 1] == color)
    {
      queue[ndx_back] = cur + 1;
      ++ndx_back;
      _grid[cur + 1] = Color::Empty;
      ++cd.size;
    }
    // Look down
    if (cur < (HEIGHT - 1) * WIDTH && _grid[cur + WIDTH] == color)
    {
      queue[ndx_back] = cur + WIDTH;
      ++ndx_back;
      _grid[cur + WIDTH] = Color::Empty;
      ++cd.size;
    }
    // Look left
    if (cur % WIDTH > 0 && _grid[cur - 1] == color)
    {
      queue[ndx_back] = cur-1;
      ++ndx_back;
      _grid[cur - 1] = Color::Empty;
      ++cd.size;
    }
    // Look up
    if (cur > WIDTH - 1 && _grid[cur - WIDTH] == color)
    {
      queue[ndx_back] = cur - WIDTH;
      ++ndx_back;
      _grid[cur - WIDTH] = Color::Empty;
      ++cd.size;
    }
  }
  // If only the rep was killed (cluster of size 1), restore it
  if (cd.size == 1)
    _grid[_cell] = color;

  return cd;
}

/// NOTE This is by far the hot spot in execution! (92% is spent here
/**
 * Kill a random cluster
 *
 * @Return The cluster that was killed, or some cluster of size 0 or 1.
 */
ClusterData kill_random_cluster(Grid& _grid, const Color target_color = Color::Empty)
{
  using CellnColor = std::pair<Cell, Color>;
  ClusterData ret{};

  // Random numbers from n_empty_rows to HEIGHT at the beginning of the array
  std::array<int, HEIGHT> rows = rand_util.gen_ordering<HEIGHT>(_grid.n_empty_rows, HEIGHT);

  // Array to hold the non-empty cells found.
  std::array<Cell, WIDTH> non_empty{};

  // Because n_empty_rows might get updated during the loop!
  const int len_rows = HEIGHT - _grid.n_empty_rows;
  Cell _cell{};
  Color _color{};
  int nonempty_ndx;
  int target_ndx;

  for (auto row_it = rows.begin(); row_it != rows.begin() + len_rows; ++row_it)
  {
    if (*row_it < _grid.n_empty_rows)
      continue;

    // Keep track of which cells are non-empty in that row.
    nonempty_ndx = -1;
    target_ndx = -1;
    for (Cell c = *row_it * WIDTH; c < (*row_it+1) * WIDTH -1; ++c)
    {
      if (_grid[c] != Color::Empty)
        non_empty[++nonempty_ndx] = c;
    }

    // If we just found a new empty row, update n_empty_rows.
    if (nonempty_ndx == -1)
    {
      _grid.n_empty_rows = *row_it;
      continue;
    }

    // Otherwise shuffle the non-empty cells and try to kill a cluster there
    // Aim for the target color first.
    rand_util.shuffle<WIDTH>(non_empty, nonempty_ndx);

    for (auto it = non_empty.begin(); it != non_empty.begin() + nonempty_ndx; ++it)
    {
      if (_grid[*it] == target_color)
        ret = kill_cluster(_grid, *it);
      if (ret.size > 1)
        return ret;
    }
    for (auto it = non_empty.begin(); it != non_empty.begin() + nonempty_ndx; ++it)
    {
      if (_grid[*it] != target_color)
        ret = kill_cluster(_grid, *it);
      if (ret.size > 1)
        return ret;
    }
  }

  return ret;
}

} // namespace

void input(std::istream& _in, Grid& _grid, ColorCounter& _cnt_colors)
{
  _grid.n_empty_rows = {0};
  int _in_color{0};
  Color _color{Color::Empty};
  bool row_empty{true};

  for (auto row = 0; row < HEIGHT; ++row)
  {
    row_empty = true;

    for (auto col = 0; col < WIDTH; ++col)
    {
      _in >> _in_color;
      _color = to_enum<Color>(_in_color + 1);
      _grid[col + row * WIDTH] = _color;

      // Generate the color data at the same time
      if (_color != Color::Empty)
      {
        row_empty = false;
        _cnt_colors[to_integral(_color)];
      }
    }
    // Count the number of empty rows (we're going from top to down)
    if (row_empty)
      ++_grid.n_empty_rows;
  }
}

std::vector<Cluster> get_valid_clusters(const Grid& _grid)
{
  std::vector<Cluster> ret;
  ret.reserve(MAX_CELLS);
  generate_clusters(_grid);
  for (auto it = grid_dsu.cbegin(); it != grid_dsu.cend(); ++it)
  {
    if (auto ndx = std::distance(grid_dsu.cbegin(), it);
        _grid[ndx] != Color::Empty && it->size() > 1)
      ret.emplace_back(*it);
  }
  return ret;
}

bool same_as_right_nbh(const Grid& _grid, const Cell _cell)
{
  const Color color = _grid[_cell];
  // check right if not already at the right edge of the _grid
  if (_cell % (WIDTH - 1) != 0 && _grid[_cell + 1] == color)
    return true;
  return false;
}

/**
 * Check if the neighbor at the right or on top of a cell has the same color.
 *
 * NOTE: If called with an empty cell, it will return true if a
 * neighbor is also empty.
 */
bool same_as_right_or_up_nbh(const Grid& _grid, const Cell _cell)
{
  const Color color = _grid[_cell];
  // check right if not already at the right edge of the _grid
  if (_cell % (WIDTH - 1) != 0 && _grid[_cell + 1] == color)
    return true;
  // check up if not on the first row
  if (_cell > CELL_UPPER_RIGHT && _grid[_cell - WIDTH] == color)
    return true;
  return false;
}

/**
 * Iterate through the cells like in the generate_clusters() method,
 * but returns false as soon as it identifies a cluster.
 */
bool has_nontrivial_cluster(const Grid& _grid)
{
  auto n_empty_rows = _grid.n_empty_rows;
  bool row_empty = true;

  auto row = HEIGHT - 1;
  // Iterate from bottom row upwards so we can stop at the first empty row.
  while (row > n_empty_rows)
  {
    // All the row except last cell
    for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell)
    {
      if (_grid[cell] == Color::Empty)
        continue;
      row_empty = false;
      // compare up
      if (_grid[cell] == _grid[cell - WIDTH])
        return true;
      // compare right
      if (_grid[cell] == _grid[cell + 1])
        return true;
    }
    // If the last cell of the row is empty
    if (_grid[(row + 1) * WIDTH - 1] == Color::Empty)
    {
      if (row_empty)
        return false;
      continue;
    }
    // If not (compare up)
    if (_grid[(row + 1) * WIDTH - 1] == _grid[row * WIDTH])
      return true;
    --row;
    row_empty = true;
  }
  // The upmost non-empty row: only compare right
  for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell)
  {
    if (_grid[cell] != Color::Empty && _grid[cell + 1] == _grid[cell])
      return true;
  }
  return true;
}

/**
 * Builds the cluster to which the given cell belongs
 * to directly from the grid.
 */
Cluster get_cluster(const Grid& _grid, const Cell _cell)
{
  Color color = _grid[_cell];

  if (color == Color::Empty)
  {
    return Cluster();
    std::cerr << "WARNING! Called get_cluster with an empty color" << std::endl;
  }

  Cluster ret{
      _cell,
  };
  ret.members.reserve(MAX_CELLS);
  std::deque<Cell> queue{_cell};
  std::set<Cell> seen{_cell};

  Cell cur;

  while (!queue.empty())
  {
    cur = queue.back();
    queue.pop_back();
    // Look right
    if (cur % WIDTH < WIDTH - 1 && seen.insert(cur + 1).second)
    {
      if (_grid[cur + 1] == color)
      {
        ret.push_back(cur + 1);
        queue.push_back(cur + 1);
      }
    }
    // Look down
    if (cur < (HEIGHT - 1) * WIDTH && seen.insert(cur + WIDTH).second)
    {
      if (_grid[cur + WIDTH] == color)
      {
        ret.push_back(cur + WIDTH);
        queue.push_back(cur + WIDTH);
      }
    }
    // Look left
    if (cur % WIDTH > 0 && seen.insert(cur - 1).second)
    {
      if (_grid[cur - 1] == color)
      {
        ret.push_back(cur - 1);
        queue.push_back(cur - 1);
      }
    }
    // Look up
    if (cur > WIDTH - 1 && seen.insert(cur - WIDTH).second)
    {
      if (_grid[cur - WIDTH] == color)
      {
        ret.push_back(cur - WIDTH);
        queue.push_back(cur - WIDTH);
      }
    }
  }

  return ret;
}

ClusterData get_cluster_data(const Grid& _grid, const Cell _cell)
{
  const Cluster cluster = get_cluster(_grid, _cell);
  return ClusterData{
      .rep = cluster.rep, .color = _grid[_cell], .size = cluster.size()};
}

namespace {

/**
    * @Return The descriptor associated to the given cluster.
    */
ClusterData get_descriptor(const Grid& _grid, const Cluster& _cluster)
{
  ClusterData ret{.rep = _cluster.rep,
                  .color = _grid[_cluster.rep],
                  .size = _cluster.size()};
  return ret;
}
} // namespace

std::vector<ClusterData> get_valid_clusters_descriptors(const Grid& _grid)
{
  std::vector<ClusterData> ret{};

  std::vector<Cluster> tmp = get_valid_clusters(_grid);
  ret.reserve(tmp.size());

  std::transform(
      tmp.begin(),
      tmp.end(),
      back_inserter(ret),
      [&_grid](const auto& cluster) { return get_descriptor(_grid, cluster); });

  return ret;
}

ClusterData apply_action(Grid& _grid, const Cell _cell)
{
  ClusterData cd_ret = kill_cluster(_grid, _cell);
  if (cd_ret.size > 1)
  {
    pull_cells_down(_grid);
    pull_cells_left(_grid);
  }
  return cd_ret;
}

ClusterData apply_random_action(Grid& _grid, const Color target_color)
{
  ClusterData cd_ret = kill_random_cluster(_grid, target_color);
  if (cd_ret.size > 1)
  {
    pull_cells_down(_grid);
    pull_cells_left(_grid);
  }
  return cd_ret;
}

} // namespace sg::clusters

namespace sg::zobrist {

struct ZobristIndex
{
  //  NOTE: The upper left cell in the grid corresponds to 0,
  // so wee need to increment the cells when computing the key!
  auto operator()(const Cell cell, const Color color)
  {
    return (cell + 1) * to_integral(color);
  }
};

typedef ::zobrist::KeyTable<ZobristIndex, State::key_type, N_ZOBRIST_KEYS> ZTable;
ZTable Table{};

State::key_type get_key(const Cell _cell, const Color _color)
{
  return Table(_cell, _color);
}

/**
 * Xor with a unique random key for each (index, color) appearing in the grid.
 * Also compute is_terminal() (Key will have first bit on once is_terminal() is known,
 * and it will be terminal iff the second bit is on).
 * Also records the number of empty rows in passing.
 */
State::key_type get_key(const Grid& _grid)
{
  State::key_type key = 0;
  bool row_empty = false, terminal_status_known = false;

  // All rows except the first one
  for (auto row = HEIGHT - 1; row > 0; --row)
  {
    row_empty = true;

    // TODO Do as in generate_clusters and write two loops so that same_as_right_nbh
    // doesn't have to make a check for cell < (ROW+1)*WIDTH-1
    for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH; ++cell)
    {
      if (const Color color = _grid[cell]; color != Color::Empty)
      {
        row_empty = false;
        key ^= Table(cell, color);

        // If the terminal status of the _grid is known, continue
        if (terminal_status_known)
          continue;

        // Otherwise, mark termin_status_known as true if current cell
        // is part of a non-trivial cluster
        if (clusters::same_as_right_or_up_nbh(_grid, cell))
          terminal_status_known = true;
      }
    }
    // Stop if there are no more non-empty cells upwards
    if (row_empty)
      break;
  }
  // Repeat for first row but only checking the right neighbour for clusters
  for (auto cell = CELL_UPPER_LEFT; cell < CELL_UPPER_RIGHT; ++cell)
  {
    if (const Color color = _grid[cell]; color != Color::Empty)
    {
      row_empty = false;

      key ^= Table(cell, color);

      // If the terminal status of the _grid is known, continue
      if (terminal_status_known)
        continue;

      // Otherwise keep checking for nontrivial clusters upwards and forward
      if (clusters::same_as_right_nbh(_grid, cell))
        // Indicate that terminal status is known
        terminal_status_known = true;
    }
  }

  // flip the first bit if we found out _grid was non-terminal earlier,
  // otherwise flip both the first and second bit.
  key += terminal_status_known ? 1 : 3;

  return key;
}

} // namespace sg::zobrist

namespace sg::display {

enum class Color_codes : int
{
  BLACK = 30,
  RED = 31,
  GREEN = 32,
  YELLOW = 33,
  BLUE = 34,
  MAGENTA = 35,
  CYAN = 36,
  WHITE = 37,
  B_BLACK = 90,
  B_RED = 91,
  B_GREEN = 92,
  B_YELLOW = 93,
  B_BLUE = 94,
  B_MAGENTA = 95,
  B_CYAN = 96,
  B_WHITE = 97,
};

enum class Shape : int
{
  SQUARE,
  DIAMOND,
  B_DIAMOND,
  NONE
};

std::map<Shape, std::string> Shape_codes{
    {Shape::SQUARE, "\u25A0"},
    {Shape::DIAMOND, "\u25C6"},
    {Shape::B_DIAMOND, "\u25C7"},
};

std::string shape_unicode(Shape s) { return Shape_codes[s]; }

std::string color_unicode(Color c)
{
  return std::to_string(to_integral(Color_codes(to_integral(c) + 90)));
}

std::string print_cell(
    const sg::Grid& grid,
    Cell ndx,
    Output output_mode,
    const ClusterT<Cell, CELL_NONE>& cluster = ClusterT<Cell, CELL_NONE>())
{
  bool ndx_in_cluster =
      find(cluster.cbegin(), cluster.cend(), ndx) != cluster.cend();
  std::stringstream ss;

  if (output_mode == Output::CONSOLE)
  {
    Shape shape = ndx == cluster.rep ? Shape::B_DIAMOND
                  : ndx_in_cluster   ? Shape::DIAMOND
                                     : Shape::SQUARE;

    ss << "\033[1;" << color_unicode(grid[ndx]) << "m" << shape_unicode(shape)
       << "\033[0m";

    return ss.str();
  }
  else
  {
    // Underline the cluster representative and output the cluster in bold
    std::string formatter = "";
    if (ndx_in_cluster)
      formatter += "\e[1m]";
    if (ndx == cluster.rep)
      formatter += "\033[4m]";

    ss << formatter << std::to_string(to_integral(grid[ndx])) << "\033[0m\e[0m";

    return ss.str();
  }
}

const int x_labels[15]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

const std::string
to_string(const Grid& grid, const Cell cell, sg::Output output_mode)
{
  using Cluster = ClusterT<Cell, MAX_CELLS>;

  Cluster cluster = clusters::get_cluster(grid, cell);
  cluster.rep = cell;
  bool labels = true;
  std::stringstream ss{std::string("\n")};

  // For every row
  for (int y = 0; y < HEIGHT; ++y)
  {
    if (labels)
      ss << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";

    // Print row except last entry
    for (int x = 0; x < WIDTH - 1; ++x)
      ss << print_cell(grid, x + y * WIDTH, output_mode, cluster) << ' ';

    // Print last entry,
    ss << print_cell(grid, WIDTH - 1 + y * WIDTH, output_mode, cluster) << '\n';
  }

  if (labels)
  {
    ss << std::string(34, '_') << '\n' << std::string(5, ' ');

    for (int x : x_labels)
      ss << x << ((x < 10) ? " " : "");
    ss << '\n';
  }

  return ss.str();
}

void enumerate_clusters(std::ostream& _out, const Grid& _grid)
{
  using namespace std::literals::chrono_literals;

  ClusterDataVec cd_vec = clusters::get_valid_clusters_descriptors(_grid);

  for (const auto& cd : cd_vec)
  {
    Cluster cluster = clusters::get_cluster(_grid, cd.rep);
    _out << cluster;
    std::this_thread::sleep_for(500.0ms);
  }
}

void view_clusters(std::ostream& _out, const Grid& _grid)
{
  using namespace std::literals::chrono_literals;

  ClusterDataVec cd_vec = clusters::get_valid_clusters_descriptors(_grid);

  for (const auto& cd : cd_vec)
  {
    _out << to_string(_grid, cd.rep);
    std::this_thread::sleep_for(300.0ms);
  }
}

void view_action_sequence(std::ostream& out,
                          Grid& grid,
                          const std::vector<ClusterData>& actions,
                          int delay_in_ms)
{
  PRINT("Printing ",
        actions.size(),
        " actions from the following starting grid:\n",
        to_string(grid));

  unsigned int score = 0;
  Grid grid_before_action{};
  ClusterData cd_check{};

  for (auto it = actions.begin(); it != actions.end(); ++it)
  {
    grid_before_action = grid;
    cd_check = clusters::apply_action(grid, it->rep);

    // Verify action is valid
    if (cd_check.rep == CELL_NONE || cd_check.color == Color::Empty
        || cd_check.size < 2)
    {
      PRINT("\n    WARNING: Action number ",
            std::distance(actions.begin(), it),
            " is invalid.");
      if (it != actions.end())
      {
        PRINT(" Skipping the remaining ",
              std::distance(it, actions.end()),
              " actions.");
        return;
      }
    }

    // Add up score of action
    double val = it->size - 2.0;
    val = (val + std::abs(val)) / 2.0;
    score += std::pow(val, 2);

    // Display
    PRINT(to_string(grid_before_action, it->rep), "SCORE : ", score, '\n');
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_in_ms));
  }

  score += 1000 * (grid[CELL_BOTTOM_LEFT] == Color::Empty);
  PRINT(to_string(grid), "\nFINAL SCORE : ", score);
}

void log_action_sequence(std::ostream& out,
                         Grid& grid,
                         const std::vector<ClusterData>& actions)
{
  double score = 0;
  ClusterData cd_check{};

  out << "Computing score.\n";

  for (auto it = actions.begin(); it != actions.end(); ++it)
  {
    cd_check = clusters::apply_action(grid, it->rep);

    if (cd_check.rep == CELL_NONE || cd_check.color == Color::Empty
        || cd_check.size < 2)
    {
      out << "\n    WARNING: Action number "
          << std::distance(actions.begin(), it) << " is invalid." << std::endl;

      if (it != actions.end())
      {
        out << " Skipping the remaining " << std::distance(it, actions.end())
            << " actions." << std::endl;
        return;
      }
    }

    int val = it->size - 2;
    val = (val + std::abs(val)) / 2;
    score += std::pow(val, 2);
  }

  score += 1000 * (grid[CELL_BOTTOM_LEFT] == Color::Empty);

  out << "    FINAL SCORE : " << score << std::endl;
}

} // namespace sg::display



namespace sg {

State::State()
  : m_key(0), m_cells{}, m_cnt_colors{0}
{
}

State::State(Grid&& grid, ColorCounter&& ccolors)
  : m_key(0), m_cells(grid), m_cnt_colors(ccolors) { }

State::State(std::istream& _in) : m_key(), m_cells{}, m_cnt_colors{}
{
  clusters::input(_in, m_cells, m_cnt_colors);
}

State::State(key_type key, const Grid& cells, const ColorCounter& ccolors)
  : m_key(key), m_cells{cells}, m_cnt_colors{ccolors}
{
}

//****************************************** Actions methods ***************************************/

ClusterDataVec State::valid_actions_data() const
{
  return clusters::get_valid_clusters_descriptors(m_cells);
}

bool key_uninitialized(const Grid& grid, State::key_type key)
{
  return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
}

State::key_type State::key()
{
  if (key_uninitialized(m_cells, m_key))
    return m_key = zobrist::get_key(m_cells);
  return m_key;
}

/**
 * First check if the key contains the answer, check for clusters but return
 * false as soon as it finds one instead of computing all clusters.
 */
bool State::is_terminal() const
{
  // If the first bit is on, then it has been computed and stored in the second bit.
  if (m_key & 1)
  {
    return m_key & 2;
  }
  return !clusters::has_nontrivial_cluster(m_cells);
}

//******************************** Apply / Undo actions **************************************/

bool State::apply_action(const ClusterData& cd)
{
  ClusterData res = clusters::apply_action(m_cells, cd.rep);
  m_cnt_colors[to_integral(res.color)] -= (res.size > 1) * res.size;
  m_key = 0;
  return !is_trivial(res);
}

ClusterData State::apply_random_action(Color target)
{
  ClusterData cd = clusters::apply_random_action(m_cells, target);
  m_cnt_colors[to_integral(cd.color)] -= cd.size;
  return cd;
}

//*************************** Display *************************/

ClusterData State::get_cd(Cell rep) const
{
  return sg::clusters::get_cluster_data(m_cells, rep);
}

void State::display(Cell rep) const
{
  std::cout << display::to_string(m_cells, rep) << std::endl;
}

void State::show_clusters() const
{
  display::view_clusters(std::cout, m_cells);
}

void State::view_action_sequence(const std::vector<ClusterData>& actions,
                                 int delay_in_ms) const
{
  Grid grid_copy = m_cells;
  display::view_action_sequence(std::cout, grid_copy, actions, delay_in_ms);
}

void State::log_action_sequence(std::ostream& out,
                                const std::vector<ClusterData>& actions) const
{
  Grid grid_copy = m_cells;
  display::log_action_sequence(out, grid_copy, actions);
}

std::ostream& operator<<(std::ostream& _out, const std::pair<Grid&, Cell>& _ga)
{
  return _out << display::to_string(_ga.first, _ga.second);
}

std::ostream& operator<<(std::ostream& _out,
                         const std::pair<const State&, Cell>& _sc)
{
  return _out << display::to_string(_sc.first.grid(), _sc.second);
}

std::ostream& operator<<(std::ostream& _out, const State& _state)
{
  return _out << display::to_string(_state.m_cells, CELL_NONE);
}

std::ostream& operator<<(std::ostream& _out, const ClusterData& _cd)
{
  return _out << _cd.rep << ' ' << display::to_string(_cd.color) << ' '
              << std::to_string(_cd.size);
}

//*************************** Evaluation *************************/

/**
 * @Note clamp(v, l, h) assigns l to v if v < l or assigns h to v if h < v else returns v.
 * In this case, (2-size)^2 is always greater than (2-size) so we're just returning
 * [max(0, size-2)]^2 in a more efficient way.
 */
State::reward_type State::evaluate(const ClusterData& action) const
{
  double val = action.size - 2.0;
  val = (val + std::abs(val)) / 2.0;
  return std::pow(val, 2) * 0.0025;
}

State::reward_type State::evaluate_terminal() const
{
  return static_cast<reward_type>(is_empty()) * 1000.0 * 0.0025;
}


} //namespace sg
