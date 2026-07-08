#pragma once

#include <cstddef>

namespace rtk::ecs {
    /**
     * @brief Utility class to generate unique, contiguous IDs for component types at compile-time.
     * This entirely replaces the slow RTTI (std::type_index).
     */
    class ComponentType {
    private:
        static inline std::size_t _next_id = 0;

    public:
        /**
         * @brief Get the unique ID for a specific Component type.
         * The static variable guarantees that the ID is generated only once per type.
         */
        template <typename Component>
        static std::size_t get_id() {
            static const std::size_t id = _next_id++;
            return id;
        }
    };
} // namespace rtk::ecs