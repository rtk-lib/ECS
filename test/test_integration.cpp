#include <criterion/criterion.h>
#include <cstddef>
#include <set>
#include <string>
#include <vector>
#include "Ecs.hpp"

using namespace rtk::ecs;

namespace {
    struct IPos {
        float x;
        float y;
    };

    struct IVel {
        float dx;
        float dy;
    };

    struct IHp {
        int hp;
    };

    struct IDamage {
        int amount;
    };

    struct ILifetime {
        int frames;
    };

    struct IPlayer {};
    struct IEnemy {};
    struct IBullet {};

    struct World {
        Registry reg;
        Scheduler sched;

        World()
        {
            reg.register_component<IPos>();
            reg.register_component<IVel>();
            reg.register_component<IHp>();
            reg.register_component<IDamage>();
            reg.register_component<ILifetime>();
            reg.register_component<IPlayer>();
            reg.register_component<IEnemy>();
            reg.register_component<IBullet>();
        }

        std::size_t player(float x, float y)
        {
            std::size_t e = reg.spawn_entity();
            reg.get_components<IPos>().insert_at(e, IPos{x, y});
            reg.get_components<IHp>().insert_at(e, IHp{100});
            reg.get_components<IPlayer>().insert_at(e, IPlayer{});
            return e;
        }

        std::size_t enemy(float x, float y, int hp)
        {
            std::size_t e = reg.spawn_entity();
            reg.get_components<IPos>().insert_at(e, IPos{x, y});
            reg.get_components<IVel>().insert_at(e, IVel{-1.f, 0.f});
            reg.get_components<IHp>().insert_at(e, IHp{hp});
            reg.get_components<IEnemy>().insert_at(e, IEnemy{});
            return e;
        }

        std::size_t bullet(float x, float y, int frames)
        {
            std::size_t e = reg.spawn_entity();
            reg.get_components<IPos>().insert_at(e, IPos{x, y});
            reg.get_components<IVel>().insert_at(e, IVel{5.f, 0.f});
            reg.get_components<IDamage>().insert_at(e, IDamage{10});
            reg.get_components<ILifetime>().insert_at(e, ILifetime{frames});
            reg.get_components<IBullet>().insert_at(e, IBullet{});
            return e;
        }

        template <typename C>
        std::size_t count()
        {
            std::size_t n = 0;
            for (auto e : reg.view<C>()) {
                (void)e;
                ++n;
            }
            return n;
        }
    };

    class MoveSystem : public ISystem {
        public:
            void update(Registry &reg, float dt) override
            {
                auto &pos = reg.get_components<IPos>();
                auto &vel = reg.get_components<IVel>();
                for (auto e : reg.view<IVel, IPos>()) {
                    pos.get(e)->x += vel.get(e)->dx * dt;
                    pos.get(e)->y += vel.get(e)->dy * dt;
                }
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class LifetimeSystem : public ISystem {
        public:
            void update(Registry &reg, float) override
            {
                auto &life = reg.get_components<ILifetime>();
                for (auto e : reg.view<ILifetime>())
                    if (--life.get(e)->frames <= 0)
                        reg.kill_entity(e);
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class HealthSystem : public ISystem {
        public:
            int deaths = 0;
            void update(Registry &reg, float) override
            {
                auto &hp = reg.get_components<IHp>();
                for (auto e : reg.view<IHp>())
                    if (hp.get(e)->hp <= 0) {
                        reg.kill_entity(e);
                        ++deaths;
                    }
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class PoisonSystem : public ISystem {
        public:
            void update(Registry &reg, float) override
            {
                auto &hp = reg.get_components<IHp>();
                for (auto e : reg.view<IEnemy, IHp>())
                    hp.get(e)->hp -= 1;
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class BulletHitSystem : public ISystem {
        public:
            void update(Registry &reg, float) override
            {
                auto &pos = reg.get_components<IPos>();
                auto &hp = reg.get_components<IHp>();
                auto &dmg = reg.get_components<IDamage>();
                for (auto b : reg.view<IBullet, IPos, IDamage>()) {
                    for (auto t : reg.view<IEnemy, IPos, IHp>()) {
                        float d = pos.get(b)->x - pos.get(t)->x;
                        if (d * d < 1.f) {
                            hp.get(t)->hp -= dmg.get(b)->amount;
                            reg.kill_entity(b);
                            break;
                        }
                    }
                }
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class SpawnOnStart : public ISystem {
        public:
            explicit SpawnOnStart(int n) : _n(n) {}
            std::vector<std::size_t> ids;
            void update(Registry &, float) override {}
            void onStart(Registry &reg) override
            {
                for (int i = 0; i < _n; ++i) {
                    std::size_t e = reg.spawn_entity();
                    reg.get_components<IEnemy>().insert_at(e, IEnemy{});
                    ids.push_back(e);
                }
            }
            void onStop(Registry &reg) override
            {
                for (auto e : ids)
                    reg.kill_entity(e);
                reg.flush();
            }
        private:
            int _n;
    };
}

Test(integration, ecs_header_exposes_all_types)
{
    Registry reg;
    Scheduler sched;
    SparseArray<IPos> sa;
    cr_assert_eq(sa.size(), 0u);
    cr_assert_eq(ComponentType::get_id<IPos>(), ComponentType::get_id<IPos>());
    sched.run(reg, 0.f);
}

Test(integration, world_setup_registers_all_pools)
{
    World w;
    std::size_t p = w.player(0.f, 0.f);
    cr_assert(w.reg.has_component<IPlayer>(p));
    cr_assert(w.reg.has_component<IHp>(p));
    cr_assert_not(w.reg.has_component<IVel>(p));
}

Test(integration, tags_partition_entities)
{
    World w;
    w.player(0.f, 0.f);
    for (int i = 0; i < 5; ++i)
        w.enemy(10.f, 0.f, 10);
    for (int i = 0; i < 3; ++i)
        w.bullet(0.f, 0.f, 5);
    cr_assert_eq(w.count<IPlayer>(), 1u);
    cr_assert_eq(w.count<IEnemy>(), 5u);
    cr_assert_eq(w.count<IBullet>(), 3u);
    cr_assert_eq(w.count<IPos>(), 9u);
}

Test(integration, move_system_moves_only_entities_with_velocity)
{
    World w;
    std::size_t p = w.player(0.f, 0.f);
    std::size_t e = w.enemy(10.f, 0.f, 10);
    w.sched.add<MoveSystem>(Order::Physics);
    w.sched.run(w.reg, 1.f);
    cr_assert_float_eq(w.reg.get_components<IPos>().get(p)->x, 0.f, 1e-6);
    cr_assert_float_eq(w.reg.get_components<IPos>().get(e)->x, 9.f, 1e-6);
}

Test(integration, bullets_expire_after_lifetime)
{
    World w;
    std::size_t b = w.bullet(0.f, 0.f, 3);
    w.sched.add<LifetimeSystem>(Order::Update);
    w.sched.run(w.reg, 0.f);
    w.sched.run(w.reg, 0.f);
    cr_assert(w.reg.has_component<IBullet>(b));
    w.sched.run(w.reg, 0.f);
    cr_assert_not(w.reg.has_component<IBullet>(b));
    cr_assert_eq(w.count<IPos>(), 0u);
}

Test(integration, bullets_with_different_lifetimes)
{
    World w;
    for (int i = 1; i <= 5; ++i)
        w.bullet(0.f, 0.f, i);
    w.sched.add<LifetimeSystem>(Order::Update);
    for (int f = 1; f <= 5; ++f) {
        w.sched.run(w.reg, 0.f);
        cr_assert_eq(w.count<IBullet>(), static_cast<std::size_t>(5 - f));
    }
}

Test(integration, poison_kills_enemies_but_not_player)
{
    World w;
    std::size_t p = w.player(0.f, 0.f);
    w.enemy(5.f, 0.f, 2);
    w.enemy(6.f, 0.f, 2);
    w.sched.add<PoisonSystem>(Order::Update);
    auto &health = w.sched.add<HealthSystem>(Order::SecondUpdate);
    w.sched.run(w.reg, 0.f);
    w.sched.run(w.reg, 0.f);
    cr_assert_eq(health.deaths, 2);
    cr_assert_eq(w.count<IEnemy>(), 0u);
    cr_assert(w.reg.has_component<IPlayer>(p));
    cr_assert_eq(w.reg.get_components<IHp>().get(p)->hp, 100);
}

Test(integration, dead_enemy_ids_are_recycled_for_bullets)
{
    World w;
    std::size_t e = w.enemy(0.f, 0.f, 0);
    w.sched.add<HealthSystem>(Order::Update);
    w.sched.run(w.reg, 0.f);
    std::size_t b = w.bullet(0.f, 0.f, 10);
    cr_assert_eq(b, e);
    cr_assert(w.reg.has_component<IBullet>(b));
    cr_assert_not(w.reg.has_component<IEnemy>(b));
    cr_assert_not(w.reg.has_component<IHp>(b));
}

Test(integration, bullet_hits_enemy_and_disappears)
{
    World w;
    std::size_t e = w.enemy(5.f, 0.f, 15);
    std::size_t b = w.bullet(5.f, 0.f, 100);
    w.sched.add<BulletHitSystem>(Order::Physics);
    w.sched.run(w.reg, 0.f);
    cr_assert_not(w.reg.has_component<IBullet>(b));
    cr_assert_eq(w.reg.get_components<IHp>().get(e)->hp, 5);
}

Test(integration, bullet_misses_far_enemy)
{
    World w;
    std::size_t e = w.enemy(50.f, 0.f, 15);
    std::size_t b = w.bullet(0.f, 0.f, 100);
    w.sched.add<BulletHitSystem>(Order::Physics);
    w.sched.run(w.reg, 0.f);
    cr_assert(w.reg.has_component<IBullet>(b));
    cr_assert_eq(w.reg.get_components<IHp>().get(e)->hp, 15);
}

Test(integration, bullet_travels_then_hits)
{
    World w;
    std::size_t e = w.enemy(10.f, 0.f, 10);
    w.reg.get_components<IVel>().erase(e);
    std::size_t b = w.bullet(0.f, 0.f, 100);
    w.sched.add<MoveSystem>(Order::Physics);
    w.sched.add<BulletHitSystem>(Order::SecondUpdate);
    auto &health = w.sched.add<HealthSystem>(Order::Render);
    w.sched.run(w.reg, 1.f);
    cr_assert(w.reg.has_component<IBullet>(b));
    cr_assert_eq(health.deaths, 0);
    w.sched.run(w.reg, 1.f);
    cr_assert_not(w.reg.has_component<IBullet>(b));
    cr_assert_eq(health.deaths, 1);
    cr_assert_not(w.reg.has_component<IEnemy>(e));
}

Test(integration, on_start_and_on_stop_manage_entities)
{
    World w;
    auto &sp = w.sched.add<SpawnOnStart>(Order::Input, 4);
    w.sched.start(w.reg);
    cr_assert_eq(w.count<IEnemy>(), 4u);
    w.sched.stop(w.reg);
    cr_assert_eq(w.count<IEnemy>(), 0u);
    std::set<std::size_t> before(sp.ids.begin(), sp.ids.end());
    std::set<std::size_t> after;
    for (int i = 0; i < 4; ++i)
        after.insert(w.reg.spawn_entity());
    cr_assert(before == after);
}

Test(integration, long_simulation_stays_consistent)
{
    World w;
    w.sched.add<MoveSystem>(Order::Physics);
    w.sched.add<LifetimeSystem>(Order::Update);
    for (int frame = 0; frame < 200; ++frame) {
        w.bullet(0.f, 0.f, 10);
        w.sched.run(w.reg, 0.016f);
        cr_assert_leq(w.count<IBullet>(), 10u);
    }
    cr_assert_eq(w.count<IBullet>(), 9u);
    for (std::size_t e = 0; e < 20; ++e)
        cr_assert_eq(w.reg.has_component<IBullet>(e), w.reg.has_component<ILifetime>(e));
}

Test(integration, entity_ids_bounded_in_long_simulation)
{
    World w;
    w.sched.add<LifetimeSystem>(Order::Update);
    std::size_t maxId = 0;
    for (int frame = 0; frame < 500; ++frame) {
        std::size_t b = w.bullet(0.f, 0.f, 5);
        if (b > maxId)
            maxId = b;
        w.sched.run(w.reg, 0.f);
    }
    cr_assert_lt(maxId, 10u);
}

Test(integration, pools_stay_coherent_after_simulation)
{
    World w;
    w.sched.add<LifetimeSystem>(Order::Update);
    w.sched.add<MoveSystem>(Order::Physics);
    for (int frame = 0; frame < 50; ++frame) {
        w.bullet(0.f, 0.f, (frame % 7) + 1);
        if (frame % 3 == 0)
            w.enemy(0.f, 0.f, 10);
        w.sched.run(w.reg, 0.1f);
    }
    auto check = [](auto &pool) {
        for (std::size_t d = 0; d < pool._dense.size(); ++d)
            cr_assert_eq(pool._sparse[pool._packed_entities[d]], d);
    };
    check(w.reg.get_components<IPos>());
    check(w.reg.get_components<IVel>());
    check(w.reg.get_components<IHp>());
    check(w.reg.get_components<ILifetime>());
    check(w.reg.get_components<IBullet>());
}

Test(integration, every_bullet_keeps_all_its_components)
{
    World w;
    w.sched.add<LifetimeSystem>(Order::Update);
    for (int frame = 0; frame < 30; ++frame) {
        w.bullet(0.f, 0.f, (frame % 4) + 1);
        w.sched.run(w.reg, 0.f);
    }
    for (auto b : w.reg.view<IBullet>()) {
        cr_assert(w.reg.has_component<IPos>(b));
        cr_assert(w.reg.has_component<IVel>(b));
        cr_assert(w.reg.has_component<IDamage>(b));
        cr_assert(w.reg.has_component<ILifetime>(b));
    }
}

Test(integration, view_driver_order_does_not_change_simulation)
{
    World a;
    World b;
    for (int i = 0; i < 20; ++i) {
        a.enemy(static_cast<float>(i), 0.f, 10);
        b.enemy(static_cast<float>(i), 0.f, 10);
    }
    a.sched.add<MoveSystem>(Order::Physics);
    for (int f = 0; f < 10; ++f)
        a.sched.run(a.reg, 0.5f);
    auto &bpos = b.reg.get_components<IPos>();
    auto &bvel = b.reg.get_components<IVel>();
    for (int f = 0; f < 10; ++f)
        for (auto e : b.reg.view<IPos, IVel>())
            bpos.get(e)->x += bvel.get(e)->dx * 0.5f;
    for (std::size_t e = 0; e < 20; ++e)
        cr_assert_float_eq(a.reg.get_components<IPos>().get(e)->x, bpos.get(e)->x, 1e-5);
}

Test(integration, kill_everything_then_reuse_world)
{
    World w;
    for (int i = 0; i < 10; ++i)
        w.enemy(0.f, 0.f, 1);
    for (std::size_t e = 0; e < 10; ++e)
        w.reg.kill_entity(e);
    w.reg.flush();
    cr_assert_eq(w.count<IPos>(), 0u);
    cr_assert_eq(w.count<IHp>(), 0u);
    std::size_t p = w.player(1.f, 1.f);
    cr_assert_lt(p, 10u);
    cr_assert_eq(w.count<IPlayer>(), 1u);
}

Test(integration, multiple_worlds_are_isolated)
{
    World a;
    World b;
    a.enemy(0.f, 0.f, 0);
    a.sched.add<HealthSystem>(Order::Update);
    std::size_t be = b.enemy(0.f, 0.f, 0);
    a.sched.run(a.reg, 0.f);
    cr_assert_eq(a.count<IEnemy>(), 0u);
    cr_assert(b.reg.has_component<IEnemy>(be));
}

Test(integration, player_and_enemy_view)
{
    World w;
    w.player(0.f, 0.f);
    w.enemy(0.f, 0.f, 1);
    std::size_t both = w.reg.spawn_entity();
    w.reg.get_components<IPlayer>().insert_at(both, IPlayer{});
    w.reg.get_components<IEnemy>().insert_at(both, IEnemy{});
    std::vector<std::size_t> out;
    for (auto e : w.reg.view<IPlayer, IEnemy>())
        out.push_back(e);
    cr_assert(out == (std::vector<std::size_t>{both}));
}

#define WORLD_WAVE_TEST(N) \
    Test(integration_param, wave_of_##N##_enemies_dies) \
    { \
        World w; \
        for (int i = 0; i < (N); ++i) \
            w.enemy(static_cast<float>(i), 0.f, (i % 3) + 1); \
        w.sched.add<PoisonSystem>(Order::Update); \
        auto &health = w.sched.add<HealthSystem>(Order::SecondUpdate); \
        for (int f = 0; f < 3; ++f) \
            w.sched.run(w.reg, 0.f); \
        cr_assert_eq(health.deaths, (N)); \
        cr_assert_eq(w.count<IEnemy>(), 0u); \
        cr_assert_eq(w.count<IPos>(), 0u); \
    }

WORLD_WAVE_TEST(1)
WORLD_WAVE_TEST(3)
WORLD_WAVE_TEST(5)
WORLD_WAVE_TEST(10)
WORLD_WAVE_TEST(33)
WORLD_WAVE_TEST(50)
WORLD_WAVE_TEST(100)
WORLD_WAVE_TEST(250)
WORLD_WAVE_TEST(500)
WORLD_WAVE_TEST(1000)

#define WORLD_LIFETIME_TEST(L) \
    Test(integration_param, steady_state_with_lifetime_##L) \
    { \
        World w; \
        w.sched.add<LifetimeSystem>(Order::Update); \
        std::size_t maxId = 0; \
        for (int f = 0; f < 100; ++f) { \
            std::size_t b = w.bullet(0.f, 0.f, (L)); \
            maxId = b > maxId ? b : maxId; \
            w.sched.run(w.reg, 0.f); \
        } \
        cr_assert_eq(w.count<IBullet>(), static_cast<std::size_t>((L) - 1)); \
        cr_assert_lt(maxId, static_cast<std::size_t>(L)); \
    }

WORLD_LIFETIME_TEST(1)
WORLD_LIFETIME_TEST(2)
WORLD_LIFETIME_TEST(3)
WORLD_LIFETIME_TEST(5)
WORLD_LIFETIME_TEST(7)
WORLD_LIFETIME_TEST(10)
WORLD_LIFETIME_TEST(15)
WORLD_LIFETIME_TEST(25)
WORLD_LIFETIME_TEST(50)
WORLD_LIFETIME_TEST(99)
