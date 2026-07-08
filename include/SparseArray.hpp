#pragma once

#include <vector>
#include <optional>

#include <cstddef>

namespace rtk::ecs
{
    template <typename Component>
    class SparseArray {
    public:
        std::vector<std::optional<Component>> _data;

        SparseArray() = default;
        ~SparseArray() = default;

        std::size_t size() const {
            return _data.size();
        }

        std::optional<Component>& insert_at(std::size_t pos, Component const& comp) {
            if (pos >= _data.size())
                _data.resize(pos + 1);
            _data[pos] = comp;
            return _data[pos];
        }
    };
} // namespace rtk::ecs