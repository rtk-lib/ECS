#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include <utility>

namespace rtk::ecs
{
    template <typename Component>
    class SparseArray {
        public:
            std::vector<std::optional<std::size_t>> _sparse;
            std::vector<Component> _dense;
            std::vector<std::size_t> _truc_packed;

            SparseArray() = default;
            ~SparseArray() = default;

            std::size_t size() const {
                return _dense.size();
            }

            /**
             * @brief Insert a component for a specific entity.
             */
            Component& insert_at(std::size_t entity, Component const& comp) {
                if (entity < _sparse.size() && _sparse[entity].has_value()) {
                    std::size_t dense_index = _sparse[entity].value();
                    _dense[dense_index] = comp;
                    return _dense[dense_index];
                }

                
                if (entity >= _sparse.size()) {
                    _sparse.resize(entity + 1, std::nullopt);
                }

                _dense.push_back(comp);

                _truc_packed.push_back(entity);

                std::size_t new_dense_index = _dense.size() - 1;
                _sparse[entity] = new_dense_index;

                return _dense.back();
            }

            /**
             * @brief Erase a component using the "Swap & Pop" technique to keep memory contiguous.
             */
            void erase(std::size_t entity) {
                if (entity >= _sparse.size() || !_sparse[entity].has_value()) {
                    return;
                }

                std::size_t index_to_remove = _sparse[entity].value();
                std::size_t index_of_last = _dense.size() - 1;

                if (index_to_remove != index_of_last) {

                    _dense[index_to_remove] = std::move(_dense[index_of_last]);
                    
                    _truc_packed[index_to_remove] = _truc_packed[index_of_last];
                    std::size_t entity_we_just_moved = _truc_packed[index_to_remove];
                    _sparse[entity_we_just_moved] = index_to_remove;
                }

                _dense.pop_back();
                _truc_packed.pop_back();

                _sparse[entity] = std::nullopt;
            }

            /**
             * @brief Get a component by entity ID.
             */
            std::optional<Component> get(std::size_t entity) const {
                if (entity < _sparse.size() && _sparse[entity].has_value()) {
                    return _dense[_sparse[entity].value()];
                }
                return std::nullopt;
            }
    };
} // namespace rtk::ecs
