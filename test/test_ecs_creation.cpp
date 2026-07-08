#include <criterion/criterion.h>
#include "ecs.hpp"

Test(SparseArray, creation) {
    rtk::ecs::SparseArray<int> array;
    cr_assert_eq(array.size(), 0, "The array must be empty");
}

Test(SparseArray, insert_and_resize) {
    rtk::ecs::SparseArray<float> array;

    array.insert_at(3, 42.5f);

    cr_assert_eq(array.size(), 4, "The size of the array must be 4");

    cr_assert_eq(array._data[3].value(), 42.5f, "basic test");

    cr_assert_not(array._data[0].has_value());
    cr_assert_not(array._data[1].has_value());
    cr_assert_not(array._data[2].has_value());
}
