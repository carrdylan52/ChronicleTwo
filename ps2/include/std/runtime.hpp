#pragma once

/**
 * Constructs array elements in allocated storage and records their size and count.
 *
 * @mangled __construct_new_array
 * @address 0x001002F0
 * @size 0x150
 */
extern "C" void *__construct_new_array(void *buffer, void *(*constructor)(void *, int),
                                     void *(*destructor)(void *, int), unsigned int size,
                                     unsigned int count);
