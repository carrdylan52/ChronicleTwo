#include "trace.hpp"

#include <cstdarg>
#include <cstdio>

namespace {
bool g_enabled;
}

void TraceEnable() {
    g_enabled = true;
}

bool TraceEnabled() {
    return g_enabled;
}

void Trace(const char *format, ...) {
    if (!g_enabled) {
        return;
    }
    std::va_list args;
    va_start(args, format);
    std::fputs("trace: ", stderr);
    std::vfprintf(stderr, format, args);
    std::fputc('\n', stderr);
    va_end(args);
}
