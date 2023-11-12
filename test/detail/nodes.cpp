//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include <catch2/catch.hpp>

#include "../spies.hpp"

#include <lager/sensor.hpp>
#include <lager/state.hpp>
#include <lager/with.hpp>

#include <lager/detail/traversal_dfs.hpp>
#include <lager/detail/traversal_topo_intrusive.hpp>

#include <zug/transducer/map.hpp>

#include <array>

using namespace zug;

using namespace lager::detail;

TEST_CASE("node, instantiate down node")
{
    make_xform_reader_node(identity, {});
}

TEST_CASE("node, instantiate state") { make_state_node(0); }

TEST_CASE("node, last_value is not visible")
{
    auto x = make_state_node(0);
    x->send_up(12);
    CHECK(0 == x->last());
    x->send_up(42);
    CHECK(0 == x->last());
}

TEMPLATE_TEST_CASE("node, send_down with traversal strategies",
                   "[dfs][topo intrusive]",
                   dfs_traversal,
                   topo_traversal_set)
{
    SECTION("node, last value becomes visible")
    {
        auto x = make_state_node(0);

        x->send_up(12);
        TestType(x).visit();
        CHECK(12 == x->last());

        x->send_up(42);
        TestType(x).visit();
        CHECK(42 == x->last());
    }

    SECTION("node, sending down")
    {
        auto x = make_state_node(5);
        auto y = make_xform_reader_node(identity, std::make_tuple(x));
        CHECK(5 == y->last());

        x->send_up(12);
        TestType(x).visit();
        CHECK(12 == y->last());

        x->send_up(42);
        TestType(x).visit();
        CHECK(42 == y->last());
    }

    SECTION("node, notifies new and previous value after send down")
    {
        auto x = make_state_node(5);
        auto s = testing::spy([](int next) { CHECK(42 == next); });
        auto c = x->observers().connect(s);

        x->send_up(42);
        CHECK(0 == s.count());

        x->notify();
        CHECK(0 == s.count());

        TestType(x).visit();
        x->notify();
        CHECK(1 == s.count());
    }

    SECTION("node, lifetime of observer")
    {
        auto x = make_state_node(5);
        auto s = testing::spy();

        using signal_t = typename decltype(x)::element_type::signal_type;
        using slot_t   = signal_t::slot<decltype(s)>;

        auto c = slot_t{s};
        {
            auto y = make_xform_reader_node(identity, std::make_tuple(x));
            y->observers().add(c);
            CHECK(c.is_linked());

            x->push_down(56);
            TestType(x).visit();
            x->notify();
            CHECK(1 == s.count());
        }
        CHECK(!c.is_linked());

        x->push_down(26);
        TestType(x).visit();
        x->notify();
        CHECK(1 == s.count());
    }

    SECTION("node, notify idempotence")
    {
        auto x = make_state_node(5);
        auto s = testing::spy();
        auto c = x->observers().connect(s);

        x->send_up(42);
        CHECK(0 == s.count());

        x->notify();
        x->notify();
        x->notify();
        CHECK(0 == s.count());

        TestType(x).visit();
        x->notify();
        x->notify();
        x->notify();
        CHECK(1 == s.count());
    }

    SECTION("node, observing is consistent")
    {
        auto x = make_state_node(5);
        auto y = make_xform_reader_node(identity, std::make_tuple(x));
        auto z = make_xform_reader_node(identity, std::make_tuple(x));
        auto w = make_xform_reader_node(identity, std::make_tuple(y));

        auto s = testing::spy([&](int new_value) {
            CHECK(42 == new_value);
            CHECK(42 == x->last());
            CHECK(42 == y->last());
            CHECK(42 == z->last());
            CHECK(42 == w->last());
        });

        auto xc = x->observers().connect(s);
        auto yc = y->observers().connect(s);
        auto zc = z->observers().connect(s);
        auto wc = w->observers().connect(s);

        x->send_up(42);
        TestType(x).visit();
        CHECK(0 == s.count());

        x->notify();
        CHECK(4 == s.count());
    }

    SECTION("node, bidirectional node sends values up")
    {
        auto x = make_state_node(5);
        auto y = make_xform_cursor_node(identity, identity, std::make_tuple(x));

        y->send_up(42);
        CHECK(5 == x->last());
        CHECK(5 == y->last());

        TestType(x).visit();
        CHECK(42 == x->last());
        CHECK(42 == y->last());
    }

    SECTION("node, bidirectional mapping")
    {
        auto inc = [](int x) { return ++x; };
        auto dec = [](int x) { return --x; };
        auto x   = make_state_node(5);
        auto y = make_xform_cursor_node(map(inc), map(dec), std::make_tuple(x));

        CHECK(5 == x->last());
        CHECK(6 == y->last());

        y->send_up(42);
        TestType(x).visit();
        CHECK(41 == x->last());
        CHECK(42 == y->last());

        x->send_up(42);
        TestType(x).visit();
        CHECK(42 == x->last());
        CHECK(43 == y->last());
    }

    SECTION("node, bidirectiona update is consistent")
    {
        using arr = std::array<int, 2>;
        auto x    = make_state_node(arr{{5, 13}});
        auto y = make_xform_cursor_node(map([](const arr& a) { return a[0]; }),
                                        lager::update([](arr a, int v) {
                                            a[0] = v;
                                            return a;
                                        }),
                                        std::make_tuple(x));
        auto z = make_xform_cursor_node(map([](const arr& a) { return a[1]; }),
                                        lager::update([](arr a, int v) {
                                            a[1] = v;
                                            return a;
                                        }),
                                        std::make_tuple(x));

        CHECK((arr{{5, 13}}) == x->last());
        CHECK(5 == y->last());
        CHECK(13 == z->last());

        z->send_up(42);
        y->send_up(69);
        CHECK((arr{{5, 13}}) == x->last());
        CHECK(5 == y->last());
        CHECK(13 == z->last());

        TestType(x).visit();
        CHECK((arr{{69, 42}}) == x->last());
        CHECK(69 == y->last());
        CHECK(42 == z->last());
    }

    SECTION("node, sensors nodes reevaluate on send down")
    {
        int count = 0;
        auto x    = make_sensor_node([&count] { return count++; });
        CHECK(0 == x->last());
        TestType(x).visit();
        CHECK(1 == x->last());
        TestType(x).visit();
        CHECK(2 == x->last());
    }

    SECTION("node, one node two parents")
    {
        int count = 0;
        auto x    = make_sensor_node([&count] { return count++; });
        auto y    = make_state_node(12);
        auto z = make_xform_reader_node(map([](int a, int b) { return a + b; }),
                                        std::make_tuple(x, y));
        auto s =
            testing::spy([&](int r) { CHECK(r == x->last() + y->last()); });
        auto c = z->observers().connect(s);
        CHECK(12 == z->last());

        // Commit first root individually
        TestType(x).visit();
        CHECK(13 == z->last());
        CHECK(0 == s.count());
        x->notify();
        CHECK(1 == s.count());
        y->notify();
        CHECK(1 == s.count());

        // Commit second root individually
        y->push_down(3);
        TestType(y).visit();
        CHECK(4 == z->last());
        y->notify();
        CHECK(2 == s.count());
        x->notify();
        CHECK(2 == s.count());

        // Commit both roots together
        TestType(x).visit();
        y->push_down(69);
        TestType(y).visit();
        x->notify();
        y->notify();
        CHECK(71 == z->last());
        CHECK(3 == s.count());
    }
}

TEST_CASE("node, schedule or send down")
{
    struct spy_traversal : traversal
    {
        std::vector<reader_node_base*> calls = {};
        void schedule(reader_node_base* n) override final
        {
            calls.push_back(n);
        }
    };

    auto x = make_state_node(12);
    auto y = make_xform_reader_node(lager::identity, std::make_tuple(x));
    auto z = make_xform_reader_node(map([](int a, int b) { return a + b; }),
                                    std::make_tuple(x, y));

    x->push_down(13);
    auto t = spy_traversal();
    x->schedule_or_send_down(t);

    CHECK(t.calls.size() == 2);
    CHECK(t.calls.at(0) == z.get());
    CHECK(t.calls.at(1) == z.get());
}

TEST_CASE("node, rank increments")
{
    int count = 0;
    auto x    = make_sensor_node([&count] { return count++; });
    auto y    = make_state_node(12);
    auto z    = make_xform_reader_node(map([](int a, int b) { return a + b; }),
                                    std::make_tuple(x, y));
    auto t    = make_merge_reader_node(std::make_tuple(x, z));
    auto u    = make_xform_reader_node(
        map([](auto tuple) { return std::get<0>(tuple); }), std::make_tuple(t));

    CHECK(0 == x->rank());
    CHECK(0 == y->rank());
    CHECK(1 == z->rank());
    CHECK(2 == t->rank());
    CHECK(3 == u->rank());
}

TEST_CASE("node schedule, default init")
{
    auto ns = node_schedule();

    CHECK(ns.rank_ == 0);
    CHECK(ns.nodes_.empty());
    CHECK(ns.next_ == nullptr);
    CHECK(!ns.member_hook_rb_.is_linked());
}

TEST_CASE("node schedule, schedule of next rank is unique")
{
    auto ns    = node_schedule();
    auto next1 = ns.get_or_create_next();
    auto next2 = ns.get_or_create_next();

    CHECK(ns.next_);
    CHECK(next1 == next2);
    CHECK(next1->rank_ == ns.rank_ + 1);
}

TEST_CASE("node, node schedule is unique")
{
    auto x = make_state_node(12);
    auto y = make_xform_reader_node(map(lager::identity), std::make_tuple(x));
    auto z = make_xform_reader_node(map(lager::identity), std::make_tuple(x));

    CHECK(&y->get_node_schedule() == &z->get_node_schedule());
}
