#ifndef __MCTSTREE_H_
#define __MCTSTREE_H_

#include <algorithm>
#include <deque>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace mcts {

template <typename StateT, typename ActionT, size_t MAX_DEPTH>
class MctsTree;

template <typename StateT, typename ActionT, size_t MAX_DEPTH>
std::ostream& operator<<(std::ostream&, const MctsTree<StateT, ActionT, MAX_DEPTH>&);

template <typename StateT, typename ActionT, size_t MAX_DEPTH>
class MctsTree {
public:
    struct Node;
    struct Edge;
    using node_pointer = Node*;
    using edge_pointer = Edge*;
    using ChildrenContainer = std::vector<Edge>;
    using key_type = typename StateT::key_type;
    using reward_type = typename StateT::reward_type;
    using player_type = typename StateT::player_type;
    struct Node {
        key_type key;
        int n_visits;
        ChildrenContainer children;
    };
    struct Edge {
        ActionT action;
        player_type player;
        reward_type total_val;
        reward_type best_val;
        int n_visits;
        bool subtree_completed;
    };

    MctsTree(key_type);

    void set_root(const key_type key)
    {
        p_root = get_node(key);
        m_depth = 0;
    }
    node_pointer get_root() const
    {
        return p_root;
    }
    node_pointer get_node(const key_type key)
    {
        auto [node_it, inserted] = m_table.insert(std::pair { key, Node {
                                                                       .key = key,
                                                                   } });
        return &(node_it->second);
    }
    void traversal_push(edge_pointer edge)
    {
        m_edge_stack[m_depth] = edge;
        ++m_depth;
    }
    void backpropagate(reward_type reward, player_type player = player_type{})
    {
#ifdef DEBUG_BACKPROPAGATION
        std::cerr << "\n\n\nInside the tree, we update the stats of each edges above "
                  << " and carefully manage the players point of views:\n"
                  << " Whenever the edge leading to the current depth corresponds"
                  << " to an action of the opposite player, we flip the value "
                  << " to preserve its invariance (what is good for me is bad for them"
                  << " and vice-versa...\n"
                  << std::endl;
#endif

        while (m_depth > 0) {

#ifdef DEBUG_BACKPROPAGATION
            std::cerr << "\nDepth: " << m_depth
                      << ", Reward: " << reward
                      << std::endl;
#endif

            --m_depth;

            // If the edge flips the players, flip the reward
            if (m_edge_stack[m_depth]->player != player) {
                player = ~player;
                reward = 1.0 - reward;
            }

            m_edge_stack[m_depth]->total_val += reward;
            ++m_edge_stack[m_depth]->n_visits;

#ifdef DEBUG_BACKPROPAGATION
            std::cerr << "\nedge with player " << m_edge_stack[m_depth]->player << ':'
                      << "\ntotal_val: " << m_edge_stack[m_depth]->total_val - reward
                      << " --> " << m_edge_stack[m_depth]->total_val
                      << "\nn_visits: " << m_edge_stack[m_depth]->n_visits - 1
                      << " --> " << m_edge_stack[m_depth]->n_visits
                      << std::endl;
#endif
        }
    }

    edge_pointer parent()
    {
        if (m_depth == 0)
            return nullptr;

        return m_edge_stack[m_depth - 1];
    }

    std::vector<ActionT> traceback() const
    {
        std::vector<ActionT> ret;
        std::transform(m_edge_stack.begin(),
            m_edge_stack.begin() + m_depth,
            std::back_inserter(ret),
            [](auto* e) {
                return e->action;
            });
        return ret;
    }
    size_t size() const
    {
        return m_table.size();
    }
    int depth() const
    {
        return m_depth;
    }
    void reserve(size_t sz)
    {
        m_table.reserve(sz);
    }

    friend std::ostream& operator<<<>(std::ostream&, const MctsTree<StateT, ActionT, MAX_DEPTH>&);

private:
    using LookupTable = typename std::unordered_map<key_type, Node>;
    using TraversalStack = std::array<edge_pointer, MAX_DEPTH>;

    LookupTable m_table;
    TraversalStack m_edge_stack;
    size_t m_depth;
    Node* p_root;

    std::string display(const Edge&) const;
    std::string display(const Node&) const;
};

template <
    typename StateT,
    typename ActionT,
    size_t MAX_DEPTH>
MctsTree<StateT,
    ActionT,
    MAX_DEPTH>::MctsTree(key_type key)
    : m_table()
    , m_edge_stack {}
    , m_depth {}
    , p_root(get_node(key))
{
}

template <typename StateT,
    typename ActionT,
    size_t MAX_DEPTH>
std::string MctsTree<StateT, ActionT, MAX_DEPTH>::display(
    const typename MctsTree<StateT, ActionT, MAX_DEPTH>::Edge& _edge) const
{
    std::stringstream ss;
    ss << std::setprecision(2)
       << "{\"Action\": \"" << _edge.action << "\", "
       << "\"Player\": \"" << (_edge.player ? "p1" : "p2") << "\", "
       << "\"avg_val\": \"" << _edge.total_val / (_edge.n_visits + 1.0) << "\", "
       << "\"n_visits\": \"" << _edge.n_visits << "\"}";
    return ss.str();
}

template <typename StateT,
    typename ActionT,
    size_t MAX_DEPTH>
std::string MctsTree<StateT, ActionT, MAX_DEPTH>::display(
    const typename MctsTree<StateT, ActionT, MAX_DEPTH>::Node& _node) const
{
    std::stringstream ss;
    ss << "\"key\": \"" << std::to_string(_node.key) << "\", "
       << "\"n_visits\": \"" << _node.n_visits << '\"';

    // std::copy(_node.children.begin(), _node.children.end(),
    //           std::ostream_iterator<typename MctsTree<StateT, ActionT, MAX_DEPTH>::Node>{ss, &sep });
    // std::transform(_node.children.begin(), _node.children.end(),
    //                std::ostream_iterator<typename MctsTree<StateT, ActionT, MAX_DEPTH>::Node>{_out, ','},
    //                [&](const auto& c){ return display(c); });
    return ss.str();
}

template <typename StateT, typename ActionT, size_t MAX_DEPTH>
std::ostream& operator<<(std::ostream& _out, const MctsTree<StateT, ActionT, MAX_DEPTH>& tree)
{
    for (const auto& [key, node] : tree.m_table) {
        _out << "{\n"
             << tree.display(node)
             << "\"children\": [";
        if (!node.children.empty()) {
            for (auto it = node.children.begin();
                 it != node.children.end() - 1;
                 ++it) {
                _out << tree.display(*it) << ", ";
            }
            _out << tree.display(node.children.back());
        }
        _out << "]\n},\n";
    }

    return _out;
}

} // namespace mcts

#endif
