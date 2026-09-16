cmake_minimum_required(VERSION 3.20)

project(rtk_ecs
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(rtk_ecs INTERFACE)

target_include_directories(rtk_ecs
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(rtk_ecs INTERFACE cxx_std_20)

add_executable(ecs_smoke_test
    test/smoke_test.cpp
)

target_link_libraries(ecs_smoke_test
    PRIVATE rtk_ecs
)

enable_testing()

add_test(
    NAME ecs_smoke_test
    COMMAND ecs_smoke_test
)