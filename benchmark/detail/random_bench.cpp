#include <nonius.h++>

#include "random_dag.hpp"

#include "lager/detail/traversal_dfs.hpp"
#include "lager/detail/traversal_topo_intrusive.hpp"
#include "lager/detail/traversal_treap.hpp"

NONIUS_PARAM(N, std::size_t{16})
NONIUS_PARAM(E, double{0.5})
NONIUS_PARAM(M, double{0.5})

template <typename Traversal>
auto benchmark_fn()
{
    return [](nonius::chronometer meter) {
        auto n = meter.param<N>();
        auto e = meter.param<E>();
        auto m = meter.param<M>();

        auto b = magic_eight_ball();
        auto v = std::vector<rdag>(meter.runs());
        std::generate(
            v.begin(), v.end(), [&] { return make_rdag(n, m, e, b); });
        meter.measure([&](int i) {
            auto&& c = v[i];
            for (int i = 1; i < 50; ++i) {
                c.root->send_up(!c.root->last());
                auto t = Traversal(c.root, n);
                t.visit();
            }
            return c.nodes_.size();
        });
    };
};

NONIUS_BENCHMARK("RDAG-DFS", benchmark_fn<dfs_traversal>())
NONIUS_BENCHMARK("RDAG-TREAP", benchmark_fn<treap_traversal>())
NONIUS_BENCHMARK("RDAG-BIMSRB", benchmark_fn<topo_intrusive_traversal_rb>())
NONIUS_BENCHMARK("RDAG-BIMS", benchmark_fn<topo_intrusive_traversal>())
// NONIUS_BENCHMARK("rdag", [](nonius::chronometer meter) {
//     auto n = meter.param<N>();
//     auto m = magic_eight_ball();
//     meter.measure([n, &m](int) {
//         auto c = make_rdag(n, 0.2, 0.5, m);
//         if (c.nodes_.size() != n + 2)
//             throw std::runtime_error("expected " + std::to_string(n) + " got
//             " +
//                                      std::to_string(c.nodes_.size()));
//         return c.nodes_.size();
//     });
// })
