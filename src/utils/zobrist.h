/// zobrist.h
#ifndef __ZOBRIST_H_
#define __ZOBRIST_H_

#include "rand.h"
#include <array>
#include <set>

namespace zobrist {

template <typename HashFunctor, typename Key, size_t NKeys>
class KeyTable {
public:
    using const_iterator = typename std::array<Key, NKeys>::const_iterator;

    explicit KeyTable(int n_reserved_bits=0)
        : m_keys(populate_keys(n_reserved_bits))
    {
    }
    template <typename... Args>
    Key constexpr operator()(Args&&... args)
    {
        return m_keys[f_hash(std::forward<Args>(args)...)];
    };
    Key operator[](size_t n) const { return m_keys[n]; }
    size_t size() const { return NKeys; }

    const_iterator begin() const { return m_keys.begin(); }
    const_iterator end() const { return m_keys.end(); }

private:
    std::array<Key, NKeys> m_keys;
    HashFunctor f_hash {};

    std::array<Key, NKeys> populate_keys(int n_reserved_bits);
};

template <typename HashFunctor, typename Key, size_t N>
std::array<Key, N> KeyTable<HashFunctor, Key, N>::populate_keys(int n_res)
{
    std::array<Key, N> ret {};
    Rand::Util<Key> randutil {};
    std::set<Key> distinct;

    const auto min = std::numeric_limits<Key>::min();
    const auto max = std::numeric_limits<Key>::max();

    for (auto i = 0; i < N; ++i) {
        bool duplicate = true;
        Key key;
        while (duplicate)
        {
            key = randutil.get(min, max);
            duplicate = !distinct.insert(key).second;
        }
        key << n_res;
        key >> n_res;
        ret[i] = key;
    }
    return ret;
}

} // namespace zobrist

#endif
