#include <nonius.h++>

#include "lager/detail/merge_nodes.hpp"
#include "lager/detail/nodes.hpp"
#include "lager/state.hpp"

#include "lager/detail/traversal.hpp"
#include "lager/detail/traversal_dfs.hpp"
#include "lager/detail/traversal_topo.hpp"
#include "lager/detail/traversal_topo_intrusive.hpp"
#include "lager/detail/traversal_topo_naive_mmap.hpp"
#include "lager/detail/traversal_treap.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <zug/transducer/map.hpp>
#include <zug/util.hpp>

#include <boost/unordered_map.hpp>
#include <chrono>
#include <unordered_map>

using namespace lager;
using namespace lager::detail;

using unique_value = long unsigned;

using node_t      = reader_node<unique_value>;
using merge_t     = reader_node<std::tuple<unique_value, unique_value>>;
using node_ptr_t  = std::shared_ptr<node_t>;
using merge_ptr_t = std::shared_ptr<merge_t>;

auto combine(const auto&... xs)
{
    auto M = std::max({xs...});
    auto S = 1;

    const auto comb = [M, &S](auto x) {
        if (x != M)
            S++;
    };

    (comb(xs), ...);

    return M + S;
}
constexpr auto inline combine_tuple = [](const auto& t) {
    return std::apply([](const auto&... xs) { return combine(xs...); }, t);
};

unique_value next(const unique_value& x) { return x + 1; }

/**
 * A chain is a series of node such that, if the value of the chain root
 * is k, and if the number of "link" in the chain is n, then the value of
 * the chain ought to be k + n.
 **/
struct chain
{
    using state_t = decltype(make_state_node(unique_value{}));

    unsigned long value() const { return last->last(); }

    state_t root    = make_state_node(unique_value{});
    node_ptr_t last = root;
};

/**
 * Node network of the simplest form:
 *
 * A - B - C - D - E - ....
 *
 * This network is well suited to a DFS. Each one of A, B, C...
 * in the diagram above is a "link" in the chain.
 **/
chain make_simple_chain(long unsigned n)
{
    auto c = chain();

    for (long unsigned i = 0; i < n; ++i) {
        auto p = std::make_tuple(c.last);
        c.last = make_xform_reader_node(zug::map(next), p);
    }

    return c;
}

/**
 * A node network having the following form:
 *
 *     B
 *    * *
 *   A   D * D'
 *    * *
 *     C
 *
 * Where:
 *
 * - A is some reader_node<int>
 * - B,C identity node forwarding A's value
 * - D is a merge node taking in B and C
 * - D' is a transform that simply increment A
 *
 * This network should be better suited to a topological traversal.
 *
 * If the value of A = 1, then the value of D' will be 2. Thus,
 * A,B,C,D,D' taken together form a "link" in the chain.
 **/
chain make_diamond_chain(long unsigned n)
{
    auto c = chain();

    for (long unsigned i = 0; i < n; ++i) {
        auto p      = std::make_tuple(c.last);
        auto xform1 = make_xform_reader_node(identity, p);
        auto xform2 = make_xform_reader_node(identity, p);
        auto merge  = make_merge_reader_node(std::make_tuple(xform1, xform2));
        c.last      = make_xform_reader_node(zug::map(combine_tuple),
                                        std::make_tuple(merge));
    }

    return c;
}

NONIUS_PARAM(N, std::size_t{16})

template <typename Traversal, typename ChainFn>
auto traversal_fn(ChainFn&& chain_fn)
{
    return [chain_fn](nonius::chronometer meter) {
        auto n = meter.param<N>();
        std::vector<chain> v(meter.runs());
        std::generate(v.begin(), v.end(), [&] { return chain_fn(n); });
        meter.measure([&](int i) {
            auto&& c = v[i];
            c.root->send_up(unique_value{1});
            Traversal t{c.root, n};
            t.visit();
            if (c.value() != n + 1)
                throw std::runtime_error{"bad stuff! expected " +
                                         std::to_string(n + 1) + " got " +
                                         std::to_string(c.value())};
            return c.value();
        });
    };
}

using std_traversal       = topo_traversal<>;
using bmultimap_traversal = topo_traversal<boost::unordered_multimap>;

NONIUS_BENCHMARK("SC-DFS", traversal_fn<dfs_traversal>(make_simple_chain))
NONIUS_BENCHMARK("SC-T-CMM",
                 traversal_fn<naive_mmap_topo_traversal>(make_simple_chain))
NONIUS_BENCHMARK("SC-T-SUMM", traversal_fn<std_traversal>(make_simple_chain))
NONIUS_BENCHMARK("SC-T-BUMM",
                 traversal_fn<bmultimap_traversal>(make_simple_chain))
NONIUS_BENCHMARK("SC-T-BIMS",
                 traversal_fn<topo_intrusive_traversal>(make_simple_chain))
NONIUS_BENCHMARK("SC-T-BIMSRB",
                 traversal_fn<topo_intrusive_traversal_rb>(make_simple_chain))
NONIUS_BENCHMARK("SC-T-TREAP", traversal_fn<treap_traversal>(make_simple_chain))

NONIUS_BENCHMARK("DC-DFS", traversal_fn<dfs_traversal>(make_diamond_chain))
NONIUS_BENCHMARK("DC-T-CMM",
                 traversal_fn<naive_mmap_topo_traversal>(make_diamond_chain))
NONIUS_BENCHMARK("DC-T-SUMM", traversal_fn<std_traversal>(make_diamond_chain))
NONIUS_BENCHMARK("DC-T-BUMM",
                 traversal_fn<bmultimap_traversal>(make_diamond_chain))
NONIUS_BENCHMARK("DC-T-BIMS",
                 traversal_fn<topo_intrusive_traversal>(make_diamond_chain))
NONIUS_BENCHMARK("DC-T-BIMSRB",
                 traversal_fn<topo_intrusive_traversal_rb>(make_diamond_chain))
NONIUS_BENCHMARK("DC-T-TREAP",
                 traversal_fn<treap_traversal>(make_diamond_chain))
