# PC port

`PLATFORM=PC` builds the game's PAL code as a native program with clang, as
C++26, on SDL3 and Vulkan 1.4. The renderer and platform layer are Kazarov's
shared port library (`tools/kazarov/port`, `kz_port_library` in
`tools/kazarov/cmake/KazarovPort.cmake`). The port does not run the game yet:
`darkchronicle` exits with code 70.

## Building

```sh
./build.sh linux-x64
(cd port/build/pc && ctest --output-on-failure -j4)
```

`linux-x64` (Debug, `port/build/pc`) and `linux-x64-release`
(`port/build/pc-release`) build in the dev container through the presets in
`CMakePresets.json`; `macos-arm64[-release]` build on the host;
`windows-x64[-release]` build on Windows. `./build.sh ps2-pal` still builds and
verifies the retail executable and stays the default.

A device needs Vulkan 1.4 (`MIN_VULKAN` in `port/CMakeLists.txt`).

## Exit codes

| Code | Meaning |
|---|---|
| 0 | normal exit |
| 3 | no game data |
| 4 | no Vulkan 1.4 device |
| 70 | a PS2 hardware stub was reached (`Ps2Unimplemented`) |

## Environment

`DC2_DATA`, `DC2_SAVE`, `DC2_AUDIO=off`, `DC2_GFX_OFFSCREEN=1`.

## Layout

| Path | Contents |
|---|---|
| `port/CMakeLists.txt` | the targets below |
| `port/include/port.h` | force-included ahead of every unit of `ps2/src` and `port/src` |
| `port/include/stubs/<unit>.hpp` | force-included ahead of that unit of `ps2/src` only |
| `port/src/` | the port's own code; a definition tagged `PORT_OVERRIDE` replaces the one `ps2/src` gives the same name |
| `port/src/tests/` | `darkchronicle_tests` (GoogleTest), with Kazarov's `port/tests` |
| `port/icon/` | the window icon as raw 128×128 RGBA |

| Target | What it is |
|---|---|
| `dc_options` | `PORT PAL`, `port.h`, the game include paths |
| `dc_ps2` | the 149 units of `ps2/src`, each a copy without the definitions `port/src` overrides |
| `dc_host` | Kazarov's `gfx` and `platform`, built without any game header |
| `dc_port` | `port/src` except `main.cpp` and the tests |
| `darkchronicle`, `darkchronicle_tests` | linked by `dc_link_game()` |
