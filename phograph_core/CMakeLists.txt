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
    src/pho_prim_math.cc
    src/pho_prim_fileio.cc
    src/pho_prim_socket.cc
    src/pho_prim_sound.cc
    src/pho_prim_midi.cc
    src/pho_prim_locale.cc
    src/pho_prim_crypto.cc
    src/pho_prim_image.cc
    src/pho_prim_bitmap.cc
    src/pho_prim_net.cc
    src/pho_prim_date.cc
    src/pho_prim_methodref.cc
    src/plugins/SoundPlugin.cpp
    src/plugins/MIDIPlugin.cpp
)

target_include_directories(phograph_core PUBLIC src)

# Use static CRT on Windows to avoid undisclosed MSVC runtime dependency
if(MSVC)
    set_target_properties(phograph_core PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

if(APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox)
    find_library(COREMIDI_LIB CoreMIDI)
    find_library(COREGRAPHICS_LIB CoreGraphics)
    find_library(IMAGEIO_LIB ImageIO)
    find_library(CORESERVICES_LIB CoreServices)
    find_library(SECURITY_LIB Security)
    find_library(FOUNDATION_LIB Foundation)
    target_link_libraries(phograph_core
        ${COREAUDIO_LIB}
        ${AUDIOTOOLBOX_LIB}
        ${COREMIDI_LIB}
        ${COREGRAPHICS_LIB}
        ${IMAGEIO_LIB}
        ${CORESERVICES_LIB}
        ${SECURITY_LIB}
        ${FOUNDATION_LIB}
    )
endif()

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

add_executable(test_comprehensive tests/test_comprehensive.cc)
target_link_libraries(test_comprehensive phograph_core)
add_test(NAME test_comprehensive COMMAND test_comprehensive)

add_executable(test_phase11_28 tests/test_phase11_28.cc)
target_link_libraries(test_phase11_28 phograph_core)
add_test(NAME test_phase11_28 COMMAND test_phase11_28)
