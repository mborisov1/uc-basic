/*
 * for_gosub_stack.c
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

#include "for_gosub_stack.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

enum FGS_ENTRY_TAGS
{
    FGS_TAG_FOR = 0,
    FGS_TAG_GOSUB
};

void fgstack_initialize(BASIC_MEM_MGR* s, void* base, unsigned size)
{
    s->base = base;
    s->free_idx = 0;
    s->max_idx = size;
    s->stktop_idx = size;
}

void fgstack_clear(BASIC_MEM_MGR* s)
{
    s->stktop_idx = s->max_idx;
}

static bool fgstack_push(BASIC_MEM_MGR* s, const void* in, uint8_t tag, unsigned size)
{
    if(s->stktop_idx - s->free_idx < size + 1)
    {
        /* Not enough free stack space */
        return false;
    }
    s->stktop_idx -= size + 1;
    s->base[s->stktop_idx] = tag;
    memcpy(s->base + s->stktop_idx + 1, in, size);

    return true;
}

bool fgstack_push_gosub(BASIC_MEM_MGR* s, const FGS_ENTRY_GOSUB* in)
{
    return fgstack_push(s, in, FGS_TAG_GOSUB, sizeof(FGS_ENTRY_GOSUB));
}

bool fgstack_push_for(BASIC_MEM_MGR* s, const FGS_ENTRY_FOR* in)
{
    return fgstack_push(s, in, FGS_TAG_FOR, sizeof(FGS_ENTRY_FOR));
}

bool fgstack_push_expression(BASIC_MEM_MGR* s, const void* in, unsigned size)
{
    /* Expression entries are pushed without tags to save space */
    if(s->stktop_idx - s->free_idx < size)
    {
        /* Not enough free stack space */
        return false;
    }
    s->stktop_idx -= size;
    memcpy(s->base + s->stktop_idx , in, size);

    return true;
}

void fgstack_pop_expression(BASIC_MEM_MGR* s, void* out, unsigned size)
{
    if(s->stktop_idx > s->max_idx - size)
    {
        /* This should never happen */
        abort();
    }
    memcpy(out, s->base + s->stktop_idx, size);
    s->stktop_idx += size;
}

bool fgstack_pop_gosub(BASIC_MEM_MGR* s, FGS_ENTRY_GOSUB* out)
{
    basic_mem_idx_t idx = s->stktop_idx;
    while(idx < s->max_idx - sizeof(FGS_ENTRY_GOSUB))
    {
        /* Check entry type */
        if(s->base[idx] == FGS_TAG_GOSUB)
        {
            /* Found a GOSUB entry - return it and pop it from the stack */
            memcpy(out, s->base + idx + 1, sizeof(FGS_ENTRY_GOSUB));
            idx += sizeof(FGS_ENTRY_GOSUB) + 1;
            s->stktop_idx = idx;
            return true;
        }
        else if(s->base[idx] == FGS_TAG_FOR && idx < s->max_idx - sizeof(FGS_ENTRY_FOR))
        {
            /* Found a FOR entry - discard it. Break all FOR loops of the subroutine */
            idx += sizeof(FGS_ENTRY_FOR) + 1;
        }
        else
        {
            /* Unknown entry type or insufficient size.
             * TODO: return an internal error status */
            return false;
        }
    }
    /* No suitable GOSUB entry found. Leave the stack untouched and return error */
    return false;
}

bool fgstack_lookup_for(BASIC_MEM_MGR* s, FGS_ENTRY_FOR* out, var_name_packed vn)
{
    /* Scan FOR entries up the stack until the one with the given variable
     * name is found. If found, returns its contents and pops all scanned entries
     * including the given one. True is returned.
     * If not found or if a GOSUB entry is encountered, returns false without
     * touching the stack */
    basic_mem_idx_t idx = s->stktop_idx;
    while(idx < s->max_idx - sizeof(FGS_ENTRY_FOR))
    {
        /* Check entry type */
        if(s->base[idx] == FGS_TAG_FOR)
        {
            /* Found a FOR entry - check variable name */
            var_name_packed vn_cmp;
            memcpy(&vn_cmp, s->base + idx + 1 + offsetof(FGS_ENTRY_FOR, vn), sizeof(var_name_packed));
            if(vn == vn_cmp)
            {
                /* Found the desired variable entry - return its contents to the caller */
                memcpy(out, (FGS_ENTRY_FOR*)(s->base + idx + 1), sizeof(FGS_ENTRY_FOR));
                s->stktop_idx = idx + sizeof(FGS_ENTRY_FOR) + 1; /* Pop all scanned entries including the found one */
                return true;
            }
            else
            {
                /* Wrong variable - keep scanning */
            }
            idx += sizeof(FGS_ENTRY_FOR) + 1;
        }
        else
        {
            /* A GOSUB entry */
            return false;
        }
    }
    /* Stack is empty or filled by less than a FOR entry */
    return false;
}
