#pragma once

#include "Logger/Logger.hpp"

namespace rtk
{
    class ecs
    {
        private:

        public:
            ecs()
            {
                LOG_INFO("Start ECS");
            }

            ~ecs()
            {
                LOG_INFO("End ECS");
            }
    };
}