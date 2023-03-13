#pragma once

#include "lager/detail/merge_nodes.hpp"
#include "lager/detail/nodes.hpp"
#include "lager/state.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

using namespace lager;
using namespace lager::detail;

struct magic_eight_ball
{
    mutable std::random_device rd_{};
    mutable std::mt19937 gen_{rd_()};
    mutable std::uniform_int_distribution<int> int_dist_{0, 1};
    mutable std::uniform_real_distribution<double> uniform_dist_{0, 1};

    auto choice(int n) const
    {
        return int_dist_(
            gen_,
            typename std::uniform_int_distribution<int>::param_type{0, n});
    }

    auto choice(auto&& begin, auto&& end) const
    {
        return std::next(begin, this->choice(std::distance(begin, end) - 1));
    }

    auto choice(auto& v) const { return choice(v.begin(), v.end()); }

    auto choice_except(auto& v, auto& except, int tries = 20) const
    {
        for (auto i = 0, p = choice(v); i < tries; ++i, p = choice(v)) {
            if (p != except)
                return p;
        }

        throw std::runtime_error("failed to chose");
    }

    bool maybe_yes(double please_say_yes = 0.5) const
    {
        return uniform_dist_(gen_) < please_say_yes;
    }

    bool draw(double entropy) const { return maybe_yes(entropy / 2.0); }
};

/**
 *
 **/
using boolean_node = bool;

using node_t      = reader_node<boolean_node>;
using merge_t     = reader_node<std::tuple<boolean_node, boolean_node>>;
using node_ptr_t  = std::shared_ptr<node_t>;
using merge_ptr_t = std::shared_ptr<merge_t>;

struct rdag
{
    using state_t = decltype(make_state_node(false));

    state_t root                   = make_state_node(false);
    std::vector<node_ptr_t> nodes_ = {
        make_xform_reader_node(identity, std::make_tuple(root)),
        make_xform_reader_node(identity, std::make_tuple(root))};
};

auto make_update_fn(double entropy, const magic_eight_ball& magic_ball)
{
    return zug::map([entropy, &magic_ball](const auto&) {
        return magic_ball.draw(entropy);
    });
}

void make_node(double entropy, const magic_eight_ball& magic_ball, rdag& d)
{
    auto parent = magic_ball.choice(d.nodes_);
    d.nodes_.push_back(make_xform_reader_node(
        make_update_fn(entropy, magic_ball), std::make_tuple(*parent)));
}

void make_merged_node(double entropy,
                      const magic_eight_ball& magic_ball,
                      rdag& d)
{
    auto father = magic_ball.choice(d.nodes_);
    auto mother = magic_ball.choice_except(d.nodes_, father);

    auto embryo = make_merge_reader_node(std::make_tuple(*father, *mother));
    auto child  = make_xform_reader_node(make_update_fn(entropy, magic_ball),
                                        std::make_tuple(embryo));

    d.nodes_.push_back(child);
}

/*!
 * @param merge_node_factor Proportion of node being merge node
 *
 * @param entropy
 *        Intuitively, the probability that the value of a
 *        particular node will change if one of it's parent's
 *        value has changed
 *
 * @param magic_ball Shake it and get some random answer
 */
rdag make_rdag(int node_count,
               double merge_node_factor,
               double entropy,
               const magic_eight_ball& magic_ball)
{
    auto d = rdag{};

    for (auto i = 0; i < node_count; ++i) {
        if (magic_ball.maybe_yes(merge_node_factor)) {
            make_merged_node(entropy, magic_ball, d);
        } else {
            make_node(entropy, magic_ball, d);
        }
    }

    return d;
}
