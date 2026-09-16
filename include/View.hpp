#pragma once

#include <vector>
#include <tuple>

namespace rtk::ecs {
    template <typename FirstComponent, typename... OtherComponents>
    class View {
    private:
        const std::vector<std::size_t>& _driver_dense_array;
        std::tuple<SparseArray<OtherComponents>&...> _other_pools;

    public:
        View(const std::vector<std::size_t>& driver_array, SparseArray<OtherComponents>&... others)
             : _driver_dense_array(driver_array), _other_pools(others...) {}

        class Iterator {
        private:
            const std::vector<std::size_t>& _dense;
            std::tuple<SparseArray<OtherComponents>&...>& _pools;
            std::size_t _index;

            bool has_all_others(std::size_t entity_id) {
                if constexpr (sizeof...(OtherComponents) == 0) {
                    return true;
                } else {
                    return std::apply(
                        [entity_id](auto&... pool) {
                            return (pool.contains(entity_id) && ...);
                        },
                        _pools
                    );
                }
            }

            void skip_invalid() {
                while (_index < _dense.size() && !has_all_others(_dense[_index])) {
                    _index++;
                }
            }

        public:
            Iterator(const std::vector<std::size_t>& dense, std::tuple<SparseArray<OtherComponents>&...>& pools, std::size_t start_idx)
                : _dense(dense), _pools(pools), _index(start_idx) {
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
            return Iterator(_driver_dense_array, _other_pools, 0);
        }

        Iterator end() {
            return Iterator(_driver_dense_array, _other_pools, _driver_dense_array.size());
        }
    };
}