add_library(ct_portable STATIC portable.cpp)
target_link_libraries(ct_portable PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(ct_portable PRIVATE -Wall -Wextra)
target_include_directories(ct_portable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(chronicle_play WIN32 play.cpp play.rc)
set_target_properties(chronicle_play PROPERTIES OUTPUT_NAME Play RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_link_libraries(chronicle_play PRIVATE ct_portable comctl32 comdlg32 shell32 ole32 uuid)
target_include_directories(chronicle_play PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
target_compile_definitions(chronicle_play PRIVATE UNICODE _UNICODE WINVER=0x0A00 _WIN32_WINNT=0x0A00)
target_compile_options(chronicle_play PRIVATE -Wall -Wextra)
target_link_options(chronicle_play PRIVATE -municode -static -Wl,--stack,4194304)
install(TARGETS chronicle_play RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(CT_BUILD_TESTS)
    add_executable(ct_portable_fixture fixture.cpp)
    target_link_libraries(ct_portable_fixture PRIVATE nlohmann_json::nlohmann_json)
    target_include_directories(ct_portable_fixture PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
    target_link_options(ct_portable_fixture PRIVATE -municode -static)
    add_test(NAME PortableLauncher
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_portable.py
            --launcher $<TARGET_FILE:chronicle_play> --fixture $<TARGET_FILE:ct_portable_fixture>
            --version ${KZ_DATA_VERSION})
    set_tests_properties(PortableLauncher PROPERTIES TIMEOUT 60 LABELS cpu)
endif()
