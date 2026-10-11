#pragma once

// DATA.HD4, the index of DATA.DAT, as the disc stores it and as the host reads it. Plain types only,
// so the asset tools can share it with the game.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace port::records {

// One index entry on the disc. The PS2 game reads these in place and overwrites name_offset with
// a 32-bit pointer (DATA_HEADER); the host keeps the disc form and builds DataHd4Entry beside it.
struct DataHd4Disk {
    uint32_t name_offset; // from the start of the index
    int32_t  size;
    int32_t  sector; // relative to DATA.DAT
};

static_assert(sizeof(DataHd4Disk) == 0xC);

struct DataHd4Entry {
    const char *name; // inside the index buffer, with '\' turned into '/' as InitCDFile does
    int32_t     size;
    int32_t     sector;
};

// Reads the index in buffer, which must outlive the entries. The entry count is the first name
// offset divided by the entry size, since the names follow the table. Returns false for an index
// whose table or names run past the buffer.
inline bool ReadDataHd4(char *buffer, std::size_t bytes, std::vector<DataHd4Entry> &entries) {
    entries.clear();
    if (bytes < sizeof(DataHd4Disk)) {
        return false;
    }
    uint32_t table;
    std::memcpy(&table, buffer, sizeof(table));
    std::size_t count = table / sizeof(DataHd4Disk);
    if (count * sizeof(DataHd4Disk) > bytes) {
        return false;
    }
    entries.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        DataHd4Disk disk;
        std::memcpy(&disk, buffer + i * sizeof(DataHd4Disk), sizeof(disk));
        if (disk.name_offset >= bytes) {
            return false;
        }
        char *name = buffer + disk.name_offset;
        char *end = static_cast<char *>(std::memchr(name, 0, bytes - disk.name_offset));
        if (end == nullptr) {
            return false;
        }
        for (char *c = name; c != end; c++) {
            if (*c == '\\') {
                *c = '/';
            }
        }
        entries.push_back({name, disk.size, disk.sector});
    }
    return true;
}

} // namespace port::records
