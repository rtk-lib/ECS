#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include <utility>
#include <limits>

namespace rtk::ecs
{
    static constexpr std::size_t NULL_ENTITY = std::numeric_limits<std::size_t>::max();

    template <typename Component>
    class SparseArray {
        public:
            std::vector<std::size_t> _sparse;
            std::vector<Component> _dense;
            std::vector<std::size_t> _packed_entities;

            SparseArray() = default;
            ~SparseArray() = default;

            std::size_t size() const {
                return _dense.size();
            }

            /**
             * @brief Insert a component for a specific entity.
             */
            Component& insert_at(std::size_t entity, Component const& comp) {
                if (entity < _sparse.size() && _sparse[entity] != NULL_ENTITY) {
                    std::size_t dense_index = _sparse[entity];
                    _dense[dense_index] = comp;
                    return _dense[dense_index];
                }

                if (entity >= _sparse.size()) {
                    _sparse.resize(entity + 1, NULL_ENTITY);
                }

                _dense.push_back(comp);
                _packed_entities.push_back(entity);

                std::size_t new_dense_index = _dense.size() - 1;
                _sparse[entity] = new_dense_index;

                return _dense.back();
            }

            /**
             * @brief Erase a component using the "Swap & Pop" technique to keep memory contiguous.
             */
            void erase(std::size_t entity) {
                if (entity >= _sparse.size() || _sparse[entity] == NULL_ENTITY) {
                    return;
                }

                std::size_t index_to_remove = _sparse[entity];
                std::size_t index_of_last = _dense.size() - 1;

                if (index_to_remove != index_of_last) {
                    _dense[index_to_remove] = std::move(_dense[index_of_last]);
                    _packed_entities[index_to_remove] = _packed_entities[index_of_last];
                    std::size_t entity_we_just_moved = _packed_entities[index_to_remove];
                    _sparse[entity_we_just_moved] = index_to_remove;
                }

                _dense.pop_back();
                _packed_entities.pop_back();
                _sparse[entity] = NULL_ENTITY;
            }

            /**
             * @brief Get a component by entity ID.
             */
            std::optional<Component> get(std::size_t entity) const {
                if (entity < _sparse.size() && _sparse[entity] != NULL_ENTITY) {
                    return _dense[_sparse[entity]];
                }
                return std::nullopt;
            }
    };
}