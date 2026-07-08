#include <criterion/criterion.h>
#include <signal.h>
#include "Registry.hpp"

struct Position { float x, y; };
struct Velocity { float dx, dy; };

Test(Registry_Safety, assert_on_unregistered_get, .signal = SIGABRT) {
    rtk::ecs::Registry registry;
    auto& pos_array = registry.get_components<Position>();
    (void)pos_array;
}

Test(Registry_Safety, assert_on_double_registration, .signal = SIGABRT) {
    rtk::ecs::Registry registry;

    registry.register_component<Velocity>();

    registry.register_component<Velocity>();
}

Test(Registry_Entities, ID_generation_and_recycling) {
    rtk::ecs::Registry registry;

    std::size_t e0 = registry.spawn_entity();
    std::size_t e1 = registry.spawn_entity();
    std::size_t e2 = registry.spawn_entity();

    cr_assert_eq(e0, 0, "First entity should be ID 0");
    cr_assert_eq(e1, 1, "Second entity should be ID 1");
    cr_assert_eq(e2, 2, "Third entity should be ID 2");

    registry.kill_entity(e1);

    std::size_t e3 = registry.spawn_entity();
    
    cr_assert_eq(e3, 1, "The engine failed to recycle the dead ID! It should have reused ID 1.");

    std::size_t e4 = registry.spawn_entity();
    
    cr_assert_eq(e4, 3, "After the recycling pool is empty, the engine should resume normal counting at 3.");
}