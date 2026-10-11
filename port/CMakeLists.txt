# The x64 port: the game's code built with clang as C++26 for the host.
#
# ps2/src holds only the game's code as the PS2 build compiles it. A definition
# port/src tags PORT_OVERRIDE replaces the one ps2/src gives the same name: the
# port compiles a copy of each unit without those definitions
# (tools/kazarov/scripts/port_override.py) and links it with port/src.
#
# The port is always the PAL release.

if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++)
endif()
enable_language(CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(KZ_ROOT ${CMAKE_SOURCE_DIR}/tools/kazarov)
if(NOT EXISTS ${KZ_ROOT}/cmake/KazarovPort.cmake)
    message(FATAL_ERROR "${KZ_ROOT} has no port library: run git submodule update --init tools/kazarov")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(SDL3 REQUIRED CONFIG)
find_package(Vulkan 1.4 REQUIRED)
include(FetchContent)
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    FIND_PACKAGE_ARGS CONFIG)
FetchContent_MakeAvailable(nlohmann_json)
include(${KZ_ROOT}/cmake/KazarovPort.cmake)

# Kazarov's platform/ttffont.cpp and platform/glyphs.cpp include these single-file libraries
# (public domain or MIT), pinned; a build sandbox without network supplies them in stb/.
if(EXISTS ${CMAKE_SOURCE_DIR}/stb/stb_truetype.h AND EXISTS ${CMAKE_SOURCE_DIR}/stb/stb_image.h)
    set(STB_DIR ${CMAKE_SOURCE_DIR}/stb)
else()
    set(STB_DIR ${CMAKE_BINARY_DIR}/stb)
    file(DOWNLOAD https://raw.githubusercontent.com/nothings/stb/2c980bb59875b0d32144a71867fbdebb2f77cd20/stb_truetype.h
        ${STB_DIR}/stb_truetype.h EXPECTED_HASH SHA256=ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab)
    file(DOWNLOAD https://raw.githubusercontent.com/nothings/stb/2c980bb59875b0d32144a71867fbdebb2f77cd20/stb_image.h
        ${STB_DIR}/stb_image.h EXPECTED_HASH SHA256=594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3)
endif()

set(PS2_DIR ${CMAKE_SOURCE_DIR}/ps2/src)
set(PORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE PS2_SOURCES CONFIGURE_DEPENDS RELATIVE ${PS2_DIR} ${PS2_DIR}/*.cpp)
file(GLOB_RECURSE PORT_SOURCES CONFIGURE_DEPENDS ${PORT_DIR}/*.cpp)
list(FILTER PORT_SOURCES EXCLUDE REGEX "/tests/")
list(FILTER PS2_SOURCES EXCLUDE REGEX "(^|/)tmp[^/]*$")

set(PS2_UNITS "")
set(PORT_DEFINITIONS PORT PAL)
list(TRANSFORM PORT_DEFINITIONS PREPEND --define= OUTPUT_VARIABLE port_override_defines)
# The names port/src's PORT_OVERRIDE definitions link under. The list is rewritten
# only when it changes, so other edits to port/src rebuild nothing of ps2/src.
# PORT_OVERRIDE_SOURCES changes when a unit or a stub header comes or goes, which no
# timestamp shows.
set(PORT_OVERRIDE_SCRIPT ${KZ_ROOT}/scripts/port_override.py)
set(PORT_OVERRIDES ${CMAKE_BINARY_DIR}/port_overrides.txt)
set(PORT_OVERRIDE_SOURCES ${CMAKE_BINARY_DIR}/port_override_sources.txt)
file(GLOB_RECURSE PORT_OVERRIDE_STUBS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/port/include/stubs/*.hpp)
string(JOIN "\n" port_override_sources ${PORT_SOURCES} ${PS2_SOURCES} ${PORT_OVERRIDE_STUBS})
file(CONFIGURE OUTPUT ${PORT_OVERRIDE_SOURCES} CONTENT "${port_override_sources}\n" @ONLY)
add_custom_command(OUTPUT ${PORT_OVERRIDES}
    COMMAND ${Python3_EXECUTABLE} ${PORT_OVERRIDE_SCRIPT} list ${PORT_DIR} -o ${PORT_OVERRIDES}
    DEPENDS ${PORT_SOURCES} ${PORT_OVERRIDE_SOURCES} ${PORT_OVERRIDE_SCRIPT}
    COMMENT "Listing port/src's PORT_OVERRIDE definitions"
    VERBATIM)
# port/include/stubs/<unit>.hpp, if it exists, is included ahead of that unit
# of ps2/src: it declares what the port defines in place of code PORT leaves
# out, and supplies or renames what the unit takes from MWCC alone.
foreach(unit IN LISTS PS2_SOURCES)
    set(copy ${CMAKE_BINARY_DIR}/ps2_src/${unit})
    string(REGEX REPLACE "\\.cpp$" ".hpp" header ${CMAKE_SOURCE_DIR}/port/include/stubs/${unit})
    if(EXISTS ${header})
        set(stubs --stubs ${header})
    else()
        set(stubs "")
        set(header "")
    endif()
    add_custom_command(OUTPUT ${copy}
        COMMAND ${Python3_EXECUTABLE} ${PORT_OVERRIDE_SCRIPT} strip --overrides ${PORT_OVERRIDES} ${stubs}
            ${port_override_defines} ${PS2_DIR}/${unit} ${copy}
        DEPENDS ${PS2_DIR}/${unit} ${PORT_OVERRIDES} ${PORT_OVERRIDE_SOURCES} ${PORT_OVERRIDE_SCRIPT} ${header}
        COMMENT "Removing overridden definitions from ps2/src/${unit}"
        VERBATIM)
    list(APPEND PS2_UNITS ${copy})
    list(APPEND PS2_ORIGINALS ${PS2_DIR}/${unit} ${header})
    if(header)
        set_property(SOURCE ${copy} APPEND PROPERTY COMPILE_OPTIONS -include ${header})
        set_property(SOURCE ${copy} APPEND PROPERTY OBJECT_DEPENDS ${header})
    endif()
endforeach()
# A tag that names no definition in ps2/src stops the build: its replacement
# would be unused wherever the two signatures differ.
set(PORT_OVERRIDES_CHECKED ${CMAKE_BINARY_DIR}/port_overrides.checked)
add_custom_command(OUTPUT ${PORT_OVERRIDES_CHECKED}
    COMMAND ${Python3_EXECUTABLE} ${PORT_OVERRIDE_SCRIPT} check --overrides ${PORT_OVERRIDES}
        --stubs ${CMAKE_SOURCE_DIR}/port/include/stubs ${port_override_defines} ${PS2_DIR}
        -o ${PORT_OVERRIDES_CHECKED}
    DEPENDS ${PS2_ORIGINALS} ${PORT_OVERRIDES} ${PORT_OVERRIDE_SOURCES} ${PORT_OVERRIDE_SCRIPT}
    COMMENT "Checking that every PORT_OVERRIDE definition replaces one of ps2/src's"
    VERBATIM)

# The game was written for MWCC, which accepts constructs clang rejects by
# default: pointers truncated to 32-bit integers, narrowing in braced
# initialisers and the register keyword.
add_library(dc_options INTERFACE)
target_compile_definitions(dc_options INTERFACE ${PORT_DEFINITIONS})
target_include_directories(dc_options INTERFACE
    ${CMAKE_SOURCE_DIR}/port/include ${KZ_ROOT}/include ${CMAKE_SOURCE_DIR}/ps2/include ${CMAKE_SOURCE_DIR}/ps2/include/sce)
target_compile_options(dc_options INTERFACE
    "SHELL:-include \"${CMAKE_SOURCE_DIR}/port/include/port.h\""
    -fms-extensions -ffunction-sections -fdata-sections
    -Wno-register -Wno-c++11-narrowing -Wno-return-mismatch)
# arm64 clang fuses a*b+c into one rounding by default; x86-64 has no FMA in its
# baseline. Off everywhere so the game's arithmetic rounds the same on both.
target_compile_options(dc_options INTERFACE -ffp-contract=off)

if(WIN32 AND (NOT MINGW OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    message(FATAL_ERROR "The Windows build requires clang/llvm-mingw")
endif()

add_library(dc_ps2 OBJECT ${PS2_UNITS} ${PORT_OVERRIDES_CHECKED})
target_link_libraries(dc_ps2 PRIVATE dc_options)
# Retail's code reads locals it never set and falls off the end of functions
# that return a value, and ps2/src keeps both because the match depends on
# them. MWCC gave each a defined if arbitrary result; these flags give the port
# a defined one too, along with the type punning and wraparound the code assumes.
target_compile_options(dc_ps2 PRIVATE -ftrivial-auto-var-init=zero -fno-strict-return
    -fno-strict-aliasing -fwrapv)
# What is turned off describes how MWCC-era source is spelled, which ps2/src
# cannot change without changing what MWCC compiles; what is left on warns about
# behaviour the port may still need to answer for.
target_compile_options(dc_ps2 PRIVATE -Wall -Wno-writable-strings -Wno-unknown-pragmas
    -Wno-mismatched-tags -Wno-missing-braces -Wno-char-subscripts -Wno-overloaded-virtual
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-int-to-void-pointer-cast
    -Wno-void-pointer-to-int-cast -Wno-unused-variable -Wno-unused-but-set-variable
    -Wno-unused-local-typedef -Wno-unused-label -Wno-unused-function -Wno-unused-value
    -Wno-unused-const-variable -Wno-undefined-internal -Wno-self-assign -Wno-deprecated-volatile
    -Wno-tautological-compare -Wno-shift-op-parentheses -Wno-implicit-conversion-floating-point-to-bool
    -Wno-switch -Wno-constant-conversion -Wno-invalid-offsetof -Wno-dynamic-class-memaccess
    -Wno-nontrivial-memcall -Wno-align-mismatch -Wno-format)
# -ftrivial-auto-var-init=zero and -fno-strict-return above already define these.
target_compile_options(dc_ps2 PRIVATE -Wno-sometimes-uninitialized -Wno-return-type)
# Nothing in ps2/src throws or catches; without exceptions the host's operator new takes the place of
# MWCC's, and the Metrowerks exception units it drew in are left with no referrer.
target_compile_options(dc_ps2 PRIVATE -fno-exceptions)

set(PORT_LIB_SOURCES ${PORT_SOURCES})
list(FILTER PORT_LIB_SOURCES EXCLUDE REGEX "/tests/|^${PORT_DIR}/main\\.cpp$")
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
# The SDK functions with no replacement: docs/port/ASM_LEDGER.tsv's stub rows.
kz_port_stubs(HEADERS ${CMAKE_SOURCE_DIR}/ps2/include/sce SYMBOLS ${CMAKE_SOURCE_DIR}/docs/port/ASM_LEDGER.tsv
    OUT ${GENERATED_DIR}/stubs/sce OUT_SOURCES SCE_STUB_SOURCES)

# The host side never sees a game header, so it is built without dc_options.
kz_port_library(dc_host
    APP_NAME "Dark Chronicle" APP_ID chronicletwo ENV_PREFIX DC2_ MIN_VULKAN 1.4
    ICON ${CMAKE_SOURCE_DIR}/port/icon/chronicletwo-128.rgba
    PROJECT_LABEL "ChronicleTwo on GitHub" PROJECT_URL https://github.com/TheMoonPeople/ChronicleTwo)
target_include_directories(dc_host PRIVATE ${PORT_DIR} ${STB_DIR})

add_library(dc_port OBJECT ${PORT_LIB_SOURCES} ${SCE_STUB_SOURCES})
target_link_libraries(dc_port PRIVATE dc_options SDL3::SDL3 Vulkan::Vulkan nlohmann_json::nlohmann_json)
target_include_directories(dc_port PRIVATE ${PORT_DIR} ${KZ_ROOT}/port ${GENERATED_DIR})
# The game's headers carry MWCC's pragmas.
target_compile_options(dc_port PRIVATE -Wall -Wno-mismatched-tags -Wno-unused-label
    -Wno-overloaded-virtual -Wno-unused-function -Wno-unknown-pragmas)

if(WIN32)
    set(DC_LINK_OPTIONS -fuse-ld=lld -Wl,--gc-sections -Wl,--error-limit=0)
elseif(APPLE)
    set(DC_LINK_OPTIONS -Wl,-dead_strip)
else()
    set(DC_LINK_OPTIONS -fuse-ld=lld -pie -Wl,-z,separate-code -Wl,--gc-sections -Wl,--error-limit=0)
endif()

function(dc_link_game target)
    target_sources(${target} PRIVATE $<TARGET_OBJECTS:dc_port> $<TARGET_OBJECTS:dc_host> $<TARGET_OBJECTS:dc_ps2>)
    target_link_libraries(${target} PRIVATE dc_options SDL3::SDL3 Vulkan::Vulkan nlohmann_json::nlohmann_json)
    target_include_directories(${target} PRIVATE ${PORT_DIR} ${KZ_ROOT}/port ${GENERATED_DIR})
    target_compile_options(${target} PRIVATE -Wall -Wno-mismatched-tags -Wno-unused-label
        -Wno-overloaded-virtual -Wno-unused-function -Wno-unknown-pragmas)
    target_link_options(${target} PRIVATE ${DC_LINK_OPTIONS})
endfunction()

add_executable(darkchronicle ${PORT_DIR}/main.cpp)
set_target_properties(darkchronicle PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
dc_link_game(darkchronicle)

include(GNUInstallDirs)
install(TARGETS darkchronicle RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# The tests use an installed GoogleTest, or fetch one at configure time; a build with neither turns them off.
option(DC_BUILD_TESTS "Build darkchronicle_tests" ON)
if(DC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()
