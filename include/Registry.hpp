#pragma once

#include <unordered_map>
#include <typeindex>
#include <any>
#include "SparseArray.hpp"

namespace rtk::ecs
{
    class Registry {
    private:
        std::unordered_map<std::type_index, std::any> _components_arrays;

    public:
        Registry() = default;
        ~Registry() = default;

        template <typename Component>
        SparseArray<Component>& register_component() {
            _components_arrays[typeid(Component)] = SparseArray<Component>();
            return get_components<Component>();
        }

        template <typename Component>
        SparseArray<Component>& get_components() {
            std::any& array = _components_arrays.at(typeid(Component));
            return std::any_cast<SparseArray<Component>&>(array);
        }
    };
} // namespace rtk::ecs
