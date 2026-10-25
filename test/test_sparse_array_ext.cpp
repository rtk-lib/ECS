#include <criterion/criterion.h>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "SparseArray.hpp"

using namespace rtk::ecs;

namespace {
    struct Pos {
        float x;
        float y;
    };

    struct Hp {
        int value;
    };

    struct Name {
        std::string value;
    };

    struct Big {
        int data[64];
    };

    struct Counted {
        static inline int copies = 0;
        int v = 0;
        Counted() = default;
        explicit Counted(int val) : v(val) {}
        Counted(Counted const &o) : v(o.v) { ++copies; }
        Counted &operator=(Counted const &o) { v = o.v; ++copies; return *this; }
        Counted(Counted &&o) noexcept = default;
        Counted &operator=(Counted &&o) noexcept = default;
    };
}

Test(sparse_array_ext, default_constructed_has_empty_internal_vectors)
{
    SparseArray<Pos> sa;
    cr_assert(sa._sparse.empty());
    cr_assert(sa._dense.empty());
    cr_assert(sa._packed_entities.empty());
}

Test(sparse_array_ext, packed_array_empty_by_default)
{
    SparseArray<Pos> sa;
    cr_assert(sa.get_packed_array().empty());
}

Test(sparse_array_ext, null_entity_is_max_size_t)
{
    cr_assert_eq(NULL_ENTITY, static_cast<std::size_t>(-1));
}

Test(sparse_array_ext, insert_at_zero_sizes_sparse_to_one)
{
    SparseArray<Pos> sa;
    sa.insert_at(0, Pos{1.f, 1.f});
    cr_assert_eq(sa._sparse.size(), 1u);
}

Test(sparse_array_ext, insert_at_n_sizes_sparse_to_n_plus_one)
{
    SparseArray<Pos> sa;
    sa.insert_at(41, Pos{1.f, 1.f});
    cr_assert_eq(sa._sparse.size(), 42u);
}

Test(sparse_array_ext, insert_lower_index_does_not_shrink_sparse)
{
    SparseArray<Pos> sa;
    sa.insert_at(20, Pos{1.f, 1.f});
    sa.insert_at(3, Pos{1.f, 1.f});
    cr_assert_eq(sa._sparse.size(), 21u);
}

Test(sparse_array_ext, gaps_in_sparse_are_null_entity)
{
    SparseArray<Pos> sa;
    sa.insert_at(5, Pos{1.f, 1.f});
    for (std::size_t i = 0; i < 5; ++i)
        cr_assert_eq(sa._sparse[i], NULL_ENTITY);
}

Test(sparse_array_ext, sparse_slot_points_to_dense_index)
{
    SparseArray<Pos> sa;
    sa.insert_at(7, Pos{1.f, 1.f});
    sa.insert_at(2, Pos{2.f, 2.f});
    cr_assert_eq(sa._sparse[7], 0u);
    cr_assert_eq(sa._sparse[2], 1u);
}

Test(sparse_array_ext, dense_and_packed_have_same_size)
{
    SparseArray<Pos> sa;
    for (std::size_t i = 0; i < 9; ++i)
        sa.insert_at(i * 3, Pos{0.f, 0.f});
    cr_assert_eq(sa._dense.size(), sa._packed_entities.size());
}

Test(sparse_array_ext, packed_array_keeps_insertion_order)
{
    SparseArray<Pos> sa;
    sa.insert_at(9, Pos{});
    sa.insert_at(1, Pos{});
    sa.insert_at(4, Pos{});
    auto const &p = sa.get_packed_array();
    cr_assert_eq(p.size(), 3u);
    cr_assert_eq(p[0], 9u);
    cr_assert_eq(p[1], 1u);
    cr_assert_eq(p[2], 4u);
}

Test(sparse_array_ext, packed_array_is_reference_to_internal_storage)
{
    SparseArray<Pos> sa;
    auto const &p = sa.get_packed_array();
    sa.insert_at(3, Pos{});
    cr_assert_eq(p.size(), 1u);
    cr_assert_eq(&p, &sa._packed_entities);
}

Test(sparse_array_ext, dense_values_keep_insertion_order)
{
    SparseArray<Hp> sa;
    sa.insert_at(5, Hp{50});
    sa.insert_at(0, Hp{10});
    sa.insert_at(3, Hp{30});
    cr_assert_eq(sa._dense[0].value, 50);
    cr_assert_eq(sa._dense[1].value, 10);
    cr_assert_eq(sa._dense[2].value, 30);
}

Test(sparse_array_ext, insert_returns_reference_into_dense)
{
    SparseArray<Hp> sa;
    Hp &ref = sa.insert_at(2, Hp{5});
    cr_assert_eq(&ref, &sa._dense.back());
}

Test(sparse_array_ext, insert_returned_reference_is_mutable)
{
    SparseArray<Hp> sa;
    Hp &ref = sa.insert_at(2, Hp{5});
    ref.value = 77;
    cr_assert_eq(sa.get(2)->value, 77);
}

Test(sparse_array_ext, overwrite_returns_reference_to_same_slot)
{
    SparseArray<Hp> sa;
    sa.insert_at(1, Hp{1});
    sa.insert_at(2, Hp{2});
    Hp &ref = sa.insert_at(1, Hp{9});
    cr_assert_eq(&ref, &sa._dense[0]);
    cr_assert_eq(ref.value, 9);
}

Test(sparse_array_ext, overwrite_does_not_change_packed_order)
{
    SparseArray<Hp> sa;
    sa.insert_at(4, Hp{1});
    sa.insert_at(8, Hp{2});
    sa.insert_at(4, Hp{3});
    cr_assert_eq(sa.get_packed_array()[0], 4u);
    cr_assert_eq(sa.get_packed_array()[1], 8u);
}

Test(sparse_array_ext, overwrite_does_not_touch_other_entities)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{1});
    sa.insert_at(1, Hp{2});
    sa.insert_at(0, Hp{100});
    cr_assert_eq(sa.get(1)->value, 2);
}

Test(sparse_array_ext, overwrite_many_times_keeps_size_one)
{
    SparseArray<Hp> sa;
    for (int i = 0; i < 50; ++i)
        sa.insert_at(3, Hp{i});
    cr_assert_eq(sa.size(), 1u);
    cr_assert_eq(sa.get(3)->value, 49);
}

Test(sparse_array_ext, insert_copies_the_component)
{
    SparseArray<Hp> sa;
    Hp original{12};
    sa.insert_at(0, original);
    original.value = 0;
    cr_assert_eq(sa.get(0)->value, 12);
}

Test(sparse_array_ext, insert_from_const_lvalue)
{
    SparseArray<Hp> sa;
    const Hp c{8};
    sa.insert_at(0, c);
    cr_assert_eq(sa.get(0)->value, 8);
}

Test(sparse_array_ext, insert_string_component)
{
    SparseArray<Name> sa;
    sa.insert_at(0, Name{"player"});
    cr_assert_str_eq(sa.get(0)->value.c_str(), "player");
}

Test(sparse_array_ext, overwrite_string_component)
{
    SparseArray<Name> sa;
    sa.insert_at(0, Name{"a"});
    sa.insert_at(0, Name{"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"});
    cr_assert_str_eq(sa.get(0)->value.c_str(), "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
}

Test(sparse_array_ext, insert_big_component)
{
    SparseArray<Big> sa;
    Big b{};
    for (int i = 0; i < 64; ++i)
        b.data[i] = i * 2;
    sa.insert_at(3, b);
    for (int i = 0; i < 64; ++i)
        cr_assert_eq(sa.get(3)->data[i], i * 2);
}

Test(sparse_array_ext, insert_int_component)
{
    SparseArray<int> sa;
    sa.insert_at(2, 42);
    cr_assert_eq(*sa.get(2), 42);
}

Test(sparse_array_ext, insert_double_component)
{
    SparseArray<double> sa;
    sa.insert_at(0, 3.5);
    cr_assert_float_eq(*sa.get(0), 3.5, 1e-9);
}

Test(sparse_array_ext, insert_vector_component)
{
    SparseArray<std::vector<int>> sa;
    sa.insert_at(1, std::vector<int>{1, 2, 3});
    cr_assert_eq(sa.get(1)->size(), 3u);
    cr_assert_eq((*sa.get(1))[2], 3);
}

Test(sparse_array_ext, insert_shared_ptr_component_shares_ownership)
{
    SparseArray<std::shared_ptr<int>> sa;
    auto p = std::make_shared<int>(5);
    sa.insert_at(0, p);
    cr_assert_eq(p.use_count(), 2);
}

Test(sparse_array_ext, erase_shared_ptr_component_releases_ownership)
{
    SparseArray<std::shared_ptr<int>> sa;
    auto p = std::make_shared<int>(5);
    sa.insert_at(0, p);
    sa.erase(0);
    cr_assert_eq(p.use_count(), 1);
}

Test(sparse_array_ext, destructor_releases_shared_ptr_components)
{
    auto p = std::make_shared<int>(5);
    {
        SparseArray<std::shared_ptr<int>> sa;
        sa.insert_at(0, p);
        sa.insert_at(1, p);
        cr_assert_eq(p.use_count(), 3);
    }
    cr_assert_eq(p.use_count(), 1);
}

Test(sparse_array_ext, insert_copies_exactly_once)
{
    SparseArray<Counted> sa;
    sa._dense.reserve(4);
    Counted::copies = 0;
    sa.insert_at(0, Counted{1});
    cr_assert_eq(Counted::copies, 1);
}

Test(sparse_array_ext, overwrite_uses_copy_assignment_once)
{
    SparseArray<Counted> sa;
    sa.insert_at(0, Counted{1});
    Counted::copies = 0;
    sa.insert_at(0, Counted{2});
    cr_assert_eq(Counted::copies, 1);
    cr_assert_eq(sa.get(0)->v, 2);
}

Test(sparse_array_ext, erase_moves_instead_of_copying)
{
    SparseArray<Counted> sa;
    sa.insert_at(0, Counted{1});
    sa.insert_at(1, Counted{2});
    Counted::copies = 0;
    sa.erase(0);
    cr_assert_eq(Counted::copies, 0);
    cr_assert_eq(sa.get(1)->v, 2);
}

Test(sparse_array_ext, get_returns_nullptr_out_of_range)
{
    SparseArray<Pos> sa;
    sa.insert_at(2, Pos{});
    cr_assert_null(sa.get(1000));
}

Test(sparse_array_ext, get_returns_nullptr_for_gap)
{
    SparseArray<Pos> sa;
    sa.insert_at(4, Pos{});
    cr_assert_null(sa.get(2));
}

Test(sparse_array_ext, get_returns_nullptr_for_null_entity_id)
{
    SparseArray<Pos> sa;
    sa.insert_at(0, Pos{});
    cr_assert_null(sa.get(NULL_ENTITY));
}

Test(sparse_array_ext, get_returns_pointer_into_dense)
{
    SparseArray<Pos> sa;
    sa.insert_at(4, Pos{});
    cr_assert_eq(sa.get(4), &sa._dense[0]);
}

Test(sparse_array_ext, get_pointer_allows_mutation)
{
    SparseArray<Pos> sa;
    sa.insert_at(4, Pos{1.f, 2.f});
    sa.get(4)->x = 10.f;
    cr_assert_float_eq(sa._dense[0].x, 10.f, 1e-6);
}

Test(sparse_array_ext, const_get_returns_value)
{
    SparseArray<Hp> sa;
    sa.insert_at(3, Hp{33});
    SparseArray<Hp> const &csa = sa;
    cr_assert_not_null(csa.get(3));
    cr_assert_eq(csa.get(3)->value, 33);
}

Test(sparse_array_ext, const_get_returns_nullptr_when_missing)
{
    SparseArray<Hp> sa;
    SparseArray<Hp> const &csa = sa;
    cr_assert_null(csa.get(3));
}

Test(sparse_array_ext, const_get_and_get_point_to_same_object)
{
    SparseArray<Hp> sa;
    sa.insert_at(3, Hp{33});
    SparseArray<Hp> const &csa = sa;
    cr_assert_eq(static_cast<const Hp *>(sa.get(3)), csa.get(3));
}

Test(sparse_array_ext, const_contains_works)
{
    SparseArray<Hp> sa;
    sa.insert_at(1, Hp{1});
    SparseArray<Hp> const &csa = sa;
    cr_assert(csa.contains(1));
    cr_assert_not(csa.contains(0));
}

Test(sparse_array_ext, const_size_works)
{
    SparseArray<Hp> sa;
    sa.insert_at(1, Hp{1});
    sa.insert_at(2, Hp{1});
    SparseArray<Hp> const &csa = sa;
    cr_assert_eq(csa.size(), 2u);
}

Test(sparse_array_ext, contains_null_entity_is_false)
{
    SparseArray<Pos> sa;
    sa.insert_at(0, Pos{});
    cr_assert_not(sa.contains(NULL_ENTITY));
}

Test(sparse_array_ext, contains_out_of_range_is_false)
{
    SparseArray<Pos> sa;
    sa.insert_at(0, Pos{});
    cr_assert_not(sa.contains(1));
}

Test(sparse_array_ext, erase_first_moves_last_into_slot_zero)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{0});
    sa.insert_at(1, Hp{1});
    sa.insert_at(2, Hp{2});
    sa.erase(0);
    cr_assert_eq(sa._dense[0].value, 2);
    cr_assert_eq(sa._packed_entities[0], 2u);
}

Test(sparse_array_ext, erase_updates_sparse_of_moved_entity)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{0});
    sa.insert_at(1, Hp{1});
    sa.insert_at(2, Hp{2});
    sa.erase(0);
    cr_assert_eq(sa._sparse[2], 0u);
}

Test(sparse_array_ext, erase_sets_sparse_slot_to_null)
{
    SparseArray<Hp> sa;
    sa.insert_at(3, Hp{0});
    sa.erase(3);
    cr_assert_eq(sa._sparse[3], NULL_ENTITY);
}

Test(sparse_array_ext, erase_does_not_shrink_sparse)
{
    SparseArray<Hp> sa;
    sa.insert_at(10, Hp{0});
    sa.erase(10);
    cr_assert_eq(sa._sparse.size(), 11u);
}

Test(sparse_array_ext, erase_last_does_not_reorder)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{0});
    sa.insert_at(1, Hp{1});
    sa.insert_at(2, Hp{2});
    sa.erase(2);
    cr_assert_eq(sa._packed_entities[0], 0u);
    cr_assert_eq(sa._packed_entities[1], 1u);
}

Test(sparse_array_ext, erase_twice_is_noop_second_time)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{0});
    sa.insert_at(1, Hp{1});
    sa.erase(0);
    sa.erase(0);
    cr_assert_eq(sa.size(), 1u);
    cr_assert(sa.contains(1));
}

Test(sparse_array_ext, erase_gap_entity_is_noop)
{
    SparseArray<Hp> sa;
    sa.insert_at(5, Hp{5});
    sa.erase(2);
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array_ext, erase_null_entity_is_noop)
{
    SparseArray<Hp> sa;
    sa.insert_at(5, Hp{5});
    sa.erase(NULL_ENTITY);
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array_ext, erase_all_in_insertion_order)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 0; i < 16; ++i)
        sa.insert_at(i, Hp{static_cast<int>(i)});
    for (std::size_t i = 0; i < 16; ++i)
        sa.erase(i);
    cr_assert_eq(sa.size(), 0u);
    cr_assert(sa.get_packed_array().empty());
}

Test(sparse_array_ext, erase_all_in_reverse_order)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 0; i < 16; ++i)
        sa.insert_at(i, Hp{static_cast<int>(i)});
    for (std::size_t i = 16; i-- > 0;)
        sa.erase(i);
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array_ext, erase_keeps_values_associated_to_right_entities)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 0; i < 10; ++i)
        sa.insert_at(i, Hp{static_cast<int>(i * 10)});
    sa.erase(3);
    sa.erase(7);
    sa.erase(0);
    for (std::size_t i = 0; i < 10; ++i) {
        if (i == 3 || i == 7 || i == 0) {
            cr_assert_null(sa.get(i));
        } else {
            cr_assert_eq(sa.get(i)->value, static_cast<int>(i * 10));
        }
    }
}

Test(sparse_array_ext, erase_then_insert_appends_at_end_of_dense)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{0});
    sa.insert_at(1, Hp{1});
    sa.erase(0);
    sa.insert_at(0, Hp{5});
    cr_assert_eq(sa._packed_entities.back(), 0u);
    cr_assert_eq(sa._sparse[0], 1u);
}

Test(sparse_array_ext, erase_string_component_keeps_moved_string)
{
    SparseArray<Name> sa;
    sa.insert_at(0, Name{"first"});
    sa.insert_at(1, Name{"second"});
    sa.erase(0);
    cr_assert_str_eq(sa.get(1)->value.c_str(), "second");
}

Test(sparse_array_ext, sparse_and_packed_stay_coherent_after_random_ops)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 0; i < 30; ++i)
        sa.insert_at(i, Hp{static_cast<int>(i)});
    for (std::size_t i = 0; i < 30; i += 3)
        sa.erase(i);
    for (std::size_t i = 0; i < 30; i += 6)
        sa.insert_at(i, Hp{static_cast<int>(i + 100)});
    for (std::size_t d = 0; d < sa._dense.size(); ++d)
        cr_assert_eq(sa._sparse[sa._packed_entities[d]], d);
}

Test(sparse_array_ext, packed_entities_are_unique)
{
    SparseArray<Hp> sa;
    for (int round = 0; round < 3; ++round)
        for (std::size_t i = 0; i < 10; ++i)
            sa.insert_at(i, Hp{round});
    auto p = sa.get_packed_array();
    std::sort(p.begin(), p.end());
    cr_assert(std::adjacent_find(p.begin(), p.end()) == p.end());
}

Test(sparse_array_ext, dense_index_valid_for_every_contained_entity)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 0; i < 20; i += 2)
        sa.insert_at(i, Hp{0});
    for (std::size_t i = 0; i < sa._sparse.size(); ++i)
        if (sa.contains(i))
            cr_assert_lt(sa._sparse[i], sa._dense.size());
}

Test(sparse_array_ext, copy_constructor_copies_components)
{
    SparseArray<Hp> a;
    a.insert_at(2, Hp{22});
    SparseArray<Hp> b(a);
    cr_assert_eq(b.get(2)->value, 22);
}

Test(sparse_array_ext, copy_is_independent_from_original)
{
    SparseArray<Hp> a;
    a.insert_at(2, Hp{22});
    SparseArray<Hp> b(a);
    b.get(2)->value = 1;
    b.erase(2);
    cr_assert_eq(a.get(2)->value, 22);
}

Test(sparse_array_ext, move_constructor_transfers_components)
{
    SparseArray<Hp> a;
    a.insert_at(2, Hp{22});
    SparseArray<Hp> b(std::move(a));
    cr_assert_eq(b.get(2)->value, 22);
    cr_assert_eq(b.size(), 1u);
}

Test(sparse_array_ext, copy_assignment_replaces_content)
{
    SparseArray<Hp> a;
    SparseArray<Hp> b;
    a.insert_at(1, Hp{1});
    b.insert_at(5, Hp{5});
    b = a;
    cr_assert(b.contains(1));
    cr_assert_not(b.contains(5));
}

Test(sparse_array_ext, very_sparse_ids_all_retrievable)
{
    SparseArray<Hp> sa;
    std::size_t ids[] = {0, 17, 256, 1024, 4095, 9999};
    for (auto id : ids)
        sa.insert_at(id, Hp{static_cast<int>(id)});
    for (auto id : ids)
        cr_assert_eq(sa.get(id)->value, static_cast<int>(id));
    cr_assert_eq(sa.size(), 6u);
}

Test(sparse_array_ext, size_is_independent_from_highest_id)
{
    SparseArray<Hp> sa;
    sa.insert_at(100000, Hp{1});
    cr_assert_eq(sa.size(), 1u);
}

Test(sparse_array_ext, fill_100k_and_check_contains)
{
    SparseArray<int> sa;
    for (std::size_t i = 0; i < 100000; ++i)
        sa.insert_at(i, static_cast<int>(i));
    cr_assert_eq(sa.size(), 100000u);
    cr_assert(sa.contains(0));
    cr_assert(sa.contains(99999));
    cr_assert_not(sa.contains(100000));
}

Test(sparse_array_ext, sum_of_dense_matches_inserted_values)
{
    SparseArray<int> sa;
    long expected = 0;
    for (int i = 0; i < 500; ++i) {
        sa.insert_at(static_cast<std::size_t>(i) * 2, i);
        expected += i;
    }
    long sum = 0;
    for (int v : sa._dense)
        sum += v;
    cr_assert_eq(sum, expected);
}

Test(sparse_array_ext, iterate_packed_and_get_matches_dense)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 0; i < 25; ++i)
        sa.insert_at(i * 7, Hp{static_cast<int>(i)});
    auto const &p = sa.get_packed_array();
    for (std::size_t d = 0; d < p.size(); ++d)
        cr_assert_eq(sa.get(p[d]), &sa._dense[d]);
}

Test(sparse_array_ext, two_arrays_same_type_are_independent)
{
    SparseArray<Hp> a;
    SparseArray<Hp> b;
    a.insert_at(0, Hp{1});
    cr_assert_not(b.contains(0));
    b.insert_at(0, Hp{2});
    cr_assert_eq(a.get(0)->value, 1);
    cr_assert_eq(b.get(0)->value, 2);
}

Test(sparse_array_ext, erase_in_one_array_does_not_affect_other)
{
    SparseArray<Hp> a;
    SparseArray<Hp> b;
    a.insert_at(0, Hp{1});
    b.insert_at(0, Hp{2});
    a.erase(0);
    cr_assert(b.contains(0));
}

Test(sparse_array_ext, bool_component_supported)
{
    SparseArray<char> sa;
    sa.insert_at(0, 'y');
    sa.insert_at(1, 'n');
    cr_assert_eq(*sa.get(0), 'y');
    cr_assert_eq(*sa.get(1), 'n');
}

Test(sparse_array_ext, pointer_from_get_invalidated_semantics_value_preserved)
{
    SparseArray<Hp> sa;
    sa.insert_at(0, Hp{1});
    for (std::size_t i = 1; i < 1000; ++i)
        sa.insert_at(i, Hp{static_cast<int>(i)});
    cr_assert_eq(sa.get(0)->value, 1);
}

Test(sparse_array_ext, alternate_insert_erase_same_entity)
{
    SparseArray<Hp> sa;
    for (int i = 0; i < 100; ++i) {
        sa.insert_at(4, Hp{i});
        cr_assert(sa.contains(4));
        sa.erase(4);
        cr_assert_not(sa.contains(4));
    }
    cr_assert_eq(sa.size(), 0u);
}

Test(sparse_array_ext, reverse_insertion_order_keeps_packed_reverse)
{
    SparseArray<Hp> sa;
    for (std::size_t i = 5; i-- > 0;)
        sa.insert_at(i, Hp{0});
    for (std::size_t d = 0; d < 5; ++d)
        cr_assert_eq(sa.get_packed_array()[d], 4 - d);
}

#define SA_FILL_TEST(N) \
    Test(sparse_array_param, fill_and_read_##N) \
    { \
        SparseArray<Hp> sa; \
        for (std::size_t i = 0; i < (N); ++i) \
            sa.insert_at(i, Hp{static_cast<int>(i) * 3}); \
        cr_assert_eq(sa.size(), static_cast<std::size_t>(N)); \
        for (std::size_t i = 0; i < (N); ++i) { \
            cr_assert(sa.contains(i)); \
            cr_assert_eq(sa.get(i)->value, static_cast<int>(i) * 3); \
        } \
        cr_assert_not(sa.contains(N)); \
    }

SA_FILL_TEST(1)
SA_FILL_TEST(2)
SA_FILL_TEST(3)
SA_FILL_TEST(7)
SA_FILL_TEST(16)
SA_FILL_TEST(64)
SA_FILL_TEST(100)
SA_FILL_TEST(255)
SA_FILL_TEST(1000)
SA_FILL_TEST(4096)

#define SA_ERASE_EVEN_TEST(N) \
    Test(sparse_array_param, erase_even_ids_##N) \
    { \
        SparseArray<Hp> sa; \
        for (std::size_t i = 0; i < (N); ++i) \
            sa.insert_at(i, Hp{static_cast<int>(i)}); \
        for (std::size_t i = 0; i < (N); i += 2) \
            sa.erase(i); \
        cr_assert_eq(sa.size(), static_cast<std::size_t>(N) / 2); \
        for (std::size_t i = 0; i < (N); ++i) { \
            if (i % 2 == 0) { \
                cr_assert_null(sa.get(i)); \
            } else { \
                cr_assert_eq(sa.get(i)->value, static_cast<int>(i)); \
            } \
        } \
    }

SA_ERASE_EVEN_TEST(2)
SA_ERASE_EVEN_TEST(4)
SA_ERASE_EVEN_TEST(8)
SA_ERASE_EVEN_TEST(10)
SA_ERASE_EVEN_TEST(32)
SA_ERASE_EVEN_TEST(50)
SA_ERASE_EVEN_TEST(128)
SA_ERASE_EVEN_TEST(500)
SA_ERASE_EVEN_TEST(1024)
SA_ERASE_EVEN_TEST(2000)

#define SA_STRIDE_TEST(S) \
    Test(sparse_array_param, stride_insert_##S) \
    { \
        SparseArray<Hp> sa; \
        for (std::size_t i = 0; i < 50; ++i) \
            sa.insert_at(i * (S), Hp{static_cast<int>(i)}); \
        cr_assert_eq(sa.size(), 50u); \
        cr_assert_eq(sa._sparse.size(), 49u * (S) + 1); \
        for (std::size_t i = 0; i < 50; ++i) \
            cr_assert_eq(sa.get(i * (S))->value, static_cast<int>(i)); \
        if ((S) > 1) { \
            cr_assert_not(sa.contains(1)); \
        } \
    }

SA_STRIDE_TEST(1)
SA_STRIDE_TEST(2)
SA_STRIDE_TEST(3)
SA_STRIDE_TEST(5)
SA_STRIDE_TEST(10)
SA_STRIDE_TEST(13)
SA_STRIDE_TEST(64)
SA_STRIDE_TEST(100)
SA_STRIDE_TEST(333)
SA_STRIDE_TEST(1000)
