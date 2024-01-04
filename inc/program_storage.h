/*
 * program_storage.h
 *
 *  Created on: Nov 19, 2023

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

#include <stdbool.h>
#include "common_mem.h"

typedef struct FIND_LINE_RESULT_
{
    unsigned idx;
    bool found;
} FIND_LINE_RESULT;

typedef struct PROGRAM_STORAGE_
{
    unsigned char* base;
    unsigned size;
    unsigned max_size;
} PROGRAM_STORAGE;

/* max_size should be >=3 */
void prog_storage_initialize(BASIC_MEM_MGR* prog, void* base, unsigned max_size);
void prog_storage_clear(BASIC_MEM_MGR* prog);
FIND_LINE_RESULT prog_storage_find_line(const BASIC_MEM_MGR* prog, unsigned line);
const unsigned char* prog_storage_get_line_parse_ptr(const BASIC_MEM_MGR* prog, unsigned line_idx);
const unsigned char* prog_storage_advance_line(const BASIC_MEM_MGR* prog, const unsigned char* parse_ptr, unsigned* pline);
bool prog_storage_store_line(BASIC_MEM_MGR* prog, unsigned line, const char* content);

static inline unsigned prog_storage_ptr_to_idx(BASIC_MEM_MGR* prog, const unsigned char* ptr) {return ptr - prog->base;}
static inline const unsigned char* prog_storage_idx_to_ptr(BASIC_MEM_MGR* prog, unsigned idx) { return prog->base + idx; }

void prog_storage_list(const BASIC_MEM_MGR* prog, unsigned first_line);
