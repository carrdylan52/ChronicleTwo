#pragma once

#include <cstdlib>

// Retail's exit closes the memory cards before the process ends; the host's knows nothing of them.
extern "C" [[noreturn]] void PortExit(int status);
#define exit PortExit
