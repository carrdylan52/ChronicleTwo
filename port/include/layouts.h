#pragma once

#include <stdint.h>

#include <string_view>

#include "layout_records.h"

// The PS2 sizes STATIC_ASSERT checks hold on the host only for records without pointers;
// kPortHostLayouts names the others, which the port lays out for the host (docs/port/LAYOUTS.md).
constexpr bool PortSameSpelling(std::string_view a, std::string_view b) {
    std::size_t i = 0;
    std::size_t j = 0;
    for (;;) {
        while (i < a.size() && a[i] == ' ') {
            i++;
        }
        while (j < b.size() && b[j] == ' ') {
            j++;
        }
        if (i == a.size() || j == b.size()) {
            return i == a.size() && j == b.size();
        }
        if (a[i++] != b[j++]) {
            return false;
        }
    }
}

constexpr bool PortLayoutChecked(std::string_view assertion) {
    std::size_t open = assertion.find('(');
    std::size_t close = assertion.rfind(')', assertion.rfind("=="));
    std::string_view record = assertion.substr(open + 1, close - open - 1);
    for (std::string_view host : kPortHostLayouts) {
        if (PortSameSpelling(record, host)) {
            return false;
        }
    }
    return true;
}

#define STATIC_ASSERT(expr) static_assert(!PortLayoutChecked(#expr) || (expr), #expr)

// mgCMemory block counts the PS2 code writes as literals fit its 32-bit layouts; the two spare
// blocks match the slack those literals carry.
template <class T, int Count = 1> constexpr int PortBlocks(int blocks) {
    constexpr int host = static_cast<int>((sizeof(T) * Count + 15) / 16 + 2);
    return blocks > host ? blocks : host;
}

[[noreturn]] void Ps2Unimplemented(const char *function, const char *file, int line);

// Where the PS2 code keeps a pointer in 32 bits and the port has not yet replaced that code.
// Pointers on the host lie above 4 GiB, so reaching one of these stops the game like a stub.
inline unsigned int PortCheckedPtr32(const volatile void *pointer, const char *file, int line) {
    uintptr_t value = reinterpret_cast<uintptr_t>(pointer);
    if (value >> 32 != 0) {
        Ps2Unimplemented("pointer truncated to 32 bits", file, line);
    }
    return static_cast<unsigned int>(value);
}

#define PortPtr32(pointer) PortCheckedPtr32((pointer), __FILE__, __LINE__)

// The host size of a record or pointer whose PS2 size the code writes as a literal stride.
template <class T> constexpr int PortSize(int) {
    return static_cast<int>(sizeof(T));
}

// Code that reads a record at a PS2 byte offset the port has not yet translated.
#define PortLayoutGuard() Ps2Unimplemented("record read at a PS2 byte offset", __FILE__, __LINE__)
