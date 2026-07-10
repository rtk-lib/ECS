#pragma once

#include "Registry.hpp"

namespace rtk::ecs  {
    template <typename FirstComponent, typename... OtherComponents>
    class View {
    private:
        Registry& _registry;

        const std::vector<std::size_t>& _driver_dense_array;

    public:
        View(Registry& reg, const std::vector<std::size_t>& driver_array) 
            : _registry(reg), _driver_dense_array(driver_array) {}

        class Iterator {
            private:
                Registry& _reg;
                const std::vector<std::size_t>& _dense;
                std::size_t _index;

                bool has_all_others(std::size_t entity_id) {
                    if constexpr (sizeof...(OtherComponents) == 0) {
                        return true;
                    } else {
                        return (_reg.has_component<OtherComponents>(entity_id) && ...);
                    }
                }
            
                void skip_invalid() {
                    while (_index < _dense.size() && !has_all_others(_dense[_index])) {
                        _index++;
                    }
                }
            
            public:
                Iterator(Registry& reg, const std::vector<std::size_t>& dense, std::size_t start_idx) 
                    : _reg(reg), _dense(dense), _index(start_idx) {
                    skip_invalid();
                }
            
                Iterator& operator++() {
                    _index++;
                    skip_invalid();
                    return *this;
                }
            
                std::size_t operator*() const {
                    return _dense[_index];
                }
            
                bool operator!=(const Iterator& other) const {
                    return _index != other._index;
                }
            };
        
        Iterator begin() {
            return Iterator(_registry, _driver_dense_array, 0);
        }
    
        Iterator end() {
            return Iterator(_registry, _driver_dense_array, _driver_dense_array.size());
        }
    };
}