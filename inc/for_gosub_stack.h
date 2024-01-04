/*
 * for_gosub_stack.h
 *
 *  Created on: Dec 1, 2023

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
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "common_mem.h"
#include "variable_storage.h"

typedef struct FGS_ENTRY_GOSUB_
{
    // The line number is only required here to set the current
    // execution line number for possible error message printing.
    uint16_t line;
    uint16_t parse_idx; // Return parse index into the program storage
} FGS_ENTRY_GOSUB;

typedef struct FGS_ENTRY_FOR_
{
    // The line number is only required here to set the current
    // execution line number for possible error message printing.
    uint16_t line; // Line where the FOR statement is
    uint16_t parse_idx; // Parse index into the program storage on loop continuation
    float to_val;
    float step;
    var_name_packed vn;
} FGS_ENTRY_FOR;

void fgstack_initialize(BASIC_MEM_MGR* s, void* base, unsigned size);
void fgstack_clear(BASIC_MEM_MGR* s);
bool fgstack_push_gosub(BASIC_MEM_MGR* s, const FGS_ENTRY_GOSUB* in);
bool fgstack_pop_gosub(BASIC_MEM_MGR* s, FGS_ENTRY_GOSUB* out);
bool fgstack_lookup_for(BASIC_MEM_MGR* s, FGS_ENTRY_FOR* out, var_name_packed vn);
bool fgstack_push_for(BASIC_MEM_MGR* s, const FGS_ENTRY_FOR* in);

bool fgstack_push_expression(BASIC_MEM_MGR* s, const void* in, unsigned size);
static inline void fgstack_push_expression_byte_nocheck(BASIC_MEM_MGR* s, uint8_t in)
{
    s->stktop_idx--;
    s->base[s->stktop_idx] = in;
}
void fgstack_pop_expression(BASIC_MEM_MGR* s, void* out, unsigned size);
static inline bool fgstack_check_space(BASIC_MEM_MGR* s, unsigned size)
{
    return basic_mem_check_space(s, size);
}
static inline basic_mem_idx_t fgstack_get_top(const BASIC_MEM_MGR* s) { return s->stktop_idx; }
static inline void fgstack_set_top(BASIC_MEM_MGR* s, basic_mem_idx_t top) {s->stktop_idx = top; }
