#pragma once
#include <utility>
#include <array>
#include <vector>
#include <memory>
#include "ISystem.hpp"

namespace rtk::ecs
{
    enum class Order {Input, Update, Physics, SecondUpdate, Render, Size};

    class Scheduler {
        private:
            std::array<std::vector<std::unique_ptr<ISystem>>, static_cast<size_t>(Order::Size)> _stages;

        public:
            template <typename T, typename ...Args>
            T &add(Order order, Args &&...args) {
                auto system = std::make_unique<T>(std::forward<Args>(args)...);
                T &reference = *system;
                _stages[static_cast<size_t>(order)].push_back(std::move(system));
                return reference;
            }

            void run(Registry &reg, float dt)
            {
                for (auto &stage : _stages) {
                    for (auto &system : stage) {
                        system->update(reg, dt);
                    }
                    reg.flush();
                }
            }

            void start(Registry &reg)
            {
                for (auto &stage : _stages)
                {
                    for (auto &system : stage)
                    {
                        system->onStart(reg);
                    }
                }
            }

            void stop(Registry &reg)
            {
                for (auto &stage : _stages)
                {
                    for (auto &system : stage)
                    {
                        system->onStop(reg);
                    }
                }
            }
    };
}
