/*
 * basic_main.h
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
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#pragma once

#include "program_storage.h"
#include "variable_storage.h"
#include "for_gosub_stack.h"

typedef struct BASIC_MAIN_STATE_
{
    const unsigned char* parse_ptr;
    const unsigned char* data_ptr;
    BASIC_MEM_MGR prog;
    unsigned current_line;
    unsigned data_line;
    bool error_in_data;
    char input_buf[80];
} BASIC_MAIN_STATE;

/* Initialization of the interpreter state */
void basic_main_initialize(BASIC_MAIN_STATE* bs, void* prog_base, unsigned prog_size);

/* Process a line as if typed in the interactive prompt
 * (executes a command, stores or modifies a program line) */
bool basic_main_process_line(BASIC_MAIN_STATE* bs, char* str);

/* Run an interactive command prompt loop
 * (the user may type program lines or commands for immediate execution) */
void basic_main_interactive_prompt(BASIC_MAIN_STATE* bs);

/*-------- Callbacks ---------*/

bool basic_callback_check_break_key(void);
