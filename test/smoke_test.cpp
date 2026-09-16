#include "Registry.hpp"

#include <cstddef>

struct Position {
    float x;
    float y;
};

struct Velocity {
    float x;
    float y;
};

int main()
{
    rtk::ecs::Registry registry;

    auto& positions = registry.register_component<Position>();
    auto& velocities = registry.register_component<Velocity>();

    const std::size_t entity = registry.spawn_entity();

    positions.insert_at(entity, Position{10.f, 20.f});
    velocities.insert_at(entity, Velocity{1.f, 2.f});

    std::size_t count = 0;

    for (const auto current :
         registry.view<Position, Velocity>()) {
        if (current == entity) {
            ++count;
        }
    }

    return count == 1 ? 0 : 1;
}