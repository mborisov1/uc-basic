/*
 * common_mem.h
 *
 *  Created on: Jan 1, 2024

Copyright (c) 2024 Michael Borisov <https://github.com/mborisov1>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Custom memory manager for program, variables, input buffer,
 * and the FOR/GOSUB stack.
 *
 * Memory layout:
 * - program storage (when lines are added or removed, variable storage is purged)
 * - variables
 * - input buffer
 * - FOR/GOSUB stack (grows from top to bottom)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef unsigned basic_mem_idx_t;

typedef struct BASIC_MEM_MGR_
{
    unsigned char* base;
    basic_mem_idx_t vars_idx; // Also marks the end of the program storage area
    basic_mem_idx_t array_idx; // Marks the end of normal variables and the beginning of arrays
    basic_mem_idx_t free_idx; // End of the input buffer, free space for the stack growth
    basic_mem_idx_t stktop_idx; // Top of the FOR/GOSUB stack
    basic_mem_idx_t max_idx; // RAMtop
} BASIC_MEM_MGR;

static inline bool basic_mem_check_space(BASIC_MEM_MGR* s, unsigned size)
{
    return s->stktop_idx - s->free_idx >= size;
}

#if 0

void basic_mem_initialize(BASIC_MEM_MGR* s, void* base, unsigned max_size);

/* Low-level stack routines */
void fgstack_clear2(BASIC_MEM_MGR* s);

bool fgstack_push_byte(BASIC_MEM_MGR* s, uint8_t in);

static inline void fgstack_push_byte_nocheck(BASIC_MEM_MGR* s, uint8_t in)
{
    s->stktop_idx--;
    s->base[s->stktop_idx] = in;
}

bool fgstack_push_bytes(BASIC_MEM_MGR* s, const void* in, unsigned size);
static inline uint8_t fgstack_pop_byte(BASIC_MEM_MGR* s)
{
    return s->base[s->stktop_idx++];
}
void fgstack_pop_bytes(BASIC_MEM_MGR* s, void* out, unsigned size);
static inline fgs_idx2_t fgstack_get_top(const BASIC_MEM_MGR* s) { return s->stktop_idx; }
static inline void fgstack_set_top(BASIC_MEM_MGR* s, fgs_idx2_t top) {s->stktop_idx = top; }

/* Low-level variable memory management */
void variable_storage_clear2(BASIC_MEM_MGR* s);
#endif
