#include <criterion/criterion.h>
#include "Ecs.hpp"

struct Position { float x, y; };
struct Vitesse  { float dx, dy; };
struct Sante    { int pv; };

Test(Registry, register_and_insert_components) {
    rtk::ecs::Registry registry;

    registry.register_component<Position>();
    registry.register_component<Vitesse>();
    registry.register_component<Sante>();

    auto& tableau_pos = registry.get_components<Position>();

    tableau_pos.insert_at(5, {100.0f, 200.0f});

    cr_assert_eq(tableau_pos.size(), 1);

    auto pos_entite_5 = tableau_pos.get(5);
    cr_assert(pos_entite_5.has_value());


    cr_assert_float_eq(pos_entite_5.value().x, 100.0f, 0.0001);
    cr_assert_float_eq(pos_entite_5.value().y, 200.0f, 0.0001);
    
    auto pos_entite_4 = tableau_pos.get(4);
    cr_assert_not(pos_entite_4.has_value());
}