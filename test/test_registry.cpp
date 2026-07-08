#include <criterion/criterion.h>
#include "Registry.hpp"
#include "chronoDebug.hpp"

struct Position { float x, y; };
struct Velocity { float dx, dy; };

Test(Registry_Hardcore, raw_pointer_cast_and_insert) {
    rtk::ecs::Registry registry;
    
    registry.register_component<Position>();
    auto& pos_array = registry.get_components<Position>();

    pos_array.insert_at(5, {10.5f, 20.5f});

    cr_assert_eq(pos_array.size(), 1, "The dense array should contain exactly 1 element.");
    cr_assert(pos_array.get(5).has_value(), "Entity 5 should possess a Position component.");
    cr_assert_float_eq(pos_array.get(5).value().x, 10.5f, 0.0001, "Position X should perfectly match.");
}

Test(Registry_Hardcore, global_entity_destruction) {
    rtk::ecs::Registry registry;
    
    registry.register_component<Position>();
    registry.register_component<Velocity>();

    auto& pos_array = registry.get_components<Position>();
    auto& vel_array = registry.get_components<Velocity>();

    pos_array.insert_at(1, {10.0f, 10.0f});
    vel_array.insert_at(1, {1.0f, 1.0f});

    pos_array.insert_at(2, {20.0f, 20.0f});

    pos_array.insert_at(3, {30.0f, 30.0f});
    vel_array.insert_at(3, {3.0f, 3.0f});

    registry.remove_entity_from_all_pools(1);

    cr_assert_eq(pos_array.size(), 2, "Position array size should drop from 3 to 2.");
    cr_assert_not(pos_array.get(1).has_value(), "Entity 1's Position should be completely erased.");
    cr_assert_float_eq(pos_array.get(3).value().x, 30.0f, 0.0001, "Entity 3's Position data should remain intact after the swap.");

    cr_assert_eq(vel_array.size(), 1, "Velocity array size should drop from 2 to 1.");
    cr_assert_not(vel_array.get(1).has_value(), "Entity 1's Velocity should be completely erased.");
    cr_assert_float_eq(vel_array.get(3).value().dx, 3.0f, 0.0001, "Entity 3's Velocity data should remain intact after the swap.");
}

Test(Registry_Hardcore, benchmark_get_components) {
    rtk::ecs::Registry registry;
    registry.register_component<Position>();

    {
        PROFILE_SCOPE("Benchmark_Get_Components_1_Million_Times");
        
        for (std::size_t i = 0; i < 1000000; ++i) {
            auto& pos_array = registry.get_components<Position>();
            (void)pos_array;
        }
    }

    cr_assert(true, "Benchmark completed without crashing.");
}