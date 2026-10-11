#include <cstdio>
#include <cstdlib>

// Exit code 70 rather than abort, so a trace tells a hardware stub apart from a crash.
[[noreturn]] void Ps2Unimplemented(const char *function, const char *file, int line) {
    std::fprintf(stderr, "PS2 hardware stub reached: %s (%s:%d)\n", function, file, line);
    std::fflush(nullptr);
    std::_Exit(70);
}
