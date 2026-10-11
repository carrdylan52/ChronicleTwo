#pragma once

// _CD_SEEK (event_func.cpp:5890) passes an unset pointer to sceCdSearchFile, which the port replaces.
#pragma clang diagnostic ignored "-Wuninitialized"
