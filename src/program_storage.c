/*
 * program_storage.c
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

#include "program_storage.h"
#include "keywords.h"
#include <string.h>
#include "basic_stdio.h"

void prog_storage_clear(BASIC_MEM_MGR* prog)
{
    /* Re-initialize the program storage memory, erasing the entire program
     * if there was any */
    unsigned char* pb = prog->base;
    pb[0] = '\0'; /* A sentinel byte needed to correctly begin execution of the first line.
                     it pretends to be the end marker of the previous line */
    pb[1] = '\0'; /* 2-byte sentinel as a program-end marker */
    pb[2] = '\0';
    prog->free_idx = prog->array_idx = prog->vars_idx = 3; /* The sentinels on both sides count */
}

void prog_storage_initialize(BASIC_MEM_MGR* prog, void* base, unsigned max_size)
{
    unsigned char* pb = (unsigned char*)base;
    prog->base = pb;
    prog->max_idx = max_size;
    prog->stktop_idx = max_size;
    prog_storage_clear(prog);
}


FIND_LINE_RESULT prog_storage_find_line(const BASIC_MEM_MGR* prog, unsigned line)
{
    const unsigned char* cpb = prog->base;
    FIND_LINE_RESULT r;
    r.idx = 1; /* Skip over the sentinel at program start */
    r.found = false;

    while(cpb[r.idx] || cpb[r.idx+1])
    {
        unsigned nxt_idx = cpb[r.idx] | cpb[r.idx+1] << 8;
        unsigned line_no = cpb[r.idx+2] | cpb[r.idx+3] << 8;
        if(line_no >= line)
        {
            r.found = line_no == line;
            return r;
        }
        r.idx = nxt_idx;
    }
    return r;
}

const unsigned char* prog_storage_get_line_parse_ptr(const BASIC_MEM_MGR* prog, unsigned line_idx)
{
    /* Back up 1 byte to pretend that we are just finishing execution
     * of the previous line, where a zero line-end marker is found.
     * For the first line of the program, the sentinel zero byte is here. */
    return prog->base + line_idx - 1;
}

const unsigned char* prog_storage_advance_line(const BASIC_MEM_MGR* prog, const unsigned char* parse_ptr, unsigned* pline)
{
    /* parse_ptr is now standing either on the sentinel byte at the start of the program,
     * or at the end of a line. We need to get to the next line if it exists
     */
    if(parse_ptr[1] || parse_ptr[2])
    {
        /* A following line exists. Get its number */
        *pline = parse_ptr[3] | parse_ptr[4] << 8;
        parse_ptr += 5;
    }
    else
    {
        /* Reached the end of the program */
        *pline = -1;
    }
    return parse_ptr;
}

static void rebuild_list(BASIC_MEM_MGR* prog)
{
    unsigned char* cpb = prog->base;

    unsigned idx = 1; /* Skip over the sentinel byte at the beginning */

    while(cpb[idx] || cpb[idx+1])
    {
        /* There is some non-null next pointer. It may be invalid,
         * but it means that the end of program has not yet been reached */

        /* Skip over "next index" and "line number" fields, and scan the line until
         * its null terminator */
        unsigned len = strlen((const char*)cpb+idx+4);
        /* Now we know the line length. Set the next-line pointer in it */
        unsigned nxt_idx = idx + 4 + len + 1;
        cpb[idx] = nxt_idx & 0xff;
        cpb[idx+1] = nxt_idx >> 8;
        idx = nxt_idx;
    }
}

bool prog_storage_store_line(BASIC_MEM_MGR* prog, unsigned line, const char* content)
{
    unsigned char* pb = prog->base;
    FIND_LINE_RESULT fl = prog_storage_find_line(prog, line);
    if(fl.found)
    {
        /* Remove an existing line first */
        unsigned nxt_idx = pb[fl.idx] | pb[fl.idx+1] << 8;
        memmove(pb+fl.idx, pb+nxt_idx, prog->vars_idx-nxt_idx);
        prog->vars_idx -= nxt_idx - fl.idx;
        prog->array_idx -= nxt_idx - fl.idx;
        prog->free_idx -= nxt_idx - fl.idx;
    }
    unsigned len = strlen(content);
    if(len)
    {
        /* Only insert if the new line is nonempty */
        if(!basic_mem_check_space(prog, len + 5))
        {
            /* Out of memory!
             * TODO: do not delete the old line if the new one
             * would not fit into memory */
            return false;
        }

        memmove(pb+fl.idx+len+5, pb+fl.idx, prog->vars_idx-fl.idx);
        pb[fl.idx] = 0xff; /* Put something nonzero here to not confuse for program end marker */
        pb[fl.idx+2] = line & 0xff;
        pb[fl.idx+3] = line >> 8;
        memcpy(pb+fl.idx+4, content, len+1); /* Also copy the null terminator */
        prog->vars_idx += len+5;
        prog->array_idx += len+5;
        prog->free_idx += len+5;
    }
    rebuild_list(prog);
    return true;
}

static void print_tokenized_line(const unsigned char* s)
{
    while(*s)
    {
        unsigned char c = *s;
        if(c >= BASIC_KEYWORD_RANGE_BEGIN && c <= BASIC_KEYWORD_RANGE_END)
        {
            basic_printf("%s", keyword_text_table[c-BASIC_KEYWORD_RANGE_BEGIN]);
        }
        else
        {
            basic_putchar(c);
        }
        s++;
    }
    basic_putchar('\n');
}

void prog_storage_list(const BASIC_MEM_MGR* prog, unsigned first_line)
{
    const unsigned char* const cpb = prog->base;
    unsigned idx;
    {
        FIND_LINE_RESULT fl = prog_storage_find_line(prog, first_line);
        idx = fl.idx;
    }
    unsigned nxt_idx;
    while((nxt_idx = cpb[idx] | cpb[idx+1] << 8))
    {
        unsigned line_num = cpb[idx+2] | cpb[idx+3] << 8;
        basic_printf("%u ", line_num);
        /* Print the line, de-tokenizing tokens */
        print_tokenized_line(cpb+idx+4);
        idx = nxt_idx;
    }

}
