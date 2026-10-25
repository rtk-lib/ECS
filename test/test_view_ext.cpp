#include <criterion/criterion.h>
#include <cstddef>
#include <set>
#include <vector>
#include <algorithm>
#include "Registry.hpp"
#include "View.hpp"

using namespace rtk::ecs;

namespace {
    struct VPos {
        float x;
        float y;
    };

    struct VVel {
        float dx;
        float dy;
    };

    struct VHp {
        int hp;
    };

    struct VTag {};

    struct VExtra {
        int v;
    };

    template <typename... Cs>
    std::vector<std::size_t> collect(Registry &reg)
    {
        std::vector<std::size_t> out;
        for (auto e : reg.view<Cs...>())
            out.push_back(e);
        return out;
    }

    template <typename... Cs>
    std::set<std::size_t> collect_set(Registry &reg)
    {
        auto v = collect<Cs...>(reg);
        return std::set<std::size_t>(v.begin(), v.end());
    }

    void register_all(Registry &reg)
    {
        reg.register_component<VPos>();
        reg.register_component<VVel>();
        reg.register_component<VHp>();
        reg.register_component<VTag>();
        reg.register_component<VExtra>();
    }
}

Test(view_ext, begin_equals_end_on_empty_pool)
{
    Registry reg;
    reg.register_component<VPos>();
    auto view = reg.view<VPos>();
    cr_assert_not(view.begin() != view.end());
}

Test(view_ext, begin_differs_from_end_when_populated)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    auto view = reg.view<VPos>();
    cr_assert(view.begin() != view.end());
}

Test(view_ext, deref_begin_gives_first_entity)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.get_components<VPos>().insert_at(4, VPos{});
    auto view = reg.view<VPos>();
    cr_assert_eq(*view.begin(), 4u);
}

Test(view_ext, increment_reaches_end)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.get_components<VPos>().insert_at(4, VPos{});
    auto view = reg.view<VPos>();
    auto it = view.begin();
    ++it;
    cr_assert_not(it != view.end());
}

Test(view_ext, increment_returns_self)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.get_components<VPos>().insert_at(1, VPos{});
    reg.get_components<VPos>().insert_at(2, VPos{});
    auto view = reg.view<VPos>();
    auto it = view.begin();
    auto &ref = ++it;
    cr_assert_eq(&ref, &it);
    cr_assert_eq(*it, 2u);
}

Test(view_ext, begin_skips_leading_non_matching_entities)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    reg.get_components<VPos>().insert_at(1, VPos{});
    reg.get_components<VPos>().insert_at(2, VPos{});
    reg.get_components<VVel>().insert_at(2, VVel{});
    auto view = reg.view<VPos, VVel>();
    cr_assert_eq(*view.begin(), 2u);
}

Test(view_ext, increment_skips_middle_non_matching_entities)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    for (std::size_t i = 0; i < 5; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    reg.get_components<VVel>().insert_at(0, VVel{});
    reg.get_components<VVel>().insert_at(4, VVel{});
    auto view = reg.view<VPos, VVel>();
    auto it = view.begin();
    cr_assert_eq(*it, 0u);
    ++it;
    cr_assert_eq(*it, 4u);
    ++it;
    cr_assert_not(it != view.end());
}

Test(view_ext, trailing_non_matching_entities_lead_to_end)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    reg.get_components<VVel>().insert_at(0, VVel{});
    reg.get_components<VPos>().insert_at(1, VPos{});
    reg.get_components<VPos>().insert_at(2, VPos{});
    cr_assert_eq((collect<VPos, VVel>(reg).size()), 1u);
}

Test(view_ext, view_constructed_manually)
{
    SparseArray<VPos> pos;
    SparseArray<VVel> vel;
    pos.insert_at(1, VPos{});
    pos.insert_at(2, VPos{});
    vel.insert_at(2, VVel{});
    View<VPos, VVel> view(pos.get_packed_array(), vel);
    std::vector<std::size_t> out;
    for (auto e : view)
        out.push_back(e);
    cr_assert_eq(out.size(), 1u);
    cr_assert_eq(out[0], 2u);
}

Test(view_ext, single_view_constructed_manually)
{
    SparseArray<VPos> pos;
    pos.insert_at(3, VPos{});
    pos.insert_at(9, VPos{});
    View<VPos> view(pos.get_packed_array());
    std::size_t count = 0;
    for (auto e : view) {
        (void)e;
        ++count;
    }
    cr_assert_eq(count, 2u);
}

Test(view_ext, single_view_follows_insertion_order)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.get_components<VPos>().insert_at(5, VPos{});
    reg.get_components<VPos>().insert_at(2, VPos{});
    reg.get_components<VPos>().insert_at(8, VPos{});
    auto out = collect<VPos>(reg);
    std::vector<std::size_t> expected{5, 2, 8};
    cr_assert(out == expected);
}

Test(view_ext, single_view_order_after_swap_and_pop)
{
    Registry reg;
    reg.register_component<VPos>();
    for (std::size_t i = 0; i < 4; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    reg.get_components<VPos>().erase(0);
    auto out = collect<VPos>(reg);
    std::vector<std::size_t> expected{3, 1, 2};
    cr_assert(out == expected);
}

Test(view_ext, multi_view_follows_driver_order)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    reg.get_components<VPos>().insert_at(3, VPos{});
    reg.get_components<VPos>().insert_at(1, VPos{});
    reg.get_components<VVel>().insert_at(1, VVel{});
    reg.get_components<VVel>().insert_at(3, VVel{});
    auto pv = collect<VPos, VVel>(reg);
    auto vp = collect<VVel, VPos>(reg);
    cr_assert(pv == (std::vector<std::size_t>{3, 1}));
    cr_assert(vp == (std::vector<std::size_t>{1, 3}));
}

Test(view_ext, driver_choice_does_not_change_result_set)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    for (std::size_t i = 0; i < 30; ++i) {
        reg.get_components<VPos>().insert_at(i, VPos{});
        if (i % 3 == 0)
            reg.get_components<VVel>().insert_at(i, VVel{});
    }
    cr_assert((collect_set<VPos, VVel>(reg)) == (collect_set<VVel, VPos>(reg)));
}

Test(view_ext, three_component_permutations_same_set)
{
    Registry reg;
    register_all(reg);
    for (std::size_t i = 0; i < 40; ++i) {
        if (i % 2 == 0)
            reg.get_components<VPos>().insert_at(i, VPos{});
        if (i % 3 == 0)
            reg.get_components<VVel>().insert_at(i, VVel{});
        if (i % 5 == 0)
            reg.get_components<VHp>().insert_at(i, VHp{});
    }
    auto ref = collect_set<VPos, VVel, VHp>(reg);
    cr_assert(ref == (std::set<std::size_t>{0, 30}));
    cr_assert(ref == (collect_set<VPos, VHp, VVel>(reg)));
    cr_assert(ref == (collect_set<VVel, VPos, VHp>(reg)));
    cr_assert(ref == (collect_set<VVel, VHp, VPos>(reg)));
    cr_assert(ref == (collect_set<VHp, VPos, VVel>(reg)));
    cr_assert(ref == (collect_set<VHp, VVel, VPos>(reg)));
}

Test(view_ext, empty_driver_with_full_others_yields_nothing)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    for (std::size_t i = 0; i < 10; ++i)
        reg.get_components<VVel>().insert_at(i, VVel{});
    cr_assert((collect<VPos, VVel>(reg).empty()));
}

Test(view_ext, full_driver_with_empty_other_yields_nothing)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    for (std::size_t i = 0; i < 10; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    cr_assert((collect<VPos, VVel>(reg).empty()));
}

Test(view_ext, disjoint_sets_yield_nothing)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    for (std::size_t i = 0; i < 10; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    for (std::size_t i = 10; i < 20; ++i)
        reg.get_components<VVel>().insert_at(i, VVel{});
    cr_assert((collect<VPos, VVel>(reg).empty()));
}

Test(view_ext, four_component_view)
{
    Registry reg;
    register_all(reg);
    for (std::size_t i = 0; i < 8; ++i) {
        reg.get_components<VPos>().insert_at(i, VPos{});
        reg.get_components<VVel>().insert_at(i, VVel{});
        reg.get_components<VHp>().insert_at(i, VHp{});
    }
    reg.get_components<VTag>().insert_at(6, VTag{});
    auto out = collect<VPos, VVel, VHp, VTag>(reg);
    cr_assert_eq(out.size(), 1u);
    cr_assert_eq(out[0], 6u);
}

Test(view_ext, five_component_view)
{
    Registry reg;
    register_all(reg);
    std::size_t e = reg.spawn_entity();
    reg.get_components<VPos>().insert_at(e, VPos{});
    reg.get_components<VVel>().insert_at(e, VVel{});
    reg.get_components<VHp>().insert_at(e, VHp{});
    reg.get_components<VTag>().insert_at(e, VTag{});
    reg.get_components<VExtra>().insert_at(e, VExtra{});
    std::size_t other = reg.spawn_entity();
    reg.get_components<VPos>().insert_at(other, VPos{});
    cr_assert((collect<VPos, VVel, VHp, VTag, VExtra>(reg)) == (std::vector<std::size_t>{e}));
}

Test(view_ext, same_component_twice_behaves_like_single)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    reg.get_components<VPos>().insert_at(3, VPos{});
    cr_assert((collect<VPos, VPos>(reg)) == (collect<VPos>(reg)));
}

Test(view_ext, tag_component_as_filter)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VTag>();
    for (std::size_t i = 0; i < 10; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    reg.get_components<VTag>().insert_at(7, VTag{});
    cr_assert((collect<VPos, VTag>(reg)) == (std::vector<std::size_t>{7}));
}

Test(view_ext, tag_component_as_driver)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VTag>();
    for (std::size_t i = 0; i < 10; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    reg.get_components<VTag>().insert_at(7, VTag{});
    reg.get_components<VTag>().insert_at(2, VTag{});
    cr_assert((collect<VTag, VPos>(reg)) == (std::vector<std::size_t>{7, 2}));
}

Test(view_ext, view_yields_each_entity_once)
{
    Registry reg;
    reg.register_component<VPos>();
    for (int round = 0; round < 3; ++round)
        for (std::size_t i = 0; i < 10; ++i)
            reg.get_components<VPos>().insert_at(i, VPos{});
    cr_assert_eq((collect<VPos>(reg).size()), 10u);
}

Test(view_ext, view_sees_entities_added_after_creation)
{
    Registry reg;
    reg.register_component<VPos>();
    auto view = reg.view<VPos>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    reg.get_components<VPos>().insert_at(1, VPos{});
    std::size_t count = 0;
    for (auto e : view) {
        (void)e;
        ++count;
    }
    cr_assert_eq(count, 2u);
}

Test(view_ext, view_sees_filter_components_added_after_creation)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    auto view = reg.view<VPos, VVel>();
    reg.get_components<VVel>().insert_at(0, VVel{});
    std::size_t count = 0;
    for (auto e : view) {
        (void)e;
        ++count;
    }
    cr_assert_eq(count, 1u);
}

Test(view_ext, view_can_be_iterated_twice)
{
    Registry reg;
    reg.register_component<VPos>();
    for (std::size_t i = 0; i < 5; ++i)
        reg.get_components<VPos>().insert_at(i, VPos{});
    auto view = reg.view<VPos>();
    std::size_t first = 0;
    std::size_t second = 0;
    for (auto e : view) { (void)e; ++first; }
    for (auto e : view) { (void)e; ++second; }
    cr_assert_eq(first, 5u);
    cr_assert_eq(second, 5u);
}

Test(view_ext, pending_kill_still_visible_until_flush)
{
    Registry reg;
    reg.register_component<VPos>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<VPos>().insert_at(e, VPos{});
    reg.kill_entity(e);
    cr_assert_eq((collect<VPos>(reg).size()), 1u);
    reg.flush();
    cr_assert((collect<VPos>(reg).empty()));
}

Test(view_ext, killed_entity_removed_from_multi_view)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    std::size_t a = reg.spawn_entity();
    std::size_t b = reg.spawn_entity();
    for (auto e : {a, b}) {
        reg.get_components<VPos>().insert_at(e, VPos{});
        reg.get_components<VVel>().insert_at(e, VVel{});
    }
    reg.kill_entity(a);
    reg.flush();
    cr_assert((collect<VPos, VVel>(reg)) == (std::vector<std::size_t>{b}));
}

Test(view_ext, removing_filter_component_excludes_entity)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    reg.get_components<VPos>().insert_at(0, VPos{});
    reg.get_components<VVel>().insert_at(0, VVel{});
    reg.get_components<VVel>().erase(0);
    cr_assert((collect<VPos, VVel>(reg).empty()));
}

Test(view_ext, recycled_entity_appears_again_once_components_set)
{
    Registry reg;
    reg.register_component<VPos>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<VPos>().insert_at(e, VPos{});
    reg.kill_entity(e);
    reg.flush();
    std::size_t r = reg.spawn_entity();
    cr_assert((collect<VPos>(reg).empty()));
    reg.get_components<VPos>().insert_at(r, VPos{});
    cr_assert((collect<VPos>(reg)) == (std::vector<std::size_t>{r}));
}

Test(view_ext, mutate_components_through_view)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    auto &pos = reg.get_components<VPos>();
    auto &vel = reg.get_components<VVel>();
    for (std::size_t i = 0; i < 10; ++i) {
        pos.insert_at(i, VPos{0.f, 0.f});
        vel.insert_at(i, VVel{static_cast<float>(i), 1.f});
    }
    for (auto e : reg.view<VPos, VVel>()) {
        pos.get(e)->x += vel.get(e)->dx;
        pos.get(e)->y += vel.get(e)->dy;
    }
    for (std::size_t i = 0; i < 10; ++i) {
        cr_assert_float_eq(pos.get(i)->x, static_cast<float>(i), 1e-6);
        cr_assert_float_eq(pos.get(i)->y, 1.f, 1e-6);
    }
}

Test(view_ext, movement_simulation_multiple_frames)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    auto &pos = reg.get_components<VPos>();
    auto &vel = reg.get_components<VVel>();
    std::size_t e = reg.spawn_entity();
    pos.insert_at(e, VPos{0.f, 0.f});
    vel.insert_at(e, VVel{2.f, -1.f});
    for (int frame = 0; frame < 10; ++frame)
        for (auto ent : reg.view<VPos, VVel>()) {
            pos.get(ent)->x += vel.get(ent)->dx * 0.5f;
            pos.get(ent)->y += vel.get(ent)->dy * 0.5f;
        }
    cr_assert_float_eq(pos.get(e)->x, 10.f, 1e-5);
    cr_assert_float_eq(pos.get(e)->y, -5.f, 1e-5);
}

Test(view_ext, static_entities_not_moved)
{
    Registry reg;
    reg.register_component<VPos>();
    reg.register_component<VVel>();
    auto &pos = reg.get_components<VPos>();
    auto &vel = reg.get_components<VVel>();
    std::size_t mover = reg.spawn_entity();
    std::size_t wall = reg.spawn_entity();
    pos.insert_at(mover, VPos{0.f, 0.f});
    pos.insert_at(wall, VPos{5.f, 5.f});
    vel.insert_at(mover, VVel{1.f, 1.f});
    for (auto ent : reg.view<VPos, VVel>())
        pos.get(ent)->x += vel.get(ent)->dx;
    cr_assert_float_eq(pos.get(wall)->x, 5.f, 1e-6);
    cr_assert_float_eq(pos.get(mover)->x, 1.f, 1e-6);
}

Test(view_ext, view_over_primitive_components)
{
    Registry reg;
    reg.register_component<int>();
    reg.register_component<char>();
    reg.get_components<int>().insert_at(0, 1);
    reg.get_components<int>().insert_at(1, 2);
    reg.get_components<char>().insert_at(1, 'c');
    cr_assert((collect<int, char>(reg)) == (std::vector<std::size_t>{1}));
}

Test(view_ext, sum_hp_with_view)
{
    Registry reg;
    reg.register_component<VHp>();
    auto &hp = reg.get_components<VHp>();
    int expected = 0;
    for (int i = 0; i < 100; ++i) {
        std::size_t e = reg.spawn_entity();
        hp.insert_at(e, VHp{i});
        expected += i;
    }
    int sum = 0;
    for (auto e : reg.view<VHp>())
        sum += hp.get(e)->hp;
    cr_assert_eq(sum, expected);
}

Test(view_ext, view_after_half_entities_killed)
{
    Registry reg;
    reg.register_component<VHp>();
    for (int i = 0; i < 100; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<VHp>().insert_at(e, VHp{i});
    }
    for (std::size_t e = 0; e < 100; e += 2)
        reg.kill_entity(e);
    reg.flush();
    for (auto e : collect<VHp>(reg))
        cr_assert_eq(e % 2, 1u);
    cr_assert_eq((collect<VHp>(reg).size()), 50u);
}

#define VIEW_MODULO_TEST(A, B) \
    Test(view_param, intersection_mod_##A##_and_##B) \
    { \
        Registry reg; \
        reg.register_component<VPos>(); \
        reg.register_component<VVel>(); \
        const std::size_t total = 600; \
        std::size_t expected = 0; \
        for (std::size_t i = 0; i < total; ++i) { \
            if (i % (A) == 0) \
                reg.get_components<VPos>().insert_at(i, VPos{}); \
            if (i % (B) == 0) \
                reg.get_components<VVel>().insert_at(i, VVel{}); \
            if (i % (A) == 0 && i % (B) == 0) \
                ++expected; \
        } \
        auto out = collect<VPos, VVel>(reg); \
        cr_assert_eq(out.size(), expected); \
        for (auto e : out) { \
            cr_assert_eq(e % (A), 0u); \
            cr_assert_eq(e % (B), 0u); \
        } \
        cr_assert_eq((collect<VVel, VPos>(reg).size()), expected); \
    }

VIEW_MODULO_TEST(1, 1)
VIEW_MODULO_TEST(1, 2)
VIEW_MODULO_TEST(2, 3)
VIEW_MODULO_TEST(3, 4)
VIEW_MODULO_TEST(4, 6)
VIEW_MODULO_TEST(5, 7)
VIEW_MODULO_TEST(6, 9)
VIEW_MODULO_TEST(10, 15)
VIEW_MODULO_TEST(11, 13)
VIEW_MODULO_TEST(1, 599)

#define VIEW_COUNT_TEST(N) \
    Test(view_param, single_view_counts_##N##_entities) \
    { \
        Registry reg; \
        reg.register_component<VHp>(); \
        for (std::size_t i = 0; i < (N); ++i) { \
            std::size_t e = reg.spawn_entity(); \
            reg.get_components<VHp>().insert_at(e, VHp{static_cast<int>(i)}); \
        } \
        std::size_t count = 0; \
        std::size_t last = NULL_ENTITY; \
        for (auto e : reg.view<VHp>()) { \
            cr_assert_neq(e, last); \
            last = e; \
            ++count; \
        } \
        cr_assert_eq(count, static_cast<std::size_t>(N)); \
    }

VIEW_COUNT_TEST(0)
VIEW_COUNT_TEST(1)
VIEW_COUNT_TEST(2)
VIEW_COUNT_TEST(9)
VIEW_COUNT_TEST(42)
VIEW_COUNT_TEST(128)
VIEW_COUNT_TEST(500)
VIEW_COUNT_TEST(1000)
VIEW_COUNT_TEST(5000)
VIEW_COUNT_TEST(20000)
