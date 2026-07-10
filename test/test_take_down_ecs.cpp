
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <optional>
#include <cstddef>
#include <vector>
#include <thread>

#include "Registry.hpp"
#include "SparseArray.hpp"
#include "ComponentType.hpp"
#include "chronoDebug.hpp"

using namespace rtk::ecs;

struct Position {
    float x;
    float y;
    bool operator==(Position const &o) const { return x == o.x && y == o.y; }
};

struct Velocity {
    float dx;
    float dy;
    bool operator==(Velocity const &o) const { return dx == o.dx && dy == o.dy; }
};

struct Health {
    int hp;
    bool operator==(Health const &o) const { return hp == o.hp; }
};

struct Tag {
    bool operator==(Tag const &) const { return true; }
};

Test(sparse_array, starts_empty)
{
    SparseArray<Position> sa;
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array, insert_increases_size)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 2.f});
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array, insert_returns_reference_to_stored_component)
{
    SparseArray<Position> sa;
    Position &ref = sa.insert_at(3, Position{4.f, 5.f});
    cr_assert_eq(ref.x, 4.f);
    cr_assert_eq(ref.y, 5.f);
}

Test(sparse_array, insert_stores_correct_value)
{
    SparseArray<Position> sa;
    sa.insert_at(2, Position{7.f, 8.f});
    auto val = sa.get(2);
    cr_assert(val.has_value());
    cr_assert_eq(val->x, 7.f);
    cr_assert_eq(val->y, 8.f);
}

Test(sparse_array, insert_overwrite_updates_value)
{
    SparseArray<Position> sa;
    sa.insert_at(1, Position{1.f, 1.f});
    sa.insert_at(1, Position{9.f, 9.f});
    auto val = sa.get(1);
    cr_assert(val.has_value());
    cr_assert_eq(val->x, 9.f);
    cr_assert_eq(val->y, 9.f);
}

Test(sparse_array, insert_overwrite_does_not_grow_size)
{
    SparseArray<Position> sa;
    sa.insert_at(1, Position{1.f, 1.f});
    sa.insert_at(1, Position{9.f, 9.f});
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array, get_on_empty_array_returns_nullopt)
{
    SparseArray<Position> sa;
    cr_assert_eq(sa.get(0).has_value(), false);
}

Test(sparse_array, get_unset_entity_returns_nullopt)
{
    SparseArray<Position> sa;
    sa.insert_at(5, Position{1.f, 1.f});
    cr_assert_eq(sa.get(6).has_value(), false);
}

Test(sparse_array, contains_true_after_insert)
{
    SparseArray<Position> sa;
    sa.insert_at(4, Position{0.f, 0.f});
    cr_assert(sa.contains(4));
}

Test(sparse_array, contains_false_before_insert)
{
    SparseArray<Position> sa;
    cr_assert_not(sa.contains(4));
}

Test(sparse_array, contains_false_for_unrelated_entity)
{
    SparseArray<Position> sa;
    sa.insert_at(1, Position{0.f, 0.f});
    cr_assert_not(sa.contains(2));
}

Test(sparse_array, erase_decreases_size)
{
    SparseArray<Position> sa;
    sa.insert_at(1, Position{0.f, 0.f});
    sa.erase(1);
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array, erase_removes_containment)
{
    SparseArray<Position> sa;
    sa.insert_at(1, Position{0.f, 0.f});
    sa.erase(1);
    cr_assert_not(sa.contains(1));
}

Test(sparse_array, erase_makes_get_return_nullopt)
{
    SparseArray<Position> sa;
    sa.insert_at(1, Position{0.f, 0.f});
    sa.erase(1);
    cr_assert_eq(sa.get(1).has_value(), false);
}

Test(sparse_array, erase_only_element_empties_array)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 1.f});
    sa.erase(0);
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array, erase_swap_pop_keeps_other_entity_readable)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 1.f});
    sa.insert_at(1, Position{2.f, 2.f});
    sa.insert_at(2, Position{3.f, 3.f});
    sa.erase(0);
    cr_assert(sa.contains(1));
    cr_assert(sa.contains(2));
    auto v1 = sa.get(1);
    auto v2 = sa.get(2);
    cr_assert(v1.has_value());
    cr_assert(v2.has_value());
    cr_assert_eq(v1->x, 2.f);
    cr_assert_eq(v2->x, 3.f);
}

Test(sparse_array, erase_middle_element_keeps_first_and_last)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 1.f});
    sa.insert_at(1, Position{2.f, 2.f});
    sa.insert_at(2, Position{3.f, 3.f});
    sa.erase(1);
    cr_assert(sa.contains(0));
    cr_assert(sa.contains(2));
    cr_assert_not(sa.contains(1));
    cr_assert_eq(sa.size(), 2u);
}

Test(sparse_array, erase_last_element_only)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 1.f});
    sa.insert_at(1, Position{2.f, 2.f});
    sa.erase(1);
    cr_assert(sa.contains(0));
    cr_assert_not(sa.contains(1));
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array, erase_nonexistent_entity_does_not_crash)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 1.f});
    sa.erase(99);
    cr_assert_eq(sa.size(), 1u);
    cr_assert(sa.contains(0));
}

Test(sparse_array, erase_on_totally_empty_array_does_not_crash)
{
    SparseArray<Position> sa;
    sa.erase(0);
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array, reinsert_after_erase_works)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{1.f, 1.f});
    sa.erase(0);
    sa.insert_at(0, Position{5.f, 5.f});
    cr_assert(sa.contains(0));
    auto v = sa.get(0);
    cr_assert(v.has_value());
    cr_assert_eq(v->x, 5.f);
}

Test(sparse_array, multiple_distinct_entities_all_retrievable)
{
    SparseArray<Position> sa;
    for (std::size_t i = 0; i < 10; ++i)
        sa.insert_at(i, Position{(float)i, (float)i});
    cr_assert_eq(sa.size(), 10u);
    for (std::size_t i = 0; i < 10; ++i) {
        auto v = sa.get(i);
        cr_assert(v.has_value());
        cr_assert_eq(v->x, (float)i);
    }
}

Test(sparse_array, insert_at_sparse_large_index_does_not_crash)
{
    SparseArray<Position> sa;
    sa.insert_at(1000, Position{1.f, 1.f});
    cr_assert(sa.contains(1000));
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array, insert_at_sparse_index_leaves_gaps_uncontained)
{
    SparseArray<Position> sa;
    sa.insert_at(50, Position{1.f, 1.f});
    cr_assert_not(sa.contains(10));
    cr_assert_not(sa.contains(49));
    cr_assert_not(sa.contains(51));
}

Test(sparse_array, size_reflects_sequence_of_inserts_and_erases)
{
    SparseArray<Position> sa;
    sa.insert_at(0, Position{0.f, 0.f});
    sa.insert_at(1, Position{1.f, 1.f});
    sa.insert_at(2, Position{2.f, 2.f});
    cr_assert_eq(sa.size(), 3u);
    sa.erase(1);
    cr_assert_eq(sa.size(), 2u);
    sa.erase(0);
    cr_assert_eq(sa.size(), 1u);
    sa.erase(2);
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array, works_independently_per_component_type)
{
    SparseArray<Position> sap;
    SparseArray<Velocity> sav;
    sap.insert_at(0, Position{1.f, 1.f});
    cr_assert(sap.contains(0));
    cr_assert_not(sav.contains(0));
    cr_assert_eq(sav.size(), 0u);
}

Test(sparse_array, tag_component_zero_sized_still_tracked)
{
    SparseArray<Tag> sa;
    sa.insert_at(0, Tag{});
    cr_assert(sa.contains(0));
    cr_assert_eq(sa.size(), 1u);
}

Test(component_type, id_is_stable_for_same_type)
{
    std::size_t id1 = ComponentType::get_id<Position>();
    std::size_t id2 = ComponentType::get_id<Position>();
    cr_assert_eq(id1, id2);
}

Test(component_type, ids_differ_between_types)
{
    std::size_t idp = ComponentType::get_id<Position>();
    std::size_t idv = ComponentType::get_id<Velocity>();
    cr_assert_neq(idp, idv);
}

Test(component_type, three_distinct_types_have_pairwise_distinct_ids)
{
    std::size_t idp = ComponentType::get_id<Position>();
    std::size_t idv = ComponentType::get_id<Velocity>();
    std::size_t idh = ComponentType::get_id<Health>();
    cr_assert_neq(idp, idv);
    cr_assert_neq(idv, idh);
    cr_assert_neq(idp, idh);
}

Test(registry_entities, spawn_entity_returns_increasing_ids)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    std::size_t e2 = reg.spawn_entity();
    cr_assert_neq(e0, e1);
    cr_assert_neq(e1, e2);
    cr_assert_neq(e0, e2);
}

Test(registry_entities, kill_entity_allows_id_recycling)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    reg.kill_entity(e0);
    std::size_t e1 = reg.spawn_entity();
    cr_assert_eq(e0, e1);
}

Test(registry_entities, recycled_id_reused_before_new_one_allocated)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    reg.kill_entity(e0);
    std::size_t e2 = reg.spawn_entity();
    cr_assert_eq(e2, e0);
    cr_assert_neq(e2, e1);
}

Test(registry_entities, killing_multiple_entities_all_recyclable)
{
    Registry reg;
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    std::size_t e2 = reg.spawn_entity();
    reg.kill_entity(e0);
    reg.kill_entity(e1);
    reg.kill_entity(e2);
    std::size_t n0 = reg.spawn_entity();
    std::size_t n1 = reg.spawn_entity();
    std::size_t n2 = reg.spawn_entity();
    cr_assert(n0 == e0 || n0 == e1 || n0 == e2);
    cr_assert(n1 == e0 || n1 == e1 || n1 == e2);
    cr_assert(n2 == e0 || n2 == e1 || n2 == e2);
    cr_assert_neq(n0, n1);
    cr_assert_neq(n1, n2);
}

Test(registry_entities, kill_entity_removes_data_from_registered_pool)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    reg.kill_entity(e);
    cr_assert_not(reg.has_component<Position>(e));
}

Test(registry_entities, kill_entity_removes_data_from_all_registered_pools)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    reg.get_components<Velocity>().insert_at(e, Velocity{2.f, 2.f});
    reg.kill_entity(e);
    cr_assert_not(reg.has_component<Position>(e));
    cr_assert_not(reg.has_component<Velocity>(e));
}

Test(registry_entities, kill_entity_does_not_affect_other_entities_data)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e0, Position{1.f, 1.f});
    reg.get_components<Position>().insert_at(e1, Position{2.f, 2.f});
    reg.kill_entity(e0);
    cr_assert(reg.has_component<Position>(e1));
}

Test(registry_entities, remove_entity_from_all_pools_clears_registered_components)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Health>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    reg.get_components<Health>().insert_at(e, Health{100});
    reg.remove_entity_from_all_pools(e);
    cr_assert_not(reg.has_component<Position>(e));
    cr_assert_not(reg.has_component<Health>(e));
}

Test(registry_entities, remove_entity_from_all_pools_does_not_kill_entity_id)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    reg.remove_entity_from_all_pools(e);
    std::size_t other = reg.spawn_entity();
    cr_assert_neq(other, e);
}

Test(registry_components, register_component_returns_usable_pool)
{
    Registry reg;
    SparseArray<Position> &pool = reg.register_component<Position>();
    cr_assert_eq(pool.size(), 0u);
}

Test(registry_components, get_components_returns_same_pool_as_register)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    cr_assert(reg.get_components<Position>().contains(e));
}

Test(registry_components, distinct_component_types_have_independent_pools)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    cr_assert(reg.get_components<Position>().contains(e));
    cr_assert_not(reg.get_components<Velocity>().contains(e));
}

Test(registry_components, has_component_true_after_insert)
{
    Registry reg;
    reg.register_component<Health>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Health>().insert_at(e, Health{50});
    cr_assert(reg.has_component<Health>(e));
}

Test(registry_components, has_component_false_when_never_inserted)
{
    Registry reg;
    reg.register_component<Health>();
    std::size_t e = reg.spawn_entity();
    cr_assert_not(reg.has_component<Health>(e));
}

Test(registry_components, has_component_false_for_unrelated_entity)
{
    Registry reg;
    reg.register_component<Health>();
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    reg.get_components<Health>().insert_at(e0, Health{50});
    cr_assert_not(reg.has_component<Health>(e1));
}

Test(registry_components, has_component_false_after_erase_via_pool)
{
    Registry reg;
    reg.register_component<Health>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Health>().insert_at(e, Health{50});
    reg.get_components<Health>().erase(e);
    cr_assert_not(reg.has_component<Health>(e));
}

Test(registry_components, insert_overwrite_through_registry_updates_value)
{
    Registry reg;
    reg.register_component<Health>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<Health>().insert_at(e, Health{50});
    reg.get_components<Health>().insert_at(e, Health{75});
    auto v = reg.get_components<Health>().get(e);
    cr_assert(v.has_value());
    cr_assert_eq(v->hp, 75);
}


Test(registry_view, single_component_view_iterates_all_holders)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e0, Position{1.f, 1.f});
    reg.get_components<Position>().insert_at(e1, Position{2.f, 2.f});

    std::size_t count = 0;
    for (auto entity : reg.view<Position>()) {
        (void)entity;
        ++count;
    }
    cr_assert_eq(count, 2u);
}

Test(registry_view, single_component_view_empty_when_no_data)
{
    Registry reg;
    reg.register_component<Position>();
    reg.spawn_entity();

    std::size_t count = 0;
    for (auto entity : reg.view<Position>()) {
        (void)entity;
        ++count;
    }
    cr_assert_eq(count, 0u);
}

Test(registry_view, two_component_view_only_intersection)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();

    std::size_t both = reg.spawn_entity();
    std::size_t only_pos = reg.spawn_entity();
    std::size_t only_vel = reg.spawn_entity();

    reg.get_components<Position>().insert_at(both, Position{0.f, 0.f});
    reg.get_components<Velocity>().insert_at(both, Velocity{1.f, 1.f});

    reg.get_components<Position>().insert_at(only_pos, Position{0.f, 0.f});
    reg.get_components<Velocity>().insert_at(only_vel, Velocity{1.f, 1.f});

    std::size_t count = 0;
    bool saw_both = false;
    for (auto entity : reg.view<Position, Velocity>()) {
        ++count;
        if (entity == both)
            saw_both = true;
    }
    cr_assert_eq(count, 1u);
    cr_assert(saw_both);
}

Test(registry_view, two_component_view_skips_entity_missing_second_component)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();

    std::size_t only_pos = reg.spawn_entity();
    reg.get_components<Position>().insert_at(only_pos, Position{0.f, 0.f});

    bool wrongly_included = false;
    for (auto entity : reg.view<Position, Velocity>()) {
        (void)entity;
        wrongly_included = true;
    }
    cr_assert_not(wrongly_included,
        "view<Position, Velocity> should not yield an entity lacking Velocity");
}

Test(registry_view, three_component_view_requires_all_three)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();
    reg.register_component<Health>();

    std::size_t full = reg.spawn_entity();
    std::size_t missing_health = reg.spawn_entity();

    reg.get_components<Position>().insert_at(full, Position{0.f, 0.f});
    reg.get_components<Velocity>().insert_at(full, Velocity{1.f, 1.f});
    reg.get_components<Health>().insert_at(full, Health{100});

    reg.get_components<Position>().insert_at(missing_health, Position{0.f, 0.f});
    reg.get_components<Velocity>().insert_at(missing_health, Velocity{1.f, 1.f});

    std::size_t count = 0;
    for (auto entity : reg.view<Position, Velocity, Health>()) {
        cr_assert_eq(entity, full);
        ++count;
    }
    cr_assert_eq(count, 1u);
}

Test(registry_view, view_excludes_killed_entity)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e0, Position{0.f, 0.f});
    reg.get_components<Position>().insert_at(e1, Position{1.f, 1.f});

    reg.kill_entity(e0);

    std::size_t count = 0;
    for (auto entity : reg.view<Position>()) {
        cr_assert_eq(entity, e1);
        ++count;
    }
    cr_assert_eq(count, 1u);
}

Test(registry_view, view_reflects_runtime_erase_via_pool)
{
    Registry reg;
    reg.register_component<Position>();
    std::size_t e0 = reg.spawn_entity();
    std::size_t e1 = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e0, Position{0.f, 0.f});
    reg.get_components<Position>().insert_at(e1, Position{1.f, 1.f});

    reg.get_components<Position>().erase(e0);

    std::size_t count = 0;
    for (auto entity : reg.view<Position>()) {
        cr_assert_eq(entity, e1);
        ++count;
    }
    cr_assert_eq(count, 1u);
}

Test(registry_view, empty_registry_view_yields_nothing)
{
    Registry reg;
    reg.register_component<Position>();

    std::size_t count = 0;
    for (auto entity : reg.view<Position>()) {
        (void)entity;
        ++count;
    }
    cr_assert_eq(count, 0u);
}

Test(registry_view, view_with_many_entities_correct_count)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Health>();

    for (int i = 0; i < 20; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<Position>().insert_at(e, Position{(float)i, (float)i});
        if (i % 2 == 0)
            reg.get_components<Health>().insert_at(e, Health{i});
    }

    std::size_t count = 0;
    for (auto entity : reg.view<Position, Health>()) {
        (void)entity;
        ++count;
    }
    cr_assert_eq(count, 10u);
}

Test(registry_integration, full_lifecycle_spawn_attach_detach_kill)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();

    std::size_t e = reg.spawn_entity();
    reg.get_components<Position>().insert_at(e, Position{1.f, 2.f});
    reg.get_components<Velocity>().insert_at(e, Velocity{3.f, 4.f});

    cr_assert(reg.has_component<Position>(e));
    cr_assert(reg.has_component<Velocity>(e));

    reg.get_components<Velocity>().erase(e);
    cr_assert_not(reg.has_component<Velocity>(e));
    cr_assert(reg.has_component<Position>(e));

    reg.kill_entity(e);
    cr_assert_not(reg.has_component<Position>(e));

    std::size_t e2 = reg.spawn_entity();
    cr_assert_eq(e2, e);
    cr_assert_not(reg.has_component<Position>(e2));
}

Test(registry_integration, spawn_kill_spawn_keeps_pool_consistent)
{
    Registry reg;
    reg.register_component<Health>();

    std::size_t e0 = reg.spawn_entity();
    reg.get_components<Health>().insert_at(e0, Health{10});

    reg.kill_entity(e0);

    std::size_t e1 = reg.spawn_entity();
    cr_assert_not(reg.has_component<Health>(e1));

    reg.get_components<Health>().insert_at(e1, Health{99});
    auto v = reg.get_components<Health>().get(e1);
    cr_assert(v.has_value());
    cr_assert_eq(v->hp, 99);
}

Test(registry_integration, many_entities_mixed_components_view_stays_correct_after_kills)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();

    std::size_t alive_both = reg.spawn_entity();
    std::size_t dead_both = reg.spawn_entity();
    std::size_t alive_pos_only = reg.spawn_entity();

    reg.get_components<Position>().insert_at(alive_both, Position{0.f, 0.f});
    reg.get_components<Velocity>().insert_at(alive_both, Velocity{0.f, 0.f});

    reg.get_components<Position>().insert_at(dead_both, Position{0.f, 0.f});
    reg.get_components<Velocity>().insert_at(dead_both, Velocity{0.f, 0.f});

    reg.get_components<Position>().insert_at(alive_pos_only, Position{0.f, 0.f});

    reg.kill_entity(dead_both);

    std::size_t count = 0;
    for (auto entity : reg.view<Position, Velocity>()) {
        cr_assert_eq(entity, alive_both);
        ++count;
    }
    cr_assert_eq(count, 1u);
}

Test(registry_integration, stress_spawn_and_kill_hundreds_of_entities)
{
    Registry reg;
    reg.register_component<Position>();

    std::vector<std::size_t> ids;
    for (int i = 0; i < 200; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<Position>().insert_at(e, Position{(float)i, (float)i});
        ids.push_back(e);
    }
    for (std::size_t i = 0; i < ids.size(); i += 2)
        reg.kill_entity(ids[i]);

    std::size_t count = 0;
    for (auto entity : reg.view<Position>()) {
        (void)entity;
        ++count;
    }
    cr_assert_eq(count, 100u);
}

Test(chrono_perf, spawn_entity_10k)
{
    Registry reg;
    {
        PROFILE_SCOPE("spawn_entity_10k");
        for (int i = 0; i < 10000; ++i)
            reg.spawn_entity();
    }
    cr_assert(true);
}

Test(chrono_perf, insert_at_10k)
{
    Registry reg;
    reg.register_component<Position>();
    std::vector<std::size_t> ids;
    for (int i = 0; i < 10000; ++i)
        ids.push_back(reg.spawn_entity());

    {
        PROFILE_SCOPE("insert_at_10k");
        for (auto e : ids)
            reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
    }
    cr_assert_eq(reg.get_components<Position>().size(), 10000u);
}

Test(chrono_perf, view_single_component_10k)
{
    Registry reg;
    reg.register_component<Position>();
    for (int i = 0; i < 10000; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<Position>().insert_at(e, Position{(float)i, (float)i});
    }

    std::size_t count = 0;
    {
        PROFILE_SCOPE("view_single_component_10k");
        for (auto entity : reg.view<Position>()) {
            (void)entity;
            ++count;
        }
    }
    cr_assert_eq(count, 10000u);
}

Test(chrono_perf, view_two_component_intersection_10k)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();
    for (int i = 0; i < 10000; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<Position>().insert_at(e, Position{(float)i, (float)i});
        if (i % 2 == 0)
            reg.get_components<Velocity>().insert_at(e, Velocity{1.f, 1.f});
    }

    std::size_t count = 0;
    {
        PROFILE_SCOPE("view_two_component_intersection_10k");
        for (auto entity : reg.view<Position, Velocity>()) {
            (void)entity;
            ++count;
        }
    }
    cr_assert_eq(count, 5000u);
}

Test(chrono_perf, sparse_array_get_lookup_10k)
{
    SparseArray<Position> sa;
    for (std::size_t i = 0; i < 10000; ++i)
        sa.insert_at(i, Position{(float)i, (float)i});

    std::size_t found = 0;
    {
        PROFILE_SCOPE("sparse_array_get_lookup_10k");
        for (std::size_t i = 0; i < 10000; ++i)
            if (sa.get(i).has_value())
                ++found;
    }
    cr_assert_eq(found, 10000u);
}

Test(chrono_perf, sparse_array_erase_10k)
{
    SparseArray<Position> sa;
    for (std::size_t i = 0; i < 10000; ++i)
        sa.insert_at(i, Position{(float)i, (float)i});

    {
        PROFILE_SCOPE("sparse_array_erase_10k");
        for (std::size_t i = 10000; i-- > 0;)
            sa.erase(i);
    }
    cr_assert_eq(sa.size(), 0u);
}

Test(chrono_perf, kill_entity_10k)
{
    Registry reg;
    reg.register_component<Position>();
    reg.register_component<Health>();
    std::vector<std::size_t> ids;
    for (int i = 0; i < 10000; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<Position>().insert_at(e, Position{1.f, 1.f});
        reg.get_components<Health>().insert_at(e, Health{100});
        ids.push_back(e);
    }

    {
        PROFILE_SCOPE("kill_entity_10k");
        for (auto e : ids)
            reg.kill_entity(e);
    }
    cr_assert_eq(reg.get_components<Position>().size(), 0u);
    cr_assert_eq(reg.get_components<Health>().size(), 0u);
}

Test(chrono_perf, scoped_timer_measures_sleep)
{
    long duration_us = 0;
    {
        TIME_START(sleep_check);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto end = std::chrono::high_resolution_clock::now();
        duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start_sleep_check).count();
    }
    cr_assert_geq(duration_us, 4000);
}