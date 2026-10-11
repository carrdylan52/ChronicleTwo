#pragma once

// SetStack(stack, f(stack++)) at actscript.cpp:475 and 656 relies on MWCC's argument order, not yet checked.
#pragma clang diagnostic ignored "-Wunsequenced"
