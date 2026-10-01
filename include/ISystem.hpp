#pragma once
#include "Registry.hpp"

namespace rtk::ecs
{
    class Registry;

    class ISystem 
    {
        private:
        public:
            ISystem() = default;
            virtual ~ISystem() = default;
            ISystem(const ISystem &s) = delete;
            ISystem &operator=(const ISystem &s) = delete;

            virtual void update(Registry &reg, float dt) = 0;
            virtual void onStart(Registry &reg) = 0;
            virtual void onStop(Registry &reg) = 0;
    };
}
