#include <nonius.h++>

#include "random-dag.hpp"

#include "lager/detail/traversal_dfs.hpp"
#include "lager/detail/traversal_topo_intrusive.hpp"

NONIUS_PARAM(N, std::size_t{16})
NONIUS_PARAM(E, double{0.5})
NONIUS_PARAM(M, double{0.5})

template <typename Traversal>
auto benchmark_fn(double m)
{
    return [m](nonius::chronometer meter) {
        auto n = meter.param<N>();
        auto e = meter.param<E>();

        const auto b = magic_eight_ball();
        auto v       = std::vector<rdag>(meter.runs());
        std::generate(
            v.begin(), v.end(), [&] { return make_rdag(n, m, e, b); });
        meter.measure([&](int i) {
            auto& c = v[i];
            for (auto i = 0; i < 50; ++i) {
                c.root->send_up(!c.root->last());
                auto t = Traversal(c.root);
                t.visit();
            }
            return c.nodes_.size();
        });
    };
};

NONIUS_BENCHMARK("RDAG-DFS", benchmark_fn<dfs_traversal>(0.0))
NONIUS_BENCHMARK("RDAG-BIMSRB", benchmark_fn<topo_intrusive_traversal_rb>(0.0))
NONIUS_BENCHMARK("RDAG-RANK-OBJ", benchmark_fn<topo_traversal_set>(0.0))

NONIUS_BENCHMARK("RDAG-DFS-0.05", benchmark_fn<dfs_traversal>(0.05))
NONIUS_BENCHMARK("RDAG-BIMSRB-0.05",
                 benchmark_fn<topo_intrusive_traversal_rb>(0.05))
NONIUS_BENCHMARK("RDAG-RANK-OBJ-0.05", benchmark_fn<topo_traversal_set>(0.05))

NONIUS_BENCHMARK("RDAG-DFS-0.2", benchmark_fn<dfs_traversal>(0.2))
NONIUS_BENCHMARK("RDAG-BIMSRB-0.2",
                 benchmark_fn<topo_intrusive_traversal_rb>(0.2))
NONIUS_BENCHMARK("RDAG-RANK-OBJ-0.2", benchmark_fn<topo_traversal_set>(0.2))

NONIUS_BENCHMARK("RDAG-DFS-0.5", benchmark_fn<dfs_traversal>(0.5))
NONIUS_BENCHMARK("RDAG-BIMSRB-0.5",
                 benchmark_fn<topo_intrusive_traversal_rb>(0.5))
NONIUS_BENCHMARK("RDAG-RANK-OBJ-0.5", benchmark_fn<topo_traversal_set>(0.5))

NONIUS_BENCHMARK("RDAG-DFS-0.8", benchmark_fn<dfs_traversal>(0.8))
NONIUS_BENCHMARK("RDAG-BIMSRB-0.8",
                 benchmark_fn<topo_intrusive_traversal_rb>(0.8))
NONIUS_BENCHMARK("RDAG-RANK-OBJ-0.8", benchmark_fn<topo_traversal_set>(0.8))
