/*
 * basic_errors.c
 *
 *  Created on: Nov 18, 2023

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

#include "basic_errors.h"
#include <limits.h>
#include "basic_stdio.h"

#define DEFINE_ERROR_TEXT(ID, TEXT) TEXT,

static const char* basic_error_text_table[] =
{
    BASIC_ERRORS_INSTANTIATE(DEFINE_ERROR_TEXT)
};

void basic_error_print(enum BASIC_ERROR_ID id, unsigned line)
{
    if(id == BASIC_ERROR_OK)
    {
        // Do not print anything if OK
        return;
    }

    if(id >= sizeof(basic_error_text_table)/sizeof(basic_error_text_table[0]))
    {
        id = BASIC_ERROR_INTERNAL;
    }
    const char* msg = basic_error_text_table[id];
    basic_printf("%s", msg);
    if(id != BASIC_ERROR_STOP)
    {
        basic_printf(" error");
    }
    if(line != UINT_MAX)
    {
        basic_printf(" in line %u", line);
    }
    basic_printf("\n");
}
