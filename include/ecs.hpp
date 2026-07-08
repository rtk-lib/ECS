#pragma once

#include "Logger/Logger.hpp"

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