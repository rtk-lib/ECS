#pragma once

#include <chrono>
#include <string>
#include "Logger/Logger.hpp"

#define TIME_START(name) \
    auto start_##name = std::chrono::high_resolution_clock::now();

#define TIME_END(name) \
    auto end_##name = std::chrono::high_resolution_clock::now(); \
    auto duration_##name = std::chrono::duration_cast<std::chrono::microseconds>(end_##name - start_##name).count(); \
    LOG_INFO(std::string("[") + #name + "]: " + std::to_string(duration_##name) + " μs");

namespace rtk::debug {
    class ScopedTimer {
    private:
        const char* _name;
        std::chrono::time_point<std::chrono::high_resolution_clock> _start;

    public:
        ScopedTimer(const char* name) : _name(name), _start(std::chrono::high_resolution_clock::now()) {}
        
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count();
            LOG_INFO(std::string("[") + _name + "]: " + std::to_string(duration) + " μs");
        }
    };
}

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#define PROFILE_SCOPE(name) rtk::debug::ScopedTimer MACRO_CONCAT(timer_, __LINE__)(name)