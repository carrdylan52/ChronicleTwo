#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "mw_runtime.h"
#include "runtime/mw_runtime.hpp"
#include "trace.hpp"

namespace {
constexpr int kExitHardwareStub = 70;
}

[[noreturn]] void PortUnimplemented(const char *function, const char *file, int line) {
    Trace("stub %s", function);
    std::fflush(stdout);
    std::fprintf(stderr, "%s:%d: %s: PlayStation 2 code is not implemented on PC\n", file, line, function);
    std::fflush(nullptr);
    std::_Exit(kExitHardwareStub);
}

// The host runs the static constructors mwInit would have run.
extern "C" void mwInit() {}

extern "C" int fptosi(float value) {
    return runtime::Fptosi(value);
}

extern "C" unsigned int fptoui(float value) {
    return runtime::Fptoui(value);
}

extern "C" [[noreturn]] void PortExit(int status) {
    runtime::Exit(status);
}

// dng_effect.cpp calls this by name with placement storage 16 bytes longer than the array, which
// MWCC's header fills with the element size and count.
extern "C" void *__construct_new_array(void *buffer, void *(*constructor)(void *), void *destructor,
                                       unsigned int element_size, int count) {
    if (buffer == nullptr) {
        return nullptr;
    }
    auto *header = static_cast<std::uint32_t *>(buffer);
    header[0] = element_size;
    header[1] = static_cast<std::uint32_t>(count);
    char *elements = static_cast<char *>(buffer) + 16;
    if (constructor != nullptr) {
        for (int i = 0; i < count; i++) {
            constructor(elements + static_cast<std::size_t>(i) * element_size);
        }
    }
    return elements;
}
