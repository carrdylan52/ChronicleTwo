#include <cstring>

#include "trace.hpp"

int Ps2Main();

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--trace") == 0) {
            TraceEnable();
        }
    }
    Trace("main");
    return Ps2Main();
}
