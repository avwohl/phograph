cmake_minimum_required(VERSION 3.16)
project(phograph_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core library
add_library(phograph_core STATIC
    src/pho_value.cc
    src/pho_prim.cc
    src/pho_prim_arith.cc
    src/pho_prim_compare.cc
    src/pho_prim_logic.cc
    src/pho_prim_string.cc
    src/pho_prim_list.cc
    src/pho_prim_dict.cc
    src/pho_prim_type.cc
    src/pho_prim_data.cc
    src/pho_prim_error.cc
    src/pho_eval.cc
    src/pho_serial.cc
    src/pho_bridge.cc
    src/pho_draw.cc
    src/pho_prim_canvas.cc
    src/pho_prim_scene.cc
    src/pho_prim_draw.cc
    src/pho_prim_input.cc
    src/pho_prim_anim.cc
    src/pho_prim_io.cc
    src/pho_thread.cc
    src/pho_platform_stub.cc
    src/pho_debug.cc
    src/pho_prim_future.cc
    src/pho_prim_channel.cc
    src/pho_prim_effect.cc
    src/pho_codegen.cc
)

target_include_directories(phograph_core PUBLIC src)

# Tests
enable_testing()

add_executable(test_eval tests/test_eval.cc)
target_link_libraries(test_eval phograph_core)
add_test(NAME test_eval COMMAND test_eval)

add_executable(test_phase2 tests/test_phase2.cc)
target_link_libraries(test_phase2 phograph_core)
add_test(NAME test_phase2 COMMAND test_phase2)

add_executable(test_phase3 tests/test_phase3.cc)
target_link_libraries(test_phase3 phograph_core)
add_test(NAME test_phase3 COMMAND test_phase3)

add_executable(test_bridge tests/test_bridge.cc)
target_link_libraries(test_bridge phograph_core)
add_test(NAME test_bridge COMMAND test_bridge)

add_executable(test_scene tests/test_scene.cc)
target_link_libraries(test_scene phograph_core)
add_test(NAME test_scene COMMAND test_scene)

add_executable(test_phase6 tests/test_phase6.cc)
target_link_libraries(test_phase6 phograph_core)
add_test(NAME test_phase6 COMMAND test_phase6)

add_executable(test_phase7 tests/test_phase7.cc)
target_link_libraries(test_phase7 phograph_core)
add_test(NAME test_phase7 COMMAND test_phase7)

add_executable(test_phase8 tests/test_phase8.cc)
target_link_libraries(test_phase8 phograph_core)
add_test(NAME test_phase8 COMMAND test_phase8)

add_executable(test_phase9 tests/test_phase9.cc)
target_link_libraries(test_phase9 phograph_core)
add_test(NAME test_phase9 COMMAND test_phase9)

add_executable(test_phase10 tests/test_phase10.cc)
target_link_libraries(test_phase10 phograph_core)
add_test(NAME test_phase10 COMMAND test_phase10)

add_executable(test_e2e_compile tests/test_e2e_compile.cc)
target_link_libraries(test_e2e_compile phograph_core)
add_test(NAME test_e2e_compile COMMAND test_e2e_compile WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
