/*
 * basic_errors.h
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
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#pragma once

#define BASIC_ERRORS_INSTANTIATE(X) \
    X(OK,                    "OK") \
    X(NEXT_WITHOUT_FOR,      "NEXT without FOR") \
    X(SYNTAX,                "Syntax") \
    X(RETURN_WITHOUT_GOSUB,  "RETURN without GOSUB") \
    X(OUT_OF_DATA,           "Out of DATA") \
    X(PARAMETER,             "Parameter") \
    X(OVERFLOW,              "Overflow") \
    X(OUT_OF_MEMORY,         "Out of memory") \
    X(NO_SUCH_LINE,          "No such line") \
    X(SUBSCRIPT,             "Subscript") \
    X(REDIMENSION,           "Redimension") \
    X(DIVISION_BY_ZERO,      "Division by 0") \
    X(IN_PROGRAM_ONLY,       "In program only") \
    X(STOP,                  "STOP") \
    X(INTERNAL,              "Internal")

#define DEFINE_ERROR_ID(ID, TEXT) BASIC_ERROR_##ID,

enum BASIC_ERROR_ID
{
    BASIC_ERRORS_INSTANTIATE(DEFINE_ERROR_ID)
    BASIC_ERROR_MAX
};

void basic_error_print(enum BASIC_ERROR_ID id, unsigned line);
