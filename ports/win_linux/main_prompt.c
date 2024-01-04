/*
 * main_prompt.c
 *
 *  Created on: Jan 3, 2024

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

#include <stdio.h>
#include <stdarg.h>
#include "basic_main.h"
#include "basic_stdio.h"

static unsigned char basic_mem[4096];

int basic_printf(const char *restrict format, ... )
{
    va_list v;
    va_start(v, format);
    int retval = vprintf(format, v);
    va_end(v);

    return retval;
}

int basic_putchar( int ch )
{
    return putchar(ch);
}

char *basic_fgets_stdin( char *restrict str, int count)
{
    return fgets(str, count, stdin);
}

bool basic_callback_check_break_key(void)
{
    /* There is no break key support in this port.
     * Just press Ctrl-C to let the OS kill the program */
    return false;
}

int main(int argc, char* argv[])
{
    BASIC_MAIN_STATE bs;
    basic_main_initialize(&bs, basic_mem, sizeof(basic_mem));
    basic_main_interactive_prompt(&bs);
}
