#include <criterion/criterion.h>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include "Registry.hpp"
#include "ISystem.hpp"
#include "Scheduler.hpp"

using namespace rtk::ecs;

namespace {
    struct SPos {
        float x;
        float y;
    };

    struct SVel {
        float dx;
        float dy;
    };

    struct SHp {
        int hp;
    };

    using Log = std::vector<std::string>;

    class Recorder : public ISystem {
        public:
            Recorder(Log &log, std::string name) : _log(log), _name(std::move(name)) {}

            void update(Registry &, float) override { _log.push_back(_name + ":update"); }
            void onStart(Registry &) override { _log.push_back(_name + ":start"); }
            void onStop(Registry &) override { _log.push_back(_name + ":stop"); }

        private:
            Log &_log;
            std::string _name;
    };

    class Counter : public ISystem {
        public:
            int updates = 0;
            int starts = 0;
            int stops = 0;
            float lastDt = -1.f;
            float totalDt = 0.f;
            Registry *lastReg = nullptr;

            void update(Registry &reg, float dt) override
            {
                ++updates;
                lastDt = dt;
                totalDt += dt;
                lastReg = &reg;
            }
            void onStart(Registry &reg) override { ++starts; lastReg = &reg; }
            void onStop(Registry &reg) override { ++stops; lastReg = &reg; }
    };

    class Killer : public ISystem {
        public:
            explicit Killer(std::size_t target) : _target(target) {}
            void update(Registry &reg, float) override { reg.kill_entity(_target); }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
        private:
            std::size_t _target;
    };

    class Observer : public ISystem {
        public:
            explicit Observer(std::size_t target) : _target(target) {}
            bool sawComponent = false;
            void update(Registry &reg, float) override { sawComponent = reg.has_component<SHp>(_target); }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
        private:
            std::size_t _target;
    };

    class Spawner : public ISystem {
        public:
            std::vector<std::size_t> spawned;
            void update(Registry &reg, float) override { spawned.push_back(reg.spawn_entity()); }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class Movement : public ISystem {
        public:
            void update(Registry &reg, float dt) override
            {
                auto &pos = reg.get_components<SPos>();
                auto &vel = reg.get_components<SVel>();
                for (auto e : reg.view<SPos, SVel>()) {
                    pos.get(e)->x += vel.get(e)->dx * dt;
                    pos.get(e)->y += vel.get(e)->dy * dt;
                }
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class Damage : public ISystem {
        public:
            explicit Damage(int amount) : _amount(amount) {}
            void update(Registry &reg, float) override
            {
                auto &hp = reg.get_components<SHp>();
                for (auto e : reg.view<SHp>())
                    hp.get(e)->hp -= _amount;
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
        private:
            int _amount;
    };

    class Reaper : public ISystem {
        public:
            void update(Registry &reg, float) override
            {
                auto &hp = reg.get_components<SHp>();
                for (auto e : reg.view<SHp>())
                    if (hp.get(e)->hp <= 0)
                        reg.kill_entity(e);
            }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class SetupSystem : public ISystem {
        public:
            std::size_t created = NULL_ENTITY;
            std::size_t destroyed = NULL_ENTITY;
            void update(Registry &, float) override {}
            void onStart(Registry &reg) override
            {
                reg.register_component<SHp>();
                created = reg.spawn_entity();
                reg.get_components<SHp>().insert_at(created, SHp{5});
            }
            void onStop(Registry &reg) override
            {
                destroyed = created;
                reg.kill_entity(created);
            }
    };

    class RefWriter : public ISystem {
        public:
            explicit RefWriter(int &target) : _target(target) {}
            void update(Registry &, float) override { ++_target; }
            void onStart(Registry &) override { _target = 100; }
            void onStop(Registry &) override { _target = -1; }
        private:
            int &_target;
    };

    class OwnsPtr : public ISystem {
        public:
            explicit OwnsPtr(std::unique_ptr<int> p) : ptr(std::move(p)) {}
            std::unique_ptr<int> ptr;
            void update(Registry &, float) override { ++*ptr; }
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class MultiArgs : public ISystem {
        public:
            MultiArgs(int a, float b, std::string c) : a(a), b(b), c(std::move(c)) {}
            int a;
            float b;
            std::string c;
            void update(Registry &, float) override {}
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
    };

    class DtorTracker : public ISystem {
        public:
            explicit DtorTracker(int &counter) : _counter(counter) {}
            ~DtorTracker() override { ++_counter; }
            void update(Registry &, float) override {}
            void onStart(Registry &) override {}
            void onStop(Registry &) override {}
        private:
            int &_counter;
    };

    std::size_t idx(Order o) { return static_cast<std::size_t>(o); }
}

Test(isystem, is_abstract)
{
    cr_assert(std::is_abstract_v<ISystem>);
}

Test(isystem, is_polymorphic)
{
    cr_assert(std::is_polymorphic_v<ISystem>);
}

Test(isystem, has_virtual_destructor)
{
    cr_assert(std::has_virtual_destructor_v<ISystem>);
}

Test(isystem, is_not_copy_constructible)
{
    cr_assert_not(std::is_copy_constructible_v<ISystem>);
}

Test(isystem, is_not_copy_assignable)
{
    cr_assert_not(std::is_copy_assignable_v<ISystem>);
}

Test(isystem, derived_is_not_copyable)
{
    cr_assert_not(std::is_copy_constructible_v<Counter>);
}

Test(isystem, derived_is_default_constructible)
{
    cr_assert(std::is_default_constructible_v<Counter>);
}

Test(isystem, derived_is_convertible_to_base_pointer)
{
    cr_assert((std::is_convertible_v<Counter *, ISystem *>));
}

Test(isystem, delete_through_base_calls_derived_destructor)
{
    int destroyed = 0;
    {
        std::unique_ptr<ISystem> s = std::make_unique<DtorTracker>(destroyed);
    }
    cr_assert_eq(destroyed, 1);
}

Test(isystem, virtual_dispatch_update)
{
    Registry reg;
    Counter c;
    ISystem &base = c;
    base.update(reg, 0.5f);
    cr_assert_eq(c.updates, 1);
    cr_assert_float_eq(c.lastDt, 0.5f, 1e-6);
}

Test(isystem, virtual_dispatch_start_stop)
{
    Registry reg;
    Counter c;
    ISystem &base = c;
    base.onStart(reg);
    base.onStop(reg);
    cr_assert_eq(c.starts, 1);
    cr_assert_eq(c.stops, 1);
}

Test(scheduler_order, enum_values_are_contiguous)
{
    cr_assert_eq(idx(Order::Input), 0u);
    cr_assert_eq(idx(Order::Update), 1u);
    cr_assert_eq(idx(Order::Physics), 2u);
    cr_assert_eq(idx(Order::SecondUpdate), 3u);
    cr_assert_eq(idx(Order::Render), 4u);
}

Test(scheduler_order, size_is_number_of_stages)
{
    cr_assert_eq(idx(Order::Size), 5u);
}

Test(scheduler, add_returns_reference_to_created_system)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    s.run(reg, 1.f);
    cr_assert_eq(c.updates, 1);
}

Test(scheduler, add_returns_distinct_instances)
{
    Scheduler s;
    Counter &a = s.add<Counter>(Order::Update);
    Counter &b = s.add<Counter>(Order::Update);
    cr_assert_neq(&a, &b);
}

Test(scheduler, returned_reference_stays_valid_after_more_adds)
{
    Scheduler s;
    Registry reg;
    Counter &first = s.add<Counter>(Order::Update);
    for (int i = 0; i < 100; ++i)
        s.add<Counter>(Order::Update);
    s.run(reg, 1.f);
    cr_assert_eq(first.updates, 1);
}

Test(scheduler, add_forwards_lvalue_reference)
{
    Scheduler s;
    Registry reg;
    int value = 0;
    s.add<RefWriter>(Order::Update, value);
    s.run(reg, 0.f);
    s.run(reg, 0.f);
    cr_assert_eq(value, 2);
}

Test(scheduler, add_forwards_rvalue_move_only)
{
    Scheduler s;
    Registry reg;
    auto &sys = s.add<OwnsPtr>(Order::Update, std::make_unique<int>(10));
    s.run(reg, 0.f);
    cr_assert_eq(*sys.ptr, 11);
}

Test(scheduler, add_forwards_multiple_arguments)
{
    Scheduler s;
    auto &m = s.add<MultiArgs>(Order::Input, 4, 2.5f, std::string("abc"));
    cr_assert_eq(m.a, 4);
    cr_assert_float_eq(m.b, 2.5f, 1e-6);
    cr_assert_str_eq(m.c.c_str(), "abc");
}

Test(scheduler, add_converts_string_literal_argument)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Update, log, "lit");
    s.run(reg, 0.f);
    cr_assert_str_eq(log.at(0).c_str(), "lit:update");
}

Test(scheduler, add_does_not_call_any_hook)
{
    Scheduler s;
    Counter &c = s.add<Counter>(Order::Update);
    cr_assert_eq(c.updates, 0);
    cr_assert_eq(c.starts, 0);
    cr_assert_eq(c.stops, 0);
}

Test(scheduler, returned_reference_can_be_configured_before_run)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    c.updates = 41;
    s.run(reg, 0.f);
    cr_assert_eq(c.updates, 42);
}

Test(scheduler, run_on_empty_scheduler)
{
    Scheduler s;
    Registry reg;
    s.run(reg, 0.016f);
    cr_assert_eq(reg.spawn_entity(), 0u);
}

Test(scheduler, run_calls_update_once_per_system)
{
    Scheduler s;
    Registry reg;
    Counter &a = s.add<Counter>(Order::Input);
    Counter &b = s.add<Counter>(Order::Render);
    s.run(reg, 0.f);
    cr_assert_eq(a.updates, 1);
    cr_assert_eq(b.updates, 1);
}

Test(scheduler, run_passes_delta_time)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Physics);
    s.run(reg, 0.125f);
    cr_assert_float_eq(c.lastDt, 0.125f, 1e-7);
}

Test(scheduler, run_passes_zero_delta_time)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Physics);
    s.run(reg, 0.f);
    cr_assert_float_eq(c.lastDt, 0.f, 1e-7);
}

Test(scheduler, run_passes_negative_delta_time_untouched)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Physics);
    s.run(reg, -1.5f);
    cr_assert_float_eq(c.lastDt, -1.5f, 1e-7);
}

Test(scheduler, run_passes_same_registry)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    s.run(reg, 0.f);
    cr_assert_eq(c.lastReg, &reg);
}

Test(scheduler, run_with_different_registries)
{
    Scheduler s;
    Registry a;
    Registry b;
    Counter &c = s.add<Counter>(Order::Update);
    s.run(a, 0.f);
    cr_assert_eq(c.lastReg, &a);
    s.run(b, 0.f);
    cr_assert_eq(c.lastReg, &b);
}

Test(scheduler, run_accumulates_delta_time)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    for (int i = 0; i < 4; ++i)
        s.run(reg, 0.25f);
    cr_assert_float_eq(c.totalDt, 1.f, 1e-6);
}

Test(scheduler, run_does_not_call_start_or_stop)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    s.run(reg, 0.f);
    cr_assert_eq(c.starts, 0);
    cr_assert_eq(c.stops, 0);
}

Test(scheduler, stages_run_in_enum_order_regardless_of_add_order)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Render, log, "render");
    s.add<Recorder>(Order::Physics, log, "physics");
    s.add<Recorder>(Order::Input, log, "input");
    s.add<Recorder>(Order::SecondUpdate, log, "second");
    s.add<Recorder>(Order::Update, log, "update");
    s.run(reg, 0.f);
    Log expected{"input:update", "update:update", "physics:update", "second:update", "render:update"};
    cr_assert(log == expected);
}

Test(scheduler, systems_in_same_stage_run_in_insertion_order)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Update, log, "a");
    s.add<Recorder>(Order::Update, log, "b");
    s.add<Recorder>(Order::Update, log, "c");
    s.run(reg, 0.f);
    Log expected{"a:update", "b:update", "c:update"};
    cr_assert(log == expected);
}

Test(scheduler, mixed_stages_and_insertion_order)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Render, log, "r1");
    s.add<Recorder>(Order::Input, log, "i1");
    s.add<Recorder>(Order::Render, log, "r2");
    s.add<Recorder>(Order::Input, log, "i2");
    s.run(reg, 0.f);
    Log expected{"i1:update", "i2:update", "r1:update", "r2:update"};
    cr_assert(log == expected);
}

Test(scheduler, repeated_runs_repeat_sequence)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Update, log, "u");
    s.add<Recorder>(Order::Input, log, "i");
    s.run(reg, 0.f);
    s.run(reg, 0.f);
    Log expected{"i:update", "u:update", "i:update", "u:update"};
    cr_assert(log == expected);
}

Test(scheduler, same_system_type_in_every_stage)
{
    Scheduler s;
    Registry reg;
    std::vector<Counter *> counters;
    for (std::size_t i = 0; i < idx(Order::Size); ++i)
        counters.push_back(&s.add<Counter>(static_cast<Order>(i)));
    s.run(reg, 0.f);
    for (auto *c : counters)
        cr_assert_eq(c->updates, 1);
}

Test(scheduler, system_state_persists_between_frames)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    for (int i = 0; i < 30; ++i)
        s.run(reg, 0.f);
    cr_assert_eq(c.updates, 30);
}

Test(scheduler, start_calls_on_start_for_every_system)
{
    Scheduler s;
    Registry reg;
    Counter &a = s.add<Counter>(Order::Input);
    Counter &b = s.add<Counter>(Order::Render);
    s.start(reg);
    cr_assert_eq(a.starts, 1);
    cr_assert_eq(b.starts, 1);
}

Test(scheduler, start_does_not_update)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Input);
    s.start(reg);
    cr_assert_eq(c.updates, 0);
    cr_assert_eq(c.stops, 0);
}

Test(scheduler, stop_calls_on_stop_for_every_system)
{
    Scheduler s;
    Registry reg;
    Counter &a = s.add<Counter>(Order::Physics);
    Counter &b = s.add<Counter>(Order::SecondUpdate);
    s.stop(reg);
    cr_assert_eq(a.stops, 1);
    cr_assert_eq(b.stops, 1);
}

Test(scheduler, stop_does_not_update)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Input);
    s.stop(reg);
    cr_assert_eq(c.updates, 0);
    cr_assert_eq(c.starts, 0);
}

Test(scheduler, start_follows_stage_order)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Render, log, "r");
    s.add<Recorder>(Order::Input, log, "i");
    s.add<Recorder>(Order::Physics, log, "p");
    s.start(reg);
    Log expected{"i:start", "p:start", "r:start"};
    cr_assert(log == expected);
}

Test(scheduler, stop_follows_stage_order)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Render, log, "r");
    s.add<Recorder>(Order::Input, log, "i");
    s.add<Recorder>(Order::Physics, log, "p");
    s.stop(reg);
    Log expected{"i:stop", "p:stop", "r:stop"};
    cr_assert(log == expected);
}

Test(scheduler, full_lifecycle_log)
{
    Scheduler s;
    Registry reg;
    Log log;
    s.add<Recorder>(Order::Update, log, "a");
    s.start(reg);
    s.run(reg, 0.f);
    s.stop(reg);
    Log expected{"a:start", "a:update", "a:stop"};
    cr_assert(log == expected);
}

Test(scheduler, start_and_stop_on_empty_scheduler)
{
    Scheduler s;
    Registry reg;
    s.start(reg);
    s.stop(reg);
    cr_assert_eq(reg.spawn_entity(), 0u);
}

Test(scheduler, restart_after_stop)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    s.start(reg);
    s.stop(reg);
    s.start(reg);
    cr_assert_eq(c.starts, 2);
    cr_assert_eq(c.stops, 1);
}

Test(scheduler, start_passes_registry)
{
    Scheduler s;
    Registry reg;
    Counter &c = s.add<Counter>(Order::Update);
    s.start(reg);
    cr_assert_eq(c.lastReg, &reg);
}

Test(scheduler, on_start_can_setup_registry)
{
    Scheduler s;
    Registry reg;
    auto &setup = s.add<SetupSystem>(Order::Input);
    s.start(reg);
    cr_assert(reg.has_component<SHp>(setup.created));
}

Test(scheduler, on_stop_kill_is_pending_until_flush)
{
    Scheduler s;
    Registry reg;
    auto &setup = s.add<SetupSystem>(Order::Input);
    s.start(reg);
    s.stop(reg);
    cr_assert(reg.has_component<SHp>(setup.destroyed));
    reg.flush();
    cr_assert_not(reg.has_component<SHp>(setup.destroyed));
}

Test(scheduler, ref_writer_lifecycle)
{
    Scheduler s;
    Registry reg;
    int value = 0;
    s.add<RefWriter>(Order::Input, value);
    s.start(reg);
    cr_assert_eq(value, 100);
    s.run(reg, 0.f);
    cr_assert_eq(value, 101);
    s.stop(reg);
    cr_assert_eq(value, -1);
}

Test(scheduler, run_flushes_kills_requested_before_run)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(e, SHp{1});
    reg.kill_entity(e);
    s.run(reg, 0.f);
    cr_assert_not(reg.has_component<SHp>(e));
}

Test(scheduler, kill_in_input_is_flushed_before_update)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(e, SHp{1});
    s.add<Killer>(Order::Input, e);
    auto &obs = s.add<Observer>(Order::Update, e);
    s.run(reg, 0.f);
    cr_assert_not(obs.sawComponent);
}

Test(scheduler, kill_in_same_stage_not_flushed_until_stage_end)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(e, SHp{1});
    s.add<Killer>(Order::Update, e);
    auto &obs = s.add<Observer>(Order::Update, e);
    s.run(reg, 0.f);
    cr_assert(obs.sawComponent);
    cr_assert_not(reg.has_component<SHp>(e));
}

Test(scheduler, kill_in_render_is_flushed_at_end_of_run)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(e, SHp{1});
    s.add<Killer>(Order::Render, e);
    s.run(reg, 0.f);
    cr_assert_not(reg.has_component<SHp>(e));
    cr_assert_eq(reg.spawn_entity(), e);
}

Test(scheduler, observer_before_killer_sees_component)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(e, SHp{1});
    auto &obs = s.add<Observer>(Order::Input, e);
    s.add<Killer>(Order::Physics, e);
    s.run(reg, 0.f);
    cr_assert(obs.sawComponent);
}

Test(scheduler, killing_already_dead_entity_each_frame_is_harmless)
{
    Scheduler s;
    Registry reg;
    std::size_t e = reg.spawn_entity();
    s.add<Killer>(Order::Update, e);
    for (int i = 0; i < 5; ++i)
        s.run(reg, 0.f);
    cr_assert_eq(reg.spawn_entity(), e);
    cr_assert_eq(reg.spawn_entity(), 1u);
}

Test(scheduler, spawner_creates_one_entity_per_frame)
{
    Scheduler s;
    Registry reg;
    auto &sp = s.add<Spawner>(Order::Update);
    for (int i = 0; i < 5; ++i)
        s.run(reg, 0.f);
    std::vector<std::size_t> expected{0, 1, 2, 3, 4};
    cr_assert(sp.spawned == expected);
}

Test(scheduler_game, movement_system_integrates_velocity)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SPos>();
    reg.register_component<SVel>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SPos>().insert_at(e, SPos{0.f, 0.f});
    reg.get_components<SVel>().insert_at(e, SVel{10.f, -2.f});
    s.add<Movement>(Order::Physics);
    for (int i = 0; i < 10; ++i)
        s.run(reg, 0.1f);
    cr_assert_float_eq(reg.get_components<SPos>().get(e)->x, 10.f, 1e-4);
    cr_assert_float_eq(reg.get_components<SPos>().get(e)->y, -2.f, 1e-4);
}

Test(scheduler_game, movement_ignores_entities_without_velocity)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SPos>();
    reg.register_component<SVel>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SPos>().insert_at(e, SPos{3.f, 3.f});
    s.add<Movement>(Order::Physics);
    s.run(reg, 1.f);
    cr_assert_float_eq(reg.get_components<SPos>().get(e)->x, 3.f, 1e-6);
}

Test(scheduler_game, damage_then_reaper_kills_entity)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t e = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(e, SHp{3});
    s.add<Damage>(Order::Update, 1);
    s.add<Reaper>(Order::SecondUpdate);
    s.run(reg, 0.f);
    s.run(reg, 0.f);
    cr_assert(reg.has_component<SHp>(e));
    s.run(reg, 0.f);
    cr_assert_not(reg.has_component<SHp>(e));
}

Test(scheduler_game, reaper_only_kills_dead_entities)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    std::size_t weak = reg.spawn_entity();
    std::size_t strong = reg.spawn_entity();
    reg.get_components<SHp>().insert_at(weak, SHp{1});
    reg.get_components<SHp>().insert_at(strong, SHp{100});
    s.add<Damage>(Order::Update, 1);
    s.add<Reaper>(Order::SecondUpdate);
    s.run(reg, 0.f);
    cr_assert_not(reg.has_component<SHp>(weak));
    cr_assert_eq(reg.get_components<SHp>().get(strong)->hp, 99);
}

Test(scheduler_game, many_entities_die_over_time)
{
    Scheduler s;
    Registry reg;
    reg.register_component<SHp>();
    for (int i = 1; i <= 10; ++i) {
        std::size_t e = reg.spawn_entity();
        reg.get_components<SHp>().insert_at(e, SHp{i});
    }
    s.add<Damage>(Order::Update, 1);
    s.add<Reaper>(Order::SecondUpdate);
    for (int frame = 1; frame <= 10; ++frame) {
        s.run(reg, 0.f);
        cr_assert_eq(reg.get_components<SHp>().size(), static_cast<std::size_t>(10 - frame));
    }
}

Test(scheduler_game, destructor_destroys_all_systems)
{
    int destroyed = 0;
    {
        Scheduler s;
        for (int i = 0; i < 7; ++i)
            s.add<DtorTracker>(static_cast<Order>(i % 5), destroyed);
    }
    cr_assert_eq(destroyed, 7);
}

#define SCHED_N_SYSTEMS_TEST(N) \
    Test(scheduler_param, n_systems_##N) \
    { \
        Scheduler s; \
        Registry reg; \
        std::vector<Counter *> cs; \
        for (int i = 0; i < (N); ++i) \
            cs.push_back(&s.add<Counter>(static_cast<Order>(i % 5))); \
        s.start(reg); \
        for (int f = 0; f < 3; ++f) \
            s.run(reg, 1.f); \
        s.stop(reg); \
        for (auto *c : cs) { \
            cr_assert_eq(c->starts, 1); \
            cr_assert_eq(c->updates, 3); \
            cr_assert_eq(c->stops, 1); \
        } \
    }

SCHED_N_SYSTEMS_TEST(1)
SCHED_N_SYSTEMS_TEST(2)
SCHED_N_SYSTEMS_TEST(5)
SCHED_N_SYSTEMS_TEST(10)
SCHED_N_SYSTEMS_TEST(50)

#define SCHED_STAGE_TEST(STAGE) \
    Test(scheduler_param, single_system_in_##STAGE) \
    { \
        Scheduler s; \
        Registry reg; \
        Log log; \
        s.add<Recorder>(Order::STAGE, log, #STAGE); \
        s.start(reg); \
        s.run(reg, 0.f); \
        s.stop(reg); \
        Log expected{#STAGE ":start", #STAGE ":update", #STAGE ":stop"}; \
        cr_assert(log == expected); \
    }

SCHED_STAGE_TEST(Input)
SCHED_STAGE_TEST(Update)
SCHED_STAGE_TEST(Physics)
SCHED_STAGE_TEST(SecondUpdate)
SCHED_STAGE_TEST(Render)

#define SCHED_KILL_STAGE_TEST(STAGE) \
    Test(scheduler_param, kill_from_##STAGE##_flushed_by_end_of_run) \
    { \
        Scheduler s; \
        Registry reg; \
        reg.register_component<SHp>(); \
        std::size_t e = reg.spawn_entity(); \
        reg.get_components<SHp>().insert_at(e, SHp{1}); \
        s.add<Killer>(Order::STAGE, e); \
        s.run(reg, 0.f); \
        cr_assert_not(reg.has_component<SHp>(e)); \
        cr_assert_eq(reg.spawn_entity(), e); \
    }

SCHED_KILL_STAGE_TEST(Input)
SCHED_KILL_STAGE_TEST(Update)
SCHED_KILL_STAGE_TEST(Physics)
SCHED_KILL_STAGE_TEST(SecondUpdate)
SCHED_KILL_STAGE_TEST(Render)

#define SCHED_FRAMES_TEST(F) \
    Test(scheduler_param, run_##F##_frames) \
    { \
        Scheduler s; \
        Registry reg; \
        Counter &c = s.add<Counter>(Order::Update); \
        for (int i = 0; i < (F); ++i) \
            s.run(reg, 0.5f); \
        cr_assert_eq(c.updates, (F)); \
        cr_assert_float_eq(c.totalDt, 0.5f * (F), 1e-3); \
    }

SCHED_FRAMES_TEST(1)
SCHED_FRAMES_TEST(10)
SCHED_FRAMES_TEST(60)
SCHED_FRAMES_TEST(144)
SCHED_FRAMES_TEST(1000)
