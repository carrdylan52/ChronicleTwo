#include "common.h"
#include "mg_memory.hpp"
#include <cstdio>
#include <cstring>

// Code (.text)
void *MG_ADDRESS_CHECK(void *address, char *where) {
    if (address == NULL) {
        printf("stack over at %s\n", where);
        return NULL;
    }
    return address;
}

void *operator new(size_t size, u_long128 *buffer) {
    return buffer;
}

void *operator new[](size_t size, u_long128 *buffer) {
    return buffer;
}

void mgCMemory::Init() {
    heap_size = 0;
    heap = NULL;
    heap_top = NULL;
    name[0] = '\0';
    stack = NULL;
    stack_used = 0;
    stack_size = 0;
    lock = 0;
}

void mgCMemory::SetHeapMem(u_long128 *buffer, int size) {
    heap = buffer;
    heap_size = size;
    if (buffer == NULL || heap_size < 16) {
        Init();
        return;
    }

    // The first header owns no contents; the last quadword of the buffer
    // is the terminating header.
    heap_top = (mgMEMORY_BLOCK *)buffer;
    heap_top->data = NULL;
    heap_top->size = 1;
    heap_top->next = (mgMEMORY_BLOCK *)&heap[heap_size] - 1;
    heap_top->next->data = NULL;
    heap_top->next->size = 0;
    heap_top->next->next = NULL;
}

void mgCMemory::ClearHeapMem() {
    u_long128 *buffer = heap;
    u_int size = heap_size;

    Init();
    SetHeapMem(buffer, size);
}

void mgCMemory::Free(u_long128 *data) {
    mgMEMORY_BLOCK *block;
    mgMEMORY_BLOCK *prev;
    mgMEMORY_BLOCK *found;

    if (data == NULL) {
        return;
    }

    prev = NULL;
    found = NULL;
    for (block = heap_top; block->next != NULL; block = block->next) {
        if (block->data == data) {
            found = block;
            break;
        }
        prev = block;
    }

    if (found == NULL) {
        printf("Illegal Free Memory %x\n", data);
        while (true) {
        }
    }

    prev->next = found->next;
}

u_long128 *mgCMemory::StartStackMode(int mode, int size) {
    stack_block = NULL;
    mgMEMORY_BLOCK *block = heap_top;
    if (block == NULL) {
        return NULL;
    }

    mgMEMORY_BLOCK *chosen = NULL;
    mgMEMORY_BLOCK *chosen_prev = NULL;
    int free_size = 0;
    u_int largest = 0;
    mgMEMORY_BLOCK *gap_start;
    u_int gap_size;
    for (; block->next != NULL; block = block->next) {
        gap_start = &block[block->size];
        gap_size = block->next - gap_start;
        // Taken from the last gap examined, not necessarily the chosen one.
        free_size = gap_size - 1;

        if (mode == MG_STACK_MODE_FIRST && gap_size > 1) {
            chosen = gap_start;
            chosen_prev = block;
            break;
        }
        if (mode == MG_STACK_MODE_LARGEST && largest < gap_size) {
            chosen = gap_start;
            chosen_prev = block;
            largest = gap_size;
        }
        if (mode == MG_STACK_MODE_FIT && gap_size > (u_int)(size + 1)) {
            chosen = gap_start;
            chosen_prev = block;
            break;
        }
    }

    if (chosen != NULL) {
        stack_block = chosen;
        stack_block->next = chosen_prev->next;
        chosen_prev->next = stack_block;
        stack_block->size = 1;
        stack_block->data = (u_long128 *)&stack_block[1];
        stack = stack_block->data;
        stack_size = free_size;
        return stack_block->data;
    }
    return NULL;
}

void mgCMemory::EndStackMode() {
    mgMEMORY_BLOCK *block = stack_block;
    if (block != NULL) {
        u_int used = stack_used;
        block->size += used;
        stack_block = NULL;
        stack = NULL;
        stack_size = 0;
        stack_used = 0;
    }
}

u_long128 *mgCMemory::stAlloc64(int size) {
    stAlign64();
    return stAlloc(size);
}

u_long128 *mgCMemory::stAllocTest(int size) {
    if (lock) {
        return NULL;
    }
    if (stack_used + size >= stack_size) {
        printf("stack over %d/%d at %s\n", stack_used + size, stack_size, name);
        return NULL;
    }
    return &stack[stack_used];
}

u_long128 *mgCMemory::stAlloc(int size) {
    int used;
    u_long128 *data;

    if (lock) {
        return NULL;
    }
    if (size <= 0) {
        return NULL;
    }

    used = stack_used;
    if (used + size >= stack_size) {
        printf("stack over %d/%d at %s\n", used + size, stack_size, name);
        return NULL;
    }
    data = &stack[used];
    stack_used = used + size;
    return data;
}

u_long128 *mgCMemory::Alloc(int size) {
    int used;
    u_long128 *data;

    if (lock) {
        return NULL;
    }
    if (size <= 0) {
        return NULL;
    }

    used = stack_used;
    if (used + size >= stack_size) {
        printf("stack over %d/%d at %s\n", used + size, stack_size, name);
        return NULL;
    }
    data = &stack[used];
    stack_used = used + size;
    return data;
}

void mgCMemory::stAlign64() {
    u_int misalign;

    if (lock) {
        return;
    }

    misalign = (u_int)&stack[stack_used] & 0x3F;
    if (misalign != 0) {
        stack_used += (int)(64 - misalign) / 16;
    }
    if (stack_used >= stack_size) {
        stack_used = stack_size;
    }
}

void mgCMemory::Align64() {
    u_int misalign;

    if (lock) {
        return;
    }

    misalign = (u_int)&stack[stack_used] & 0x3F;
    if (misalign != 0) {
        stack_used += (int)(64 - misalign) / 16;
    }
    if (stack_used >= stack_size) {
        stack_used = stack_size;
    }
}

void mgCMemory::stSetBuffer(u_long128 *buffer, int size) {
    stack = buffer;
    stack_used = 0;
    stack_size = size;
}

char *mgCopyString(char *text, mgCMemory *memory) {
    u_int length;
    u_int quadwords;
    char *copy;

    if (text == NULL || memory == NULL) {
        return NULL;
    }

    length = strlen(text) + 1;
    quadwords = (length & 0xF) ? (length >> 4) + 1 : length >> 4;

    copy = (char *)memory->Alloc(quadwords);
    if (copy == NULL) {
        return NULL;
    }
    strcpy(copy, text);
    return copy;
}

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_memory", at_166__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_memory", at_238__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_memory", at_288__DATA);
