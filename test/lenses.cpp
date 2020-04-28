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

#include <catch.hpp>

#include <zug/compose.hpp>
#include <immer/vector.hpp>
#include <zug/util.hpp>

#include <lager/lenses.hpp>
#include <lager/lenses/at.hpp>
#include <lager/lenses/attr.hpp>
#include <lager/lenses/optional.hpp>
#include <lager/lenses/variant.hpp>

struct yearday
{
    int day;
    int month;
};

struct person
{
    yearday birthday;
    std::string name;
    std::vector<std::string> things {};
};

using namespace lager;
using namespace lager::lenses;
using namespace zug;

TEST_CASE("lenses, minimal example")
{
    auto month = zug::comp([](auto&& f) {
        return [=](auto&& p) {
            return f(p.month)([&](auto&& x) {
                auto r  = std::forward<decltype(p)>(p);
                r.month = x;
                return r;
            });
        };
    });

    auto birthday = zug::comp([](auto&& f) {
        return [=](auto&& p) {
            return f(p.birthday)([&](auto&& x) {
                auto r     = std::forward<decltype(p)>(p);
                r.birthday = x;
                return r;
            });
        };
    });

    auto name = zug::comp([](auto&& f) {
        return [=](auto&& p) {
            return f(p.name)([&](auto&& x) {
                auto r = std::forward<decltype(p)>(p);
                r.name = x;
                return r;
            });
        };
    });

    auto birthday_month = birthday | month;

    auto p1 = person{{5, 4}, "juanpe"};
    CHECK(view(name, p1) == "juanpe");
    CHECK(view(birthday_month, p1) == 4);

    auto p2 = set(birthday_month, p1, 6);
    CHECK(p2.birthday.month == 6);
    CHECK(view(birthday_month, p2) == 6);

    auto p3 = over(birthday_month, p1, [](auto x) { return --x; });
    CHECK(view(birthday_month, p3) == 3);
    CHECK(p3.birthday.month == 3);
}

TEST_CASE("lenses, attr")
{
    auto name           = attr(&person::name);
    auto birthday_month = attr(&person::birthday) | attr(&yearday::month);

    auto p1 = person{{5, 4}, "juanpe"};
    CHECK(view(name, p1) == "juanpe");
    CHECK(view(birthday_month, p1) == 4);

    auto p2 = set(birthday_month, p1, 6);
    CHECK(p2.birthday.month == 6);
    CHECK(view(birthday_month, p2) == 6);

    auto p3 = over(birthday_month, p1, [](auto x) { return --x; });
    CHECK(view(birthday_month, p3) == 3);
    CHECK(p3.birthday.month == 3);
}

TEST_CASE("lenses, attr, references")
{
    auto name           = attr(&person::name);
    auto birthday_month = attr(&person::birthday) | attr(&yearday::month);

    auto p1       = person{{5, 4}, "juanpe", {{"foo"}, {"bar"}}};
    const auto p2 = p1;

    CHECK(&view(name, p1) == &p1.name);
    CHECK(&view(birthday_month, p1) == &p1.birthday.month);
    CHECK(&view(name, p2) == &p2.name);
    CHECK(&view(birthday_month, p2) == &p2.birthday.month);

    {
        [[maybe_unused]] int& x       = view(birthday_month, p1);
        [[maybe_unused]] int&& y      = view(birthday_month, std::move(p1));
        [[maybe_unused]] const int& z = view(birthday_month, p2);
    }
}

TEST_CASE("lenses, at")
{
    auto first      = at(0);
    auto first_name = first | with_opt(attr(&person::name));

    auto v1 = std::vector<person>{};
    CHECK(view(first_name, v1) == std::nullopt);
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == std::nullopt);

    v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

// This is an alternative definition of lager::lenses::attr using
// lager::lens::getset.  The standard definition is potentially more efficient
// whene the whole lens can not be optimized away, because there is only one
// capture of member, as opposed to two.  However, getset is still an
// interesting device, since it provides an easier way to define lenses for
// people not used to the pattern.
template <typename Member>
auto attr2(Member member)
{
    return getset(
        [=](auto&& x) -> decltype(auto) {
            return std::forward<decltype(x)>(x).*member;
        },
        [=](auto x, auto&& v) {
            x.*member = std::forward<decltype(v)>(v);
            return x;
        });
};

TEST_CASE("lenses, attr2")
{
    auto name = attr2(&person::name);
    auto birthday_month = attr2(&person::birthday) | attr2(&yearday::month);

    auto p1 = person{{5, 4}, "juanpe"};
    CHECK(view(name, p1) == "juanpe");
    CHECK(view(birthday_month, p1) == 4);

    auto p2 = set(birthday_month, p1, 6);
    CHECK(p2.birthday.month == 6);
    CHECK(view(birthday_month, p2) == 6);

    auto p3 = over(birthday_month, p1, [](auto x) { return --x; });
    CHECK(view(birthday_month, p3) == 3);
    CHECK(p3.birthday.month == 3);
}

TEST_CASE("lenses, attr2, references")
{
    auto name = attr2(&person::name);
    auto birthday_month = attr2(&person::birthday) | attr2(&yearday::month);

    auto p1       = person{{5, 4}, "juanpe", {{"foo"}, {"bar"}}};
    const auto p2 = p1;

    CHECK(&view(name, p1) == &p1.name);
    CHECK(&view(birthday_month, p1) == &p1.birthday.month);
    CHECK(&view(name, p2) == &p2.name);
    CHECK(&view(birthday_month, p2) == &p2.birthday.month);

    {
        [[maybe_unused]] int& x       = view(birthday_month, p1);
        [[maybe_unused]] int&& y      = view(birthday_month, std::move(p1));
        [[maybe_unused]] const int& z = view(birthday_month, p2);
    }
}

TEST_CASE("lenses, at immutable index")
{
    auto first      = at(0);
    auto first_name = first | with_opt(attr(&person::name));

    auto v1 = immer::vector<person>{};
    CHECK(view(first_name, v1) == std::nullopt);
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == std::nullopt);
    CHECK(view(first_name, set(first_name, v1, "bar")) == std::nullopt);

    v1 = v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, value_or")
{
    auto first      = at(0);
    auto first_name = first | with_opt(attr(&person::name)) | value_or("NULL");

    auto v1 = immer::vector<person>{};
    CHECK(view(first_name, v1) == "NULL");
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == "NULL");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "NULL");

    v1 = v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, alternative")
{
    auto the_person  = alternative<person>;
    auto person_name = the_person | with_opt(attr(&person::name)) | value_or("NULL");

    auto v1 = std::variant<person, std::string>{"nonesuch"};
    CHECK(view(person_name, v1) == "NULL");
    CHECK(view(person_name, set(alternative<person>, v1, person{{}, "foo"})) == "NULL");
    CHECK(view(person_name, set(person_name, v1, "bar")) == "NULL");

    v1 = person{{}, "foo"};
    CHECK(view(person_name, v1) == "foo");
    CHECK(view(person_name, set(alternative<person>, v1, person{{}, "bar"})) == "bar");
    CHECK(view(person_name, set(person_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, with_opt")
{
    auto first          = at(0);
    auto birthday       = attr(&person::birthday);
    auto month          = attr(&yearday::month);
    auto birthday_month = birthday | month;

    SECTION("lifting composed lenses") {
        auto first_month = first
                | with_opt(birthday_month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }

    SECTION("composing lifted lenses") {
        auto first_month = first
                | with_opt(birthday)
                | with_opt(month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }
}

TEST_CASE("lenses, map_opt")
{
    auto first          = at(0);
    auto birthday       = attr(&person::birthday);
    auto month          = attr(&yearday::month);
    auto birthday_month = birthday | month;

    SECTION("mapping composed lenses") {
        auto first_month = first
                | map_opt(birthday_month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }

    SECTION("composing mapped lenses") {
        auto first_month = first
                | map_opt(birthday)
                | map_opt(month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }
}

TEST_CASE("lenses, bind_opt")
{
    [[maybe_unused]] auto wrong = bind_opt(attr(&person::name));
    std::optional<person> p1 = person{{5, 4}, "juanpe"};
    // CHECK(view(wrong, p1) == "juanpe"); // should not compile

    SECTION("composing bound lenses") {
        auto first       = bind_opt(at(0));
        auto first_first = first | first;

        std::optional<std::vector<std::vector<int>>> v1;

        v1 = {};
        CHECK(view(first, v1) == std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first, set(first_first, v1, 256)) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{std::vector<int>{}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{{42}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == 42);
        CHECK(view(first_first, set(first_first, v1, 256)) == 256);
    }

    SECTION("binding composed bound lenses") {
        auto raw_first   = at(0);
        auto first       = bind_opt(at(0));
        auto first_first = bind_opt(raw_first | first);

        std::optional<std::vector<std::vector<int>>> v1;

        v1 = {};
        CHECK(view(first, v1) == std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first, set(first_first, v1, 256)) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{std::vector<int>{}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{{42}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == 42);
        CHECK(view(first_first, set(first_first, v1, 256)) == 256);
    }
}
