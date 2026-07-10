#include <criterion/criterion.h>
#include "Registry.hpp"
#include "chronoDebug.hpp"

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health { int hp; };

Test(System_Performance, iterate_single_component_1M) {
    rtk::ecs::Registry registry;
    registry.register_component<Position>();
    auto& pos_array = registry.get_components<Position>();

    const std::size_t num_entities = 1000000;
    for (std::size_t i = 0; i < num_entities; ++i) {
        std::size_t entity = registry.spawn_entity();
        pos_array.insert_at(entity, {1.0f, 1.0f});
    }

    {
        PROFILE_SCOPE("System_Single_Component_1M");
        for (auto entity : registry.view<Position>()) {
            auto pos = pos_array.get(entity);
            (void)pos; 
        }
    }
    cr_assert(true, "Benchmark 1 composant terminé.");
}

Test(System_Performance, iterate_two_components_dense) {
    rtk::ecs::Registry registry;
    registry.register_component<Position>();
    registry.register_component<Velocity>();

    auto& pos_array = registry.get_components<Position>();
    auto& vel_array = registry.get_components<Velocity>();

    const std::size_t num_entities = 1000000;
    for (std::size_t i = 0; i < num_entities; ++i) {
        std::size_t entity = registry.spawn_entity();
        pos_array.insert_at(entity, {0.0f, 0.0f});
        vel_array.insert_at(entity, {1.5f, 1.5f});
    }

    {
        PROFILE_SCOPE("System_Two_Components_100_Percent_Match_1M");
        for (auto entity : registry.view<Position, Velocity>()) {
            auto pos = pos_array.get(entity);
            auto vel = vel_array.get(entity);
            (void)pos;
            (void)vel;
        }
    }
    cr_assert(true, "Benchmark 2 composants denses terminé.");
}

Test(System_Performance, iterate_two_components_sparse) {
    rtk::ecs::Registry registry;
    registry.register_component<Position>();
    registry.register_component<Velocity>();

    auto& pos_array = registry.get_components<Position>();
    auto& vel_array = registry.get_components<Velocity>();

    const std::size_t num_entities = 1000000;
    for (std::size_t i = 0; i < num_entities; ++i) {
        std::size_t entity = registry.spawn_entity();
        pos_array.insert_at(entity, {0.0f, 0.0f});
        if (i % 10 == 0) {
            vel_array.insert_at(entity, {1.0f, 1.0f});
        }
    }

    {
        PROFILE_SCOPE("System_Sparse_Unoptimized_Driver");
        for (auto entity : registry.view<Position, Velocity>()) {
            (void)entity;
        }
    }
    
    {
        PROFILE_SCOPE("System_Sparse_Optimized_Driver");
        for (auto entity : registry.view<Velocity, Position>()) {
            (void)entity;
        }
    }
    cr_assert(true, "Benchmark sparse (filtrage) terminé.");
}