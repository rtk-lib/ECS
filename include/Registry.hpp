#pragma once

#include <vector>
#include <cstddef>
#include "SparseArray.hpp"
#include "ComponentType.hpp"

namespace rtk::ecs {
    
    /**
     * @brief Erasure structure without any VTable overhead.
     * Holds a raw pointer to the array and static function pointers to manipulate it.
     */
    struct ErasedPool {
        void* pool_ptr = nullptr;
        void (*erase_fn)(void*, std::size_t) = nullptr;
        void (*delete_fn)(void*) = nullptr;
    };

    class Registry {
    private:
        std::vector<ErasedPool> _pools;

    public:
        Registry() = default;

        /**
         * @brief Destructor strictly cleans up the memory allocated for the raw pointers.
         */
        ~Registry() {
            for (auto& pool : _pools) {
                if (pool.pool_ptr && pool.delete_fn) {
                    pool.delete_fn(pool.pool_ptr);
                }
            }
        }

        /**
         * @brief Registers a new component type and allocates its SparseArray.
         */
        template <typename Component>
        SparseArray<Component>& register_component() {
            std::size_t id = ComponentType::get_id<Component>();

            if (id >= _pools.size()) {
                _pools.resize(id + 1);
            }

            auto* new_pool = new SparseArray<Component>();

            _pools[id].pool_ptr = new_pool;
            

            _pools[id].erase_fn = [](void* ptr, std::size_t entity) {
                static_cast<SparseArray<Component>*>(ptr)->erase(entity);
            };

            _pools[id].delete_fn = [](void* ptr) {
                delete static_cast<SparseArray<Component>*>(ptr);
            };

            return *new_pool;
        }

        /**
         * @brief Retrieves the component array. Extremely fast (Fast Path).
         */
        template <typename Component>
        SparseArray<Component>& get_components() {
            std::size_t id = ComponentType::get_id<Component>();            
            return *static_cast<SparseArray<Component>*>(_pools[id].pool_ptr);
        }

        /**
         * @brief Erases an entity globally from ALL registered component arrays.
         * This calls the stored function pointers to trigger the Swap & Pop everywhere.
         */
        void remove_entity_from_all_pools(std::size_t entity) {
            for (auto& pool : _pools) {
                if (pool.pool_ptr && pool.erase_fn) {
                    pool.erase_fn(pool.pool_ptr, entity);
                }
            }
        }
    };
} // namespace rtk::ecs