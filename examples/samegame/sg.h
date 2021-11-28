/// samegame.h

#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <algorithm>
#include <array>
#include <deque>
#include <iosfwd>
#include <memory>

template<typename Index, Index IndexNone>
struct ClusterT;

namespace sg {


inline constexpr auto WIDTH = 15;
inline constexpr auto HEIGHT = 15;
inline constexpr auto MAX_COLORS = 5;
inline constexpr auto MAX_CELLS = HEIGHT * WIDTH;
inline constexpr auto CELL_UPPER_LEFT = 0;
inline constexpr auto CELL_UPPER_RIGHT = WIDTH - 1;
inline constexpr auto CELL_BOTTOM_LEFT = (HEIGHT - 1) * WIDTH;
inline constexpr auto CELL_BOTTOM_RIGHT = MAX_CELLS - 1;
inline constexpr auto CELL_NONE = MAX_CELLS;

typedef int Cell;
enum class Color
{
  Empty = 0,
  Nb = MAX_COLORS + 1
};

/**
 * Simple wrapper around std::array representing the grid of a Samegame state.
 */
struct Grid
{
  using Array = std::array<Color, MAX_CELLS>;
  using value_type = Color;
  using reference = Color&;
  using const_reference = const Color&;
  using iterator = Array::iterator;
  using const_iterator = Array::const_iterator;
  using difference_type = Array::difference_type;
  using size_type = Array::size_type;

  bool operator==(const Grid& other) const { return m_data == other.m_data; }
  void swap(Grid& other) { std::swap(m_data, other.m_data); }
  size_type size() { return MAX_CELLS; }
  size_type max_size() { return MAX_CELLS; }
  bool empty() {
    return std::all_of(m_data.begin(), m_data.end(), [](const Color c) {
      return c == Color::Empty;
    });
  }
  Color& operator[](Cell c) { return m_data[c]; }
  Color operator[](Cell c) const { return m_data[c]; }
  iterator begin() { return m_data.begin(); }
  iterator end() { return m_data.end(); }
  const_iterator begin() const { return m_data.cbegin(); }
  const_iterator end() const { return m_data.cend(); }
  const_iterator cbegin() { return m_data.cbegin(); }
  const_iterator cend() { return m_data.end(); }

private:
  Array m_data{Color::Empty};

public:
  mutable size_type n_empty_rows{0};
};

inline constexpr Grid EMPTY_GRID();

//using Key = uint64_t;
auto inline constexpr N_ZOBRIST_KEYS = (MAX_CELLS + 1) * MAX_COLORS;

// State descriptor
typedef std::array<int, MAX_COLORS + 1> ColorCounter;
using Cluster = ClusterT<Cell, CELL_NONE>;

// Cluster or Action descriptor
struct ClusterData
{
  Cell rep{CELL_NONE};
  Color color{Color::Empty};
  size_t size{0};
};
using ClusterDataVec = std::vector<ClusterData>;
enum class Output
{
  CONSOLE,
  FILE
};

/**
 * @Class An interface to the game exposing it only as a general State/Action system.
 *
 * @Note `StateData` and `ClusterData` are compact descriptors for the Grid and the
 * Clusters respectively.
 */
class State
{
 public:
  using reward_type = double;
  using key_type = uint64_t;
  using action_type = ClusterData;
  using player_type = bool;

  State();
  explicit State(std::istream&);
  State(Grid&&, ColorCounter&&);
  State(key_type, const Grid&, const ColorCounter&);

  ClusterDataVec valid_actions_data() const;
  bool apply_action(const ClusterData&);
  ClusterData apply_random_action(Color = Color::Empty);
  reward_type evaluate(const ClusterData&) const;
  reward_type evaluate_terminal() const;
  bool is_terminal() const;
  key_type key();
  bool is_trivial(const ClusterData& cd) const { return cd.size < 2; }
  bool is_empty() const { return m_cells[CELL_BOTTOM_LEFT] == Color::Empty; }
  const Grid& grid() const { return m_cells; }
  const ColorCounter& color_counter() const { return m_cnt_colors; }
  friend std::ostream& operator<<(std::ostream&, const State&);

  ///TODO: Get rid of this! (Move to namespace scope)
  ClusterData get_cd(Cell rep) const;
  void display(Cell rep) const;
  void show_clusters() const;
  void view_action_sequence(const std::vector<ClusterData>&, int = 0) const;
  void log_action_sequence(std::ostream&,
                           const std::vector<ClusterData>&) const;

  bool operator==(const State& other) const { return m_cells == other.m_cells; }

 private:
  key_type m_key;
  Grid m_cells;
  ColorCounter m_cnt_colors;
};

/** Display a colored board with the chosen cluster highlighted. */
extern std::ostream& operator<<(
    std::ostream&, const std::pair<const State&, Cell>&);
extern std::ostream& operator<<(
    std::ostream&, const std::pair<Grid&, int>&);
extern std::ostream& operator<<(
    std::ostream&, const State&);
extern std::ostream& operator<<(
    std::ostream&, const ClusterData&);
inline bool operator==(const Grid& a, const Grid& b) {
  return a.operator==(b);
}
inline bool operator==(const ClusterData& a, const ClusterData& b) {
 return a.rep == b.rep && a.color == b.color && a.size == b.size;
}

template<typename _Index_T, _Index_T IndexNone>
extern bool operator==(
    const ClusterT<_Index_T, IndexNone>& a,
    const ClusterT<_Index_T, IndexNone>& b);
template<>
inline bool operator==(
    const ClusterT<Cell, MAX_CELLS>& a,
    const ClusterT<Cell, MAX_CELLS>& b)
{
  return a == b;
}

} // namespace sg

template<typename E>
auto inline constexpr to_integral(E e)
{
  return static_cast<std::underlying_type_t<E>>(e);
}

template<typename E, typename I>
auto inline constexpr to_enum(I i)
{
  return static_cast<E>(i);
}








#endif
