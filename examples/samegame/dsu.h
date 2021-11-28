#ifndef __DSU_H_
#define __DSU_H_

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <type_traits>
#include <vector>

template<typename Index, Index IndexNone>
struct ClusterT;

template<typename _Index, _Index _IndexNone>
std::ostream& operator<<(std::ostream&, const ClusterT<_Index, _IndexNone>&);

/**
 * A Cluster (of indices) has a `representative` index along with a container
 * containing all of its members.
 */
template<typename Index_T, Index_T IndexNone>
struct ClusterT
{
  // Basic type aliases
  using Index = Index_T; // This is to make the Index type available to DSU
  static_assert(
      std::is_integral<Index>::value);
  using value_type = Index;
  using Container = std::vector<Index>;
  using size_type = typename Container::size_type;
  using iterator = typename Container::iterator;
  using const_iterator = typename Container::const_iterator;

  // Data members
  Index rep;
  Container members;

  constexpr ClusterT() : rep{IndexNone}, members(1, IndexNone) {}
  /** For reseting the DSU. */
  explicit constexpr ClusterT(Index _ndx) : rep{_ndx}, members(1, _ndx) {}
  ClusterT(Container&& _cont) : rep{IndexNone}, members(_cont)
  {
    if (!_cont.empty())
    {
      rep = _cont.back();
    }
  }
  ClusterT(Index _ndx, Container&& _cont) : rep{_ndx}, members(_cont) {}

  void push_back(Index ndx) { members.push_back(ndx); }
  auto size() const { return members.size(); }

  auto begin() { return members.begin(); }
  auto end() { return members.end(); }
  auto cbegin() const { return members.begin(); }
  auto cend() const { return members.end(); }

  friend std::ostream& operator<<<>(std::ostream& _out,
                                    const ClusterT<Index, IndexNone>&);
  bool operator==(const ClusterT<Index, IndexNone>& other) const
  {
    if (size() != other.size())
    {
      return false;
    }
    auto this_members = members;
    auto other_members = other.members;

    std::sort(this_members.begin(), this_members.end());
    std::sort(other_members.begin(), other_members.end());

    for (auto it = this_members.begin(); it != this_members.end(); ++it)
    {
      if (*it != other_members[std::distance(this_members.begin(), it)])
      {
        return false;
      }
    }
    return true;
  }
};

/**
 * The `Disjoint Set Union` data structure represents a partition of indices into clusters
 * using an array. To find the cluster to which some element e_{i} belongs,
 * follow e_{i+1} = array[i]. When finally e_{i+1} == i then i is the representative and the
 * whole cluster is stored there.
 */
template<typename _Cluster, size_t N>
class DSU
{
 public:
  // Basic type aliases
  using Cluster = _Cluster;
  using Index = typename Cluster::Index;
  using Container = typename Cluster::Container;
  using ClusterList = std::array<_Cluster, N>;
  using value_type = Cluster;
  using iterator = typename ClusterList::iterator;
  using const_iterator = typename ClusterList::const_iterator;

  static const inline ClusterList init = []() {
    ClusterList _init{};
    std::array<typename _Cluster::Index, N> ndx;
    std::iota(ndx.begin(), ndx.end(), 0);
    std::transform(ndx.begin(), ndx.end(), _init.begin(), [](auto n) {
      return Cluster(n);
    });
    return _init;
  }();

  constexpr DSU() : m_clusters{init} {}

  constexpr void reset() { m_clusters = init; }

  /** Find the representative of the cluster to which a given cell belongs. */
  Index find_rep(Index ndx)
  {
    bool not_the_rep = m_clusters[ndx].rep != ndx;

    if (not_the_rep)
    {
      return m_clusters[ndx].rep = find_rep(m_clusters[ndx].rep);
    }

    return ndx;
  }

  /** Merge the two clusters to which the two given cells belong. */
  void unite(Index a, Index b)
  {
    a = find_rep(a);
    b = find_rep(b);

    if (a != b)
    {
      if (m_clusters[a].size() < m_clusters[b].size())
      {
        std::swap(a, b);
      }
      merge_clusters(m_clusters[a], m_clusters[b]);
    }
  }

  /** Return a copy of the cluster containing index ndx. */
  Cluster get_cluster(Index ndx)
  {
    auto rep = find_rep(ndx);
    return m_clusters[rep];
  }

  auto begin() { return m_clusters.begin(); }
  auto end() { return m_clusters.end(); }
  auto begin() const { return m_clusters.begin(); }
  auto end() const { return m_clusters.end(); }
  auto cbegin() const { return m_clusters.cbegin(); }
  auto cend() const { return m_clusters.cend(); }

 private:
  ClusterList m_clusters;

  /** Append a cluster at the end of another one. */
  void merge_clusters(Cluster& larger_a, Cluster& smaller_b)
  {
    smaller_b.rep = larger_a.rep;

    larger_a.members.insert(
        larger_a.cend(), smaller_b.begin(), smaller_b.end());

    smaller_b.members.clear();
  }
};

template<typename _Index, _Index _IndexNone>
inline std::ostream& operator<<(
    std::ostream& _out,
    const ClusterT<_Index, _IndexNone>& cluster)
{
  _out << "Rep =" << cluster.rep << " Members = {";
  for (auto m : cluster.members)
  {
    _out << m << ' ';
  }
  return _out << "}";
}

template<typename _Index_T, _Index_T _IndexNone>
inline bool operator==(
    const ClusterT<_Index_T, _IndexNone>& a,
    const ClusterT<_Index_T, _IndexNone>& b)
{
  return ClusterT<_Index_T, _IndexNone>::operator==(b);
}

#endif
