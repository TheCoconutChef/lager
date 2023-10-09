#include <nonius.h++>

#include "random-dag.hpp"

NONIUS_PARAM(N, std::size_t{16})
NONIUS_PARAM(E, double{0.5})
NONIUS_PARAM(M, double{0.5})

auto init_benchmark_fn()
{
    return [](nonius::chronometer meter) {
        auto n = meter.param<N>();
        auto e = meter.param<E>();
        auto m = meter.param<M>();

        auto b = magic_eight_ball();

        meter.measure([&](int) {
            auto v = std::vector<rdag>(10);
            std::generate(
                v.begin(), v.end(), [&] { return make_rdag(n, m, e, b); });
            return v.size();
        });
    };
};

NONIUS_BENCHMARK("init", init_benchmark_fn())
