/*
 * basic_parsing.h
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

#include "variable_storage.h"
#include "for_gosub_stack.h"
#include "basic_errors.h"

/* The BASIC_PARSING_RESULT enums are extending the BASIC_ERROR ones */
enum BASIC_PARSING_RESULT_
{
    BASIC_PARSING_NOT_FOUND = BASIC_ERROR_MAX
};

typedef uint_fast8_t BASIC_PARSING_RESULT;

const unsigned char* basic_parsing_skipws(const unsigned char* parse_ptr);

const unsigned char* basic_parsing_skip_to_end_statement(const unsigned char* parse_ptr);

BASIC_PARSING_RESULT basic_parsing_uint16(const unsigned char** parse_ptr, unsigned* out);

BASIC_PARSING_RESULT basic_parsing_float(const unsigned char** parse_ptr, float* out);

BASIC_PARSING_RESULT basic_parsing_varname(const unsigned char** parse_ptr, var_name_packed* out);

BASIC_PARSING_RESULT basic_parsing_variable_ref(const unsigned char** parse_ptr, var_name_packed* pvn, VARIABLE_VALUE** out, BASIC_MEM_MGR* mem);

BASIC_PARSING_RESULT basic_parsing_variable_dim(const unsigned char** parse_ptr, BASIC_MEM_MGR* mem);

BASIC_PARSING_RESULT basic_parsing_variable_val(const unsigned char** parse_ptr, var_name_packed* pvn, float* out, BASIC_MEM_MGR* mem);

BASIC_PARSING_RESULT basic_parsing_arrayindex(const unsigned char** parse_ptr, unsigned* out, BASIC_MEM_MGR* mem);

BASIC_PARSING_RESULT basic_parsing_expression(const unsigned char** parse_ptr, float* out, BASIC_MEM_MGR* mem);
