cmake_minimum_required(VERSION 3.20)

project(Aliva VERSION 0.1.0 LANGUAGES CXX)

option(ALIVA_BUILD_TESTS "Build Aliva's test suite" ON)
option(ALIVA_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

add_library(aliva_core
    src/neuron.cpp
    src/brain.cpp
)
add_library(Aliva::Core ALIAS aliva_core)

target_include_directories(aliva_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(aliva_core PUBLIC cxx_std_20)

if(ALIVA_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(aliva_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(aliva_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endif()

add_executable(aliva src/main.cpp)
target_link_libraries(aliva PRIVATE Aliva::Core)

# --- Associative-learning benchmark (experiments/association) ---------------
add_library(aliva_association_lib
    experiments/association/src/config.cpp
    experiments/association/src/patterns.cpp
    experiments/association/src/network.cpp
    experiments/association/src/experiment.cpp
    experiments/association/src/export.cpp
)
add_library(Aliva::Association ALIAS aliva_association_lib)

target_include_directories(aliva_association_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/experiments/association/include>
)
target_link_libraries(aliva_association_lib PUBLIC Aliva::Core)
target_compile_features(aliva_association_lib PUBLIC cxx_std_20)

if(ALIVA_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(aliva_association_lib PRIVATE /W4 /permissive-)
    else()
        target_compile_options(aliva_association_lib PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
    endif()
endif()

add_executable(aliva_association experiments/association/src/main.cpp)
target_link_libraries(aliva_association PRIVATE Aliva::Association)

if(ALIVA_BUILD_TESTS)
    enable_testing()
    add_executable(aliva_tests tests/test_brain.cpp)
    target_link_libraries(aliva_tests PRIVATE Aliva::Core)
    add_test(NAME aliva_tests COMMAND aliva_tests)

    add_executable(aliva_association_tests tests/test_association.cpp)
    target_link_libraries(aliva_association_tests PRIVATE Aliva::Association)
    add_test(NAME aliva_association_tests COMMAND aliva_association_tests)
endif()
