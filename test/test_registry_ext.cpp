#include <criterion/criterion.h>
#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include "Registry.hpp"

using namespace rtk::ecs;

namespace {
    struct RPos {
        float x;
        float y;
    };

    struct RVel {
        float dx;
        float dy;
    };

    struct RHp {
        int hp;
    };

    struct RName {
        std::string name;
    };

    struct RTag {};

    struct RNeverRegistered {
        int unused;
    };

    struct ROwned {
        std::shared_ptr<int> ptr;
    };
}

Test(registry_ext, first_spawn_is_zero)
{
    Registry reg;
    cr_assert_eq(reg.spawn_entity(), 0u);
}

Test(registry_ext, spawn_is_sequential)
{
    Registry reg;
    for (std::size_t i = 0; i < 50; ++i)
        cr_assert_eq(reg.spawn_entity(), i);
}

Test(registry_ext, spawn_without_any_component_registered)
{
    Registry reg;
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e);
}

Test(registry_ext, two_registries_have_independent_id_counters)
{
    Registry a;
    Registry b;
    a.spawn_entity();
    a.spawn_entity();
    cr_assert_eq(b.spawn_entity(), 0u);
    cr_assert_eq(a.spawn_entity(), 2u);
}

Test(registry_ext, two_registries_have_independent_pools)
{
    Registry a;
    Registry b;
    a.register_component<RHp>();
    b.register_component<RHp>();
    a.get_components<RHp>().insert_at(0, RHp{1});
    cr_assert_not(b.has_component<RHp>(0));
}

Test(registry_ext, kill_is_deferred_until_flush)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(e, RHp{10});
    reg.kill_entity(e);
    cr_assert(reg.has_component<RHp>(e));
    reg.flush();
    cr_assert_not(reg.has_component<RHp>(e));
}

Test(registry_ext, id_not_recycled_before_flush)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    reg.kill_entity(e0);
    std::size_t e1 = reg.spawn_entity();
    cr_assert_neq(e0, e1);
    cr_assert_eq(e1, 1u);
}

Test(registry_ext, id_recycled_after_flush)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    reg.spawn_entity();
    reg.kill_entity(e0);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e0);
}

Test(registry_ext, free_ids_are_reused_lifo)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    std::size_t e2 = reg.spawn_entity();
    reg.kill_entity(e0);
    reg.kill_entity(e1);
    reg.kill_entity(e2);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e2);
    cr_assert_eq(reg.spawn_entity(), e1);
    cr_assert_eq(reg.spawn_entity(), e0);
}

Test(registry_ext, counter_resumes_after_free_ids_exhausted)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    reg.spawn_entity();
    reg.kill_entity(e0);
    reg.flush();
    reg.spawn_entity();
    cr_assert_eq(reg.spawn_entity(), 2u);
}

Test(registry_ext, double_kill_before_flush_recycles_once)
{
    Registry reg;
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.kill_entity(e);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, kill_after_flush_on_dead_entity_is_ignored)
{
    Registry reg;
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.flush();
    reg.kill_entity(e);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, kill_unknown_entity_is_ignored)
{
    Registry reg;
    reg.kill_entity(42);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), 0u);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, kill_null_entity_is_ignored)
{
    Registry reg;
    reg.spawn_entity();
    reg.kill_entity(NULL_ENTITY);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, kill_unknown_entity_does_not_erase_its_pool_data)
{
    Registry reg;
    reg.register_component<RHp>();
    reg.get_components<RHp>().insert_at(7, RHp{7});
    reg.kill_entity(7);
    reg.flush();
    cr_assert(reg.has_component<RHp>(7));
}

Test(registry_ext, flush_without_pending_is_noop)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(e, RHp{1});
    reg.flush();
    reg.flush();
    cr_assert(reg.has_component<RHp>(e));
}

Test(registry_ext, flush_on_empty_registry_is_noop)
{
    Registry reg;
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), 0u);
}

Test(registry_ext, second_flush_does_not_recycle_again)
{
    Registry reg;
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.flush();
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, kill_respawned_entity_works)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.flush();
    std::size_t again = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(again, RHp{3});
    reg.kill_entity(again);
    reg.flush();
    cr_assert_not(reg.has_component<RHp>(again));
    cr_assert_eq(reg.spawn_entity(), again);
}

Test(registry_ext, flush_only_kills_pending_entities)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t a = reg.spawn_entity();
    std::size_t b = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(a, RHp{1});
    reg.get_components<RHp>().insert_at(b, RHp{2});
    reg.kill_entity(a);
    reg.flush();
    cr_assert_not(reg.has_component<RHp>(a));
    cr_assert(reg.has_component<RHp>(b));
    cr_assert_eq(reg.get_components<RHp>().get(b)->hp, 2);
}

Test(registry_ext, kill_entity_without_components_then_flush)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.flush();
    cr_assert_eq(reg.get_components<RHp>().size(), 0u);
}

Test(registry_ext, kill_removes_from_every_pool)
{
    Registry reg;
    reg.register_component<RPos>();
    reg.register_component<RVel>();
    reg.register_component<RHp>();
    reg.register_component<RName>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RPos>().insert_at(e, RPos{});
    reg.get_components<RVel>().insert_at(e, RVel{});
    reg.get_components<RHp>().insert_at(e, RHp{});
    reg.get_components<RName>().insert_at(e, RName{"x"});
    reg.kill_entity(e);
    reg.flush();
    cr_assert_not(reg.has_component<RPos>(e));
    cr_assert_not(reg.has_component<RVel>(e));
    cr_assert_not(reg.has_component<RHp>(e));
    cr_assert_not(reg.has_component<RName>(e));
}

Test(registry_ext, kill_keeps_swapped_entity_value_intact)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t a = reg.spawn_entity();
    std::size_t b = reg.spawn_entity();
    std::size_t c = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(a, RHp{1});
    reg.get_components<RHp>().insert_at(b, RHp{2});
    reg.get_components<RHp>().insert_at(c, RHp{3});
    reg.kill_entity(a);
    reg.flush();
    cr_assert_eq(reg.get_components<RHp>().get(b)->hp, 2);
    cr_assert_eq(reg.get_components<RHp>().get(c)->hp, 3);
}

Test(registry_ext, recycled_entity_starts_with_no_components)
{
    Registry reg;
    reg.register_component<RPos>();
    reg.register_component<RVel>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RPos>().insert_at(e, RPos{1.f, 1.f});
    reg.get_components<RVel>().insert_at(e, RVel{1.f, 1.f});
    reg.kill_entity(e);
    reg.flush();
    std::size_t r = reg.spawn_entity();
    cr_assert_eq(r, e);
    cr_assert_not(reg.has_component<RPos>(r));
    cr_assert_not(reg.has_component<RVel>(r));
}

Test(registry_ext, kill_many_then_flush_once)
{
    Registry reg;
    reg.register_component<RHp>();
    for (int i = 0; i < 100; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<RHp>().insert_at(e, RHp{i});
    }
    for (std::size_t e = 0; e < 100; ++e)
        reg.kill_entity(e);
    cr_assert_eq(reg.get_components<RHp>().size(), 100u);
    reg.flush();
    cr_assert_eq(reg.get_components<RHp>().size(), 0u);
}

Test(registry_ext, respawn_after_mass_kill_reuses_exactly_same_ids)
{
    Registry reg;
    std::set<std::size_t> original;
    for (int i = 0; i < 64; ++i)
        original.insert(reg.spawn_entity());
    for (auto e : original)
        reg.kill_entity(e);
    reg.flush();
    std::set<std::size_t> reused;
    for (int i = 0; i < 64; ++i)
        reused.insert(reg.spawn_entity());
    cr_assert(original == reused);
    cr_assert_eq(reg.spawn_entity(), 64u);
}

Test(registry_ext, interleaved_kill_spawn_before_flush)
{
    Registry reg;
    std::size_t a = reg.spawn_entity();
    reg.kill_entity(a);
    std::size_t b = reg.spawn_entity();
    reg.flush();
    std::size_t c = reg.spawn_entity();
    cr_assert_eq(a, 0u);
    cr_assert_eq(b, 1u);
    cr_assert_eq(c, 0u);
}

Test(registry_ext, kill_same_entity_between_flushes_is_stable)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(e, RHp{1});
    for (int i = 0; i < 5; ++i)
        reg.kill_entity(e);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, insert_on_pending_killed_entity_is_erased_on_flush)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.kill_entity(e);
    reg.get_components<RHp>().insert_at(e, RHp{1});
    reg.flush();
    cr_assert_not(reg.has_component<RHp>(e));
}

Test(registry_ext, register_returns_same_pool_as_get)
{
    Registry reg;
    auto &pool = reg.register_component<RHp>();
    cr_assert_eq(&pool, &reg.get_components<RHp>());
}

Test(registry_ext, get_components_is_stable_across_calls)
{
    Registry reg;
    reg.register_component<RHp>();
    auto *first = &reg.get_components<RHp>();
    reg.register_component<RPos>();
    reg.register_component<RVel>();
    cr_assert_eq(first, &reg.get_components<RHp>());
}

Test(registry_ext, const_get_components_returns_same_pool)
{
    Registry reg;
    reg.register_component<RHp>();
    Registry const &creg = reg;
    cr_assert_eq(&creg.get_components<RHp>(), &reg.get_components<RHp>());
}

Test(registry_ext, const_get_components_reads_values)
{
    Registry reg;
    reg.register_component<RHp>();
    reg.get_components<RHp>().insert_at(2, RHp{20});
    Registry const &creg = reg;
    cr_assert_eq(creg.get_components<RHp>().get(2)->hp, 20);
}

Test(registry_ext, registered_pool_starts_empty)
{
    Registry reg;
    auto &pool = reg.register_component<RName>();
    cr_assert_eq(pool.size(), 0u);
    cr_assert(pool.get_packed_array().empty());
}

Test(registry_ext, register_order_does_not_matter)
{
    Registry a;
    Registry b;
    a.register_component<RPos>();
    a.register_component<RVel>();
    b.register_component<RVel>();
    b.register_component<RPos>();
    a.get_components<RPos>().insert_at(0, RPos{1.f, 2.f});
    b.get_components<RPos>().insert_at(0, RPos{1.f, 2.f});
    cr_assert(a.has_component<RPos>(0));
    cr_assert(b.has_component<RPos>(0));
    cr_assert_not(b.has_component<RVel>(0));
}

Test(registry_ext, register_with_unregistered_lower_ids_creates_gaps)
{
    (void)ComponentType::get_id<RNeverRegistered>();
    Registry reg;
    reg.register_component<RTag>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RTag>().insert_at(e, RTag{});
    reg.kill_entity(e);
    reg.flush();
    cr_assert_not(reg.has_component<RTag>(e));
}

Test(registry_ext, remove_entity_skips_unregistered_slots)
{
    (void)ComponentType::get_id<RNeverRegistered>();
    (void)ComponentType::get_id<RPos>();
    Registry reg;
    reg.register_component<RHp>();
    reg.get_components<RHp>().insert_at(1, RHp{1});
    reg.remove_entity_from_all_pools(1);
    cr_assert_not(reg.has_component<RHp>(1));
}

Test(registry_ext, has_component_reflects_insert_and_erase)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    cr_assert_not(reg.has_component<RHp>(e));
    reg.get_components<RHp>().insert_at(e, RHp{1});
    cr_assert(reg.has_component<RHp>(e));
    reg.get_components<RHp>().erase(e);
    cr_assert_not(reg.has_component<RHp>(e));
}

Test(registry_ext, has_component_per_type)
{
    Registry reg;
    reg.register_component<RPos>();
    reg.register_component<RVel>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RVel>().insert_at(e, RVel{});
    cr_assert_not(reg.has_component<RPos>(e));
    cr_assert(reg.has_component<RVel>(e));
}

Test(registry_ext, has_component_for_huge_id_is_false)
{
    Registry reg;
    reg.register_component<RPos>();
    cr_assert_not(reg.has_component<RPos>(1u << 20));
}

Test(registry_ext, component_can_be_attached_to_non_spawned_id)
{
    Registry reg;
    reg.register_component<RPos>();
    reg.get_components<RPos>().insert_at(500, RPos{5.f, 5.f});
    cr_assert(reg.has_component<RPos>(500));
}

Test(registry_ext, tag_component_registration)
{
    Registry reg;
    reg.register_component<RTag>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RTag>().insert_at(e, RTag{});
    cr_assert(reg.has_component<RTag>(e));
}

Test(registry_ext, string_component_survives_other_kills)
{
    Registry reg;
    reg.register_component<RName>();
    std::size_t a = reg.spawn_entity();
    std::size_t b = reg.spawn_entity();
    reg.get_components<RName>().insert_at(a, RName{"alpha"});
    reg.get_components<RName>().insert_at(b, RName{"beta"});
    reg.kill_entity(a);
    reg.flush();
    cr_assert_str_eq(reg.get_components<RName>().get(b)->name.c_str(), "beta");
}

Test(registry_ext, primitive_component_types)
{
    Registry reg;
    reg.register_component<int>();
    reg.register_component<double>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<int>().insert_at(e, 3);
    reg.get_components<double>().insert_at(e, 2.5);
    cr_assert_eq(*reg.get_components<int>().get(e), 3);
    cr_assert_float_eq(*reg.get_components<double>().get(e), 2.5, 1e-9);
}

Test(registry_ext, many_component_types_registered)
{
    Registry reg;
    reg.register_component<RPos>();
    reg.register_component<RVel>();
    reg.register_component<RHp>();
    reg.register_component<RName>();
    reg.register_component<RTag>();
    reg.register_component<int>();
    reg.register_component<char>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<char>().insert_at(e, 'z');
    cr_assert(reg.has_component<char>(e));
    cr_assert_not(reg.has_component<int>(e));
}

Test(registry_ext, remove_from_all_pools_is_immediate)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(e, RHp{1});
    reg.remove_entity_from_all_pools(e);
    cr_assert_not(reg.has_component<RHp>(e));
}

Test(registry_ext, remove_from_all_pools_does_not_recycle_id)
{
    Registry reg;
    std::size_t e = reg.spawn_entity();
    reg.remove_entity_from_all_pools(e);
    reg.flush();
    cr_assert_neq(reg.spawn_entity(), e);
}

Test(registry_ext, remove_from_all_pools_on_entity_without_components)
{
    Registry reg;
    reg.register_component<RHp>();
    reg.register_component<RPos>();
    std::size_t e = reg.spawn_entity();
    reg.remove_entity_from_all_pools(e);
    cr_assert_eq(reg.get_components<RHp>().size(), 0u);
    cr_assert_eq(reg.get_components<RPos>().size(), 0u);
}

Test(registry_ext, remove_from_all_pools_with_no_pools)
{
    Registry reg;
    std::size_t e = reg.spawn_entity();
    reg.remove_entity_from_all_pools(e);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(registry_ext, remove_from_all_pools_entity_can_get_components_again)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<RHp>().insert_at(e, RHp{1});
    reg.remove_entity_from_all_pools(e);
    reg.get_components<RHp>().insert_at(e, RHp{2});
    cr_assert_eq(reg.get_components<RHp>().get(e)->hp, 2);
}

Test(registry_ext, remove_from_all_pools_leaves_entity_killable)
{
    Registry reg;
    reg.register_component<RHp>();
    std::size_t e = reg.spawn_entity();
    reg.remove_entity_from_all_pools(e);
    reg.kill_entity(e);
    reg.flush();
    cr_assert_eq(reg.spawn_entity(), e);
}

Test(registry_ext, destructor_releases_component_resources)
{
    auto p = std::make_shared<int>(1);
    {
        Registry reg;
        reg.register_component<ROwned>();
        std::size_t e = reg.spawn_entity();
        reg.get_components<ROwned>().insert_at(e, ROwned{p});
        cr_assert_eq(p.use_count(), 2);
    }
    cr_assert_eq(p.use_count(), 1);
}

Test(registry_ext, flush_releases_component_resources)
{
    auto p = std::make_shared<int>(1);
    Registry reg;
    reg.register_component<ROwned>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<ROwned>().insert_at(e, ROwned{p});
    reg.kill_entity(e);
    cr_assert_eq(p.use_count(), 2);
    reg.flush();
    cr_assert_eq(p.use_count(), 1);
}

Test(registry_ext, destructor_with_pending_kills_does_not_crash)
{
    auto p = std::make_shared<int>(1);
    {
        Registry reg;
        reg.register_component<ROwned>();
        std::size_t e = reg.spawn_entity();
        reg.get_components<ROwned>().insert_at(e, ROwned{p});
        reg.kill_entity(e);
    }
    cr_assert_eq(p.use_count(), 1);
}

Test(registry_ext, destructor_of_empty_registry)
{
    {
        Registry reg;
    }
    cr_assert(true);
}

Test(registry_ext, destructor_with_many_pools_and_entities)
{
    auto p = std::make_shared<int>(1);
    {
        Registry reg;
        reg.register_component<ROwned>();
        reg.register_component<RPos>();
        reg.register_component<RName>();
        for (int i = 0; i < 100; ++i) {
            std::size_t e = reg.spawn_entity();
            reg.get_components<ROwned>().insert_at(e, ROwned{p});
            reg.get_components<RName>().insert_at(e, RName{"n"});
        }
        cr_assert_eq(p.use_count(), 101);
    }
    cr_assert_eq(p.use_count(), 1);
}

Test(registry_ext, churn_spawn_kill_flush_keeps_ids_bounded)
{
    Registry reg;
    reg.register_component<RHp>();
    for (int round = 0; round < 100; ++round) {
        std::vector<std::size_t> ids;
        for (int i = 0; i < 10; ++i) {
            std::size_t e = reg.spawn_entity();
            reg.get_components<RHp>().insert_at(e, RHp{i});
            ids.push_back(e);
        }
        for (auto e : ids)
            reg.kill_entity(e);
        reg.flush();
    }
    for (int i = 0; i < 10; ++i)
        cr_assert_lt(reg.spawn_entity(), 10u);
    cr_assert_eq(reg.spawn_entity(), 10u);
}

Test(registry_ext, kill_odd_entities_keeps_even_data)
{
    Registry reg;
    reg.register_component<RHp>();
    for (int i = 0; i < 40; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<RHp>().insert_at(e, RHp{i});
    }
    for (std::size_t e = 1; e < 40; e += 2)
        reg.kill_entity(e);
    reg.flush();
    for (std::size_t e = 0; e < 40; e += 2)
        cr_assert_eq(reg.get_components<RHp>().get(e)->hp, static_cast<int>(e));
    cr_assert_eq(reg.get_components<RHp>().size(), 20u);
}

Test(registry_ext, pool_coherent_after_mixed_kills)
{
    Registry reg;
    reg.register_component<RHp>();
    for (int i = 0; i < 60; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<RHp>().insert_at(e, RHp{i});
    }
    for (std::size_t e = 0; e < 60; e += 3)
        reg.kill_entity(e);
    reg.flush();
    auto &pool = reg.get_components<RHp>();
    for (std::size_t d = 0; d < pool._dense.size(); ++d) {
        std::size_t ent = pool._packed_entities[d];
        cr_assert_eq(pool._sparse[ent], d);
        cr_assert_eq(pool._dense[d].hp, static_cast<int>(ent));
    }
}

Test(registry_ext, spawn_100k_entities)
{
    Registry reg;
    std::size_t last = 0;
    for (int i = 0; i < 100000; ++i)
        last = reg.spawn_entity();
    cr_assert_eq(last, 99999u);
}

#define REG_RECYCLE_TEST(N) \
    Test(registry_param, kill_all_and_recycle_##N) \
    { \
        Registry reg; \
        reg.register_component<RHp>(); \
        for (std::size_t i = 0; i < (N); ++i) { \
            std::size_t e = reg.spawn_entity(); \
            reg.get_components<RHp>().insert_at(e, RHp{static_cast<int>(i)}); \
        } \
        for (std::size_t i = 0; i < (N); ++i) \
            reg.kill_entity(i); \
        reg.flush(); \
        cr_assert_eq(reg.get_components<RHp>().size(), 0u); \
        std::set<std::size_t> seen; \
        for (std::size_t i = 0; i < (N); ++i) { \
            std::size_t e = reg.spawn_entity(); \
            cr_assert_lt(e, static_cast<std::size_t>(N)); \
            seen.insert(e); \
        } \
        cr_assert_eq(seen.size(), static_cast<std::size_t>(N)); \
        cr_assert_eq(reg.spawn_entity(), static_cast<std::size_t>(N)); \
    }

REG_RECYCLE_TEST(1)
REG_RECYCLE_TEST(2)
REG_RECYCLE_TEST(5)
REG_RECYCLE_TEST(10)
REG_RECYCLE_TEST(25)
REG_RECYCLE_TEST(64)
REG_RECYCLE_TEST(100)
REG_RECYCLE_TEST(256)
REG_RECYCLE_TEST(777)
REG_RECYCLE_TEST(2048)

#define REG_KILL_MODULO_TEST(M) \
    Test(registry_param, kill_every_##M##th_entity) \
    { \
        Registry reg; \
        reg.register_component<RHp>(); \
        reg.register_component<RPos>(); \
        const std::size_t total = 300; \
        for (std::size_t i = 0; i < total; ++i) { \
            std::size_t e = reg.spawn_entity(); \
            reg.get_components<RHp>().insert_at(e, RHp{static_cast<int>(i)}); \
            reg.get_components<RPos>().insert_at(e, RPos{static_cast<float>(i), 0.f}); \
        } \
        std::size_t killed = 0; \
        for (std::size_t i = 0; i < total; i += (M)) { \
            reg.kill_entity(i); \
            ++killed; \
        } \
        reg.flush(); \
        cr_assert_eq(reg.get_components<RHp>().size(), total - killed); \
        cr_assert_eq(reg.get_components<RPos>().size(), total - killed); \
        for (std::size_t i = 0; i < total; ++i) { \
            bool dead = (i % (M)) == 0; \
            cr_assert_eq(reg.has_component<RHp>(i), !dead); \
            if (!dead) { \
                cr_assert_eq(reg.get_components<RHp>().get(i)->hp, static_cast<int>(i)); \
            } \
        } \
    }

REG_KILL_MODULO_TEST(1)
REG_KILL_MODULO_TEST(2)
REG_KILL_MODULO_TEST(3)
REG_KILL_MODULO_TEST(4)
REG_KILL_MODULO_TEST(5)
REG_KILL_MODULO_TEST(7)
REG_KILL_MODULO_TEST(10)
REG_KILL_MODULO_TEST(17)
REG_KILL_MODULO_TEST(50)
REG_KILL_MODULO_TEST(299)
