/*
 * variable_storage.h
 *
 *  Created on: Nov 20, 2023

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
#include "basic_errors.h"

typedef uint16_t var_name_packed;

#pragma pack(push,1)
typedef struct VARIABLE_VALUE_
{
    float f; // Unaligned
} VARIABLE_VALUE;
#pragma pack(pop)

static inline var_name_packed var_name_empty(void) {return 0;}
static inline var_name_packed var_name_add_char(var_name_packed n, unsigned char c)
{
    return n << 8 | c;
}

void variable_storage_initialize(BASIC_MEM_MGR* s, unsigned char* base, unsigned size);
VARIABLE_VALUE* variable_storage_create_var(BASIC_MEM_MGR* s, var_name_packed var);
enum BASIC_ERROR_ID variable_storage_create_array_var(BASIC_MEM_MGR* s, var_name_packed var, VARIABLE_VALUE** ppv, unsigned subscript, bool dim);
float variable_storage_read_var(BASIC_MEM_MGR* s, var_name_packed var);
void variable_storage_clear(BASIC_MEM_MGR* s);
