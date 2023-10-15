//
// lager - library for functional interactive c++ programs
// Copyright (C) 2023 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include <catch2/catch.hpp>

#include <lager/detail/nodes.hpp>
#include <lager/detail/traversal_topo_intrusive.hpp>
#include <lager/state.hpp>
#include <lager/with.hpp>
#include <zug/transducer/map.hpp>

using namespace lager::detail;
using namespace zug;

constexpr auto inline increment = [](auto x) { return x + 1; };
constexpr auto inline sum       = [](auto... xs) { return (xs + ...); };

TEST_CASE("traversal, schedule")
{
    auto x = make_state_node(10);
    auto y = make_xform_reader_node(map(increment), std::make_tuple(x));
    auto z = make_xform_reader_node(map(sum), std::make_tuple(x, y));
    auto u = make_xform_reader_node(map(sum), std::make_tuple(x, y));

    x->push_down(11);
    auto t = topo_traversal_set(x);
    CHECK(x->member_hook_list_.is_linked());

    t.schedule(y.get());
    CHECK(y->member_hook_list_.is_linked());
    CHECK(y->get_node_schedule().nodes_.size() == 1);
    CHECK(y->get_node_schedule().member_hook_rb_.is_linked());

    t.schedule(z.get());
    t.schedule(u.get());
    const auto& ns = z->get_node_schedule();
    CHECK(ns.member_hook_rb_.is_linked());
    CHECK(ns.nodes_.size() == 2);
    CHECK(z->member_hook_list_.is_linked());
    CHECK(u->member_hook_list_.is_linked());
}

TEST_CASE("merged transform, visit once")
{
    using model_t = std::pair<int, int>;
    auto tr_count = 0;
    auto state    = lager::make_state(model_t{}, lager::transactional_tag{});
    auto ca       = state[&model_t::first].make();
    auto cb       = state[&model_t::second].make();
    auto m        = lager::with(cb, ca).make();
    auto tr       = m.map([&tr_count](const auto& tuple) {
                   auto [b, a] = tuple;
                   CHECK(a <= b);
                   tr_count++;
                   return std::sqrt(b - a);
               }).make();
    tr_count      = 0;

    ca.set(11);
    cb.set(21);
    lager::commit(state);

    CHECK(tr_count == 1);
}
