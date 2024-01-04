/*
 * common_mem.c
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
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#include "common_mem.h"
#include <string.h>
#include <stdlib.h>

#if 0
void prog_storage_clear2(BASIC_MEM_MGR* s)
{
    /* Re-initialize the program storage memory, erasing the entire program
     * if there was any */
    unsigned char* pb = s->base;
    pb[0] = '\0'; /* A sentinel byte needed to correctly begin execution of the first line.
                     it pretends to be the end marker of the previous line */
    pb[1] = '\0'; /* 2-byte sentinel as a program-end marker */
    pb[2] = '\0';
    s->vars_idx = 3; /* The sentinels on both sides count */
}

void basic_mem_initialize(BASIC_MEM_MGR* s, void* base, unsigned max_size)
{
    s->base = base;
    s->max_idx = max_size;
    prog_storage_clear2(s);
    variable_storage_clear2(s);
    fgstack_clear2(s);
}



bool fgstack_push_byte(BASIC_MEM_MGR* s, uint8_t in)
{
    if(!fgstack_check_space2(s, 1))
    {
        /* Not enough free stack space */
        return false;
    }
    fgstack_push_byte_nocheck(s, in);
    return true;
}

bool fgstack_push_bytes(BASIC_MEM_MGR* s, const void* in, unsigned size)
{
    if(!fgstack_check_space2(s, size))
    {
        /* Not enough free stack space */
        return false;
    }
    /* Push some bytes on the stack */
    s->stktop_idx -= size;
    memcpy(s->base + s->stktop_idx , in, size);

    return true;
}

void fgstack_pop_bytes(BASIC_MEM_MGR* s, void* out, unsigned size)
{
    if(s->stktop_idx > s->max_idx - size)
    {
        /* This should never happen */
        abort();
    }
    memcpy(out, s->base + s->stktop_idx, size);
    s->stktop_idx += size;
}
#endif
