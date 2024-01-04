/*
 * variable_storage.c
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

#include "variable_storage.h"
#include <string.h>
#include <stddef.h>

#pragma pack(push,1)
typedef struct VARIABLE_ENTRY_
{
    var_name_packed name;
    VARIABLE_VALUE value;
} VARIABLE_ENTRY;

typedef struct ARRAY_VARIABLE_HEADER_
{
    var_name_packed name;
    uint16_t block_size; // Array block size, in bytes, excluding header
} ARRAY_VARIABLE_HEADER;
#pragma pack(pop)


void variable_storage_clear(BASIC_MEM_MGR* s)
{
    s->free_idx = s->array_idx = s->vars_idx;
}

void variable_storage_initialize(BASIC_MEM_MGR* s, unsigned char* base, unsigned size)
{
    /* Initialize an empty variable storage */
    s->base = base;
    s->vars_idx = 0;
    s->max_idx = size;
    s->stktop_idx = size;
    variable_storage_clear(s);
}

static VARIABLE_VALUE* lookup_var(BASIC_MEM_MGR* s, var_name_packed var)
{
    unsigned char* const pb = s->base;
    unsigned idx = s->vars_idx;
    while(idx < s->array_idx)
    {
        var_name_packed v0 = pb[idx] | pb[idx+1] << 8;
        if(var == v0)
        {
            return (VARIABLE_VALUE*)(pb+idx+offsetof(VARIABLE_ENTRY, value));
        }
        idx += sizeof(VARIABLE_ENTRY);
    }
    return 0; /* Not found! */
}

enum BASIC_ERROR_ID variable_storage_create_array_var(BASIC_MEM_MGR* s, var_name_packed var, VARIABLE_VALUE** ppv, unsigned subscript, bool dim)
{
    unsigned char* const pb = s->base;
    unsigned idx = s->array_idx;
    while(idx + sizeof(ARRAY_VARIABLE_HEADER) <= s->free_idx)
    {
        const ARRAY_VARIABLE_HEADER* avh = (const ARRAY_VARIABLE_HEADER*)&pb[idx];
        if(avh->name == var)
        {
            /* Check if we are trying to re-dimension an array */
            if(dim)
            {
                return BASIC_ERROR_REDIMENSION;
            }
            /* Check subscript bounds */
            if(subscript * sizeof(VARIABLE_VALUE) >= avh->block_size)
            {
                return BASIC_ERROR_SUBSCRIPT;
            }
            *ppv = (VARIABLE_VALUE*)(pb+idx+sizeof(ARRAY_VARIABLE_HEADER)) + subscript;
            return BASIC_ERROR_OK;
        }
        idx += sizeof(ARRAY_VARIABLE_HEADER) + avh->block_size;
    }
    /* Array variable not found - create one */
    unsigned new_block_size;
    if(dim)
    {
        new_block_size = subscript;
    }
    else if(subscript > 10)
    {
        /* For default dimensioning by the first array reference, check
         * that the subscript is within the default bounds */
        return BASIC_ERROR_SUBSCRIPT;
    }
    else
    {
        /* Default array size */
        new_block_size = 10;
    }
    /* Add the 0th element and account for element size */
    new_block_size = (new_block_size+1)*sizeof(VARIABLE_VALUE);
    /* Check for OOMEM */
    if(!basic_mem_check_space(s, new_block_size + sizeof(ARRAY_VARIABLE_HEADER)))
    {
        return BASIC_ERROR_OUT_OF_MEMORY;
    }
    /* Initialize the new array variable */
    ARRAY_VARIABLE_HEADER* avh = (ARRAY_VARIABLE_HEADER*)(pb+s->free_idx);
    VARIABLE_VALUE* pae = (VARIABLE_VALUE*)(pb+s->free_idx+sizeof(ARRAY_VARIABLE_HEADER));
    avh->name = var;
    avh->block_size = new_block_size;
    memset(pae, 0, new_block_size); /* Initialize all array elements to zeros */
    s->free_idx += new_block_size+sizeof(ARRAY_VARIABLE_HEADER); /* Mark that it is now allocated */
    *ppv = pae + subscript; /* For DIM, the return value will point to 1 past the last element and will never be used */
    return BASIC_ERROR_OK;
}

VARIABLE_VALUE* variable_storage_create_var(BASIC_MEM_MGR* s, var_name_packed var)
{
    VARIABLE_VALUE* retval = lookup_var(s, var);
    if(retval)
    {
        return retval;
    }
    /* Not found. Allocate a new variable, moving the array block upwards if it is nonempty */
    if(!basic_mem_check_space(s, sizeof(VARIABLE_ENTRY)))
    {
        /* OOMEM - cannot create the variable */
        return 0;
    }
    unsigned char* const pb = s->base;
    if(s->array_idx != s->free_idx)
    {
        memmove(pb+s->array_idx+sizeof(VARIABLE_ENTRY), pb+s->array_idx, s->free_idx-s->array_idx);
    }
    retval = (VARIABLE_VALUE*)(pb+s->array_idx+offsetof(VARIABLE_ENTRY, value));
    /* Store the variable name and initialize its value to zero */
    memcpy(pb+s->array_idx, &var, sizeof(var_name_packed));
    memset(retval, 0, sizeof(VARIABLE_VALUE));
    s->array_idx += sizeof(VARIABLE_ENTRY);
    s->free_idx += sizeof(VARIABLE_ENTRY);
    return retval;
}

float variable_storage_read_var(BASIC_MEM_MGR* s, var_name_packed var)
{
    VARIABLE_VALUE* pval = lookup_var(s, var);
    if(pval)
    {
        return pval->f;
    }
    else
    {
        /* All variables read as zero until initialized otherwise */
        return 0.0f;
    }
}
