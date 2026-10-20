# Opt-in donor library. No ct_options, rewritten DC2 headers, or DC2 SDK shims.
if(NOT EXISTS "${IXION_DC1_SOURCE_DIR}/ps2/src/charakey.cpp" OR
   NOT EXISTS "${IXION_DC1_SOURCE_DIR}/tools/kazarov/include/kazarov/port.h")
    message(FATAL_ERROR "IXION_DC1_SOURCE_DIR needs a populated Chronicle checkout")
endif()
add_library(ixdc1_combat STATIC charakey.cpp shim.cpp vu0.cpp)
# A provenance/checker edit must rerun the archive gate on incremental builds.
set_property(SOURCE shim.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/helpers.json;${CMAKE_CURRENT_SOURCE_DIR}/check_symbols.py")
# Xiao R1: PAL_TIMING as retail's PAL build defines it (Chronicle ps2/CMakeLists.txt:51). It picks Play_Jinn's
# hand-aim window (21.0-21.8, gameloop.cpp:7749-7753) and SetMotionSpeed's 6/5 (character.hpp:183), which charakey.cpp
# never calls.
target_compile_definitions(ixdc1_combat PRIVATE PORT PAL PAL_TIMING
    IXION_DC1_CHARAKEY_PATH="${IXION_DC1_SOURCE_DIR}/ps2/src/charakey.cpp")
target_include_directories(ixdc1_combat PRIVATE
    ${CMAKE_SOURCE_DIR}/port/include
    ${IXION_DC1_SOURCE_DIR}/port/include
    ${IXION_DC1_SOURCE_DIR}/tools/kazarov/include
    ${IXION_DC1_SOURCE_DIR}/ps2/include
    ${IXION_DC1_SOURCE_DIR}/ps2/include/sce)
target_compile_options(ixdc1_combat PRIVATE
    "SHELL:-include \"${IXION_DC1_SOURCE_DIR}/port/include/port.h\""
    -fms-extensions -ffunction-sections -fdata-sections -ffp-contract=off
    -ftrivial-auto-var-init=zero -fno-strict-return -fno-strict-aliasing -fwrapv
    -Wno-register -Wno-c++11-narrowing -Wno-return-mismatch -Wno-writable-strings
    -Wno-unknown-pragmas -Wno-mismatched-tags -Wno-pointer-to-int-cast
    -Wno-int-to-pointer-cast -Wno-overloaded-virtual)
if(WIN32)
    target_compile_options(ixdc1_combat PRIVATE -Xclang -mno-constructor-aliases)
endif()
add_custom_command(TARGET ixdc1_combat POST_BUILD
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/check_symbols.py
        --nm ${CMAKE_NM} --archive $<TARGET_FILE:ixdc1_combat>
        --donor ${IXION_DC1_SOURCE_DIR} --helpers ${CMAKE_CURRENT_SOURCE_DIR}/helpers.json
    VERBATIM)

# A separate headless executable avoids rebuilding/running the whole game suite.
if(CT_BUILD_TESTS)
    add_executable(ixdc1_toan_tests toan_test.cpp goro_test.cpp ungaga_test.cpp xiao_test.cpp)
    target_include_directories(ixdc1_toan_tests PRIVATE ${CMAKE_SOURCE_DIR}/port/include)
    target_link_libraries(ixdc1_toan_tests PRIVATE ixdc1_combat GTest::gtest_main)
    set_target_properties(ixdc1_toan_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    gtest_discover_tests(ixdc1_toan_tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 5)
    if(WIN32)
        include(${KZ_ROOT}/cmake/KazarovWindowsRuntime.cmake)
        kz_windows_runtime(ixdc1_toan_tests)
    endif()
endif()
