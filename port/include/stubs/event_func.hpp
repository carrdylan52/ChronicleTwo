#pragma once

// event_func.cpp:13975 relies on MWCC's argument order, not yet checked; _CD_SEEK (5890) passes an unset
// pointer to sceCdSearchFile, which the port replaces.
#pragma clang diagnostic ignored "-Wunsequenced"
#pragma clang diagnostic ignored "-Wuninitialized"
