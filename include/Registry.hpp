#pragma once

#include <vector>
#include <cstddef>
#include "Assert.hpp"
#include "SparseArray.hpp"
#include "ComponentType.hpp"

namespace rtk::ecs
{
    //fwd
    template <typename FirstComponent, typename... OtherComponents>
    class View;

    /**
     * @brief Erasure structure without any VTable overhead.
     * Holds a raw pointer to the array and static function pointers to manipulate it.
     */
    struct ErasedPool {
        void* poolPtr = nullptr;
        void (*eraseFn)(void*, std::size_t) = nullptr;
        void (*deleteFn)(void*) = nullptr;
    };

    class Registry {
    private:
        std::vector<ErasedPool> _pools;

        std::size_t _entitiesCount = 0;
        std::vector<std::size_t> _deadEntities;
        std::vector<std::size_t> _pendingKills;
        std::vector<std::size_t> _freeIds;
        std::vector<bool> _alives;

    public:
        Registry() = default;

        /**
         * @brief Destructor strictly cleans up the memory allocated for the raw pointers.
         */
        ~Registry() {
            for (auto& pool : _pools) {
                if (pool.poolPtr && pool.deleteFn) {
                    pool.deleteFn(pool.poolPtr);
                }
            }
        }

        /**
         * @brief Registers a new component type and allocates its SparseArray.
         */
        template <typename Component>
        SparseArray<Component>&
        register_component() {
            std::size_t id = ComponentType::get_id<Component>();

            if (id >= _pools.size()) {
                _pools.resize(id + 1);
            }

            RTK_ASSERT(_pools[id].poolPtr == nullptr, "Component already registered!");

            auto *newPool = new SparseArray<Component>();
            _pools[id].poolPtr = newPool;
            _pools[id].eraseFn = [](void *ptr, std::size_t entity) {
                static_cast<SparseArray<Component>*>(ptr)->erase(entity);
            };
            _pools[id].deleteFn = [](void *ptr) {
                delete static_cast<SparseArray<Component>*>(ptr);
            };
            return *newPool;
        }

        /**
         * @brief Retrieves the component array. Extremely fast (Fast Path).
         */
        template <typename Component>
        SparseArray<Component>& get_components() {
            std::size_t id = ComponentType::get_id<Component>();
            RTK_ASSERT(id < _pools.size() && _pools[id].poolPtr != nullptr, "Tried to get an unregistered component!");
            return *static_cast<SparseArray<Component>*>(_pools[id].poolPtr);
        }

        /**
         * @brief Retrieves the component array for ReadOnly. Extremely fast (Fast Path).
         */
        template <typename Component>
        const SparseArray<Component>& get_components() const {
            std::size_t id = ComponentType::get_id<Component>();

            RTK_ASSERT(id < _pools.size() && _pools[id].poolPtr != nullptr, "Tried to get an unregistered component!");

            return *static_cast<const SparseArray<Component>*>(_pools[id].poolPtr);
        }

        /**
         * @brief Erases an entity globally from ALL registered component arrays.
         * This calls the stored function pointers to trigger the Swap & Pop everywhere.
         */
        void remove_entity_from_all_pools(std::size_t entity) {
            for (auto& pool : _pools) {
                if (pool.poolPtr && pool.eraseFn) {
                    pool.eraseFn(pool.poolPtr, entity);
                }
            }
        }

        /**
         * @brief Spawns a new entity. Reuses old IDs to prevent memory fragmentation.
         * @return std::size_t The unique Entity ID.
         */
        std::size_t spawn_entity() {
            if (!_freeIds.empty()) {
                std::size_t id = _freeIds.back();
                _freeIds.pop_back();
                _alives[id] = true;
                return id;
            }
            _alives.push_back(true);
            return _entitiesCount++;
        }
        
        /**
         * @brief Kills an entity, erases all its components, and recycles its ID.
         * @param entity The Entity ID to destroy.
         */
        void kill_entity(std::size_t entity) 
        {
            if (entity < _alives.size() && _alives[entity]) {
                _pendingKills.push_back(entity);
            }
        }

        void flush() {

            for (std::size_t entity : _pendingKills){
                if (_alives[entity])
                    continue;
                _alives[entity] = false;
                remove_entity_from_all_pools(entity); 
                _freeIds.push_back(entity);
            }
            _pendingKills.clear();
        }

        template <typename Component>
        bool has_component(std::size_t entity) {
            return get_components<Component>().contains(entity);
        }

        template <typename FirstComponent, typename... OtherComponents>
        View<FirstComponent, OtherComponents...> view();
    };
} // namespace rtk::ecs

#include "View.hpp"

namespace rtk::ecs {
    template <typename FirstComponent, typename... OtherComponents>
    View<FirstComponent, OtherComponents...> Registry::view() {
        auto& driverPool = get_components<FirstComponent>();
        return View<FirstComponent, OtherComponents...>(
            driverPool.get_packed_array(),
            get_components<OtherComponents>()...
        );
    };
}
