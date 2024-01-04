/*
 * keywords.h
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

#pragma once

#define KEYWORDS_INSTANTIATE(X, XFIRST, XALT, XMARK) \
    XFIRST(END) \
    XMARK(END, RANGE_BEGIN) \
    XMARK(END, RANGE_BEGIN_GENERAL) \
    X(FOR) \
    X(NEXT) \
    X(DATA) \
    X(INPUT) \
    X(DIM) \
    X(READ) \
    X(LET) \
    X(GOTO) \
    X(RUN) \
    X(IF) \
    X(RESTORE) \
    X(GOSUB) \
    X(RETURN) \
    X(REM) \
    X(STOP) \
    X(PRINT) \
    X(LIST) \
    X(CLEAR) \
    X(NEW) \
    XMARK(NEW, RANGE_END_GENERAL) \
    XALT(TAB, "TAB(") \
    XMARK(TAB, RANGE_BEGIN_SUPPLEMENTARY) \
    X(TO) \
    X(THEN) \
    X(STEP) \
    XMARK(STEP, RANGE_END_SUPPLEMENTARY) \
    XALT(PLUS, "+") \
    XMARK(PLUS, RANGE_BEGIN_OPERATORS) \
    XALT(MINUS, "-") \
    XALT(MULTIPLY, "*") \
    XALT(DIVIDE, "/") \
    XMARK(DIVIDE, RANGE_END_OPERATORS) \
    XALT(GREATER, ">") \
    XMARK(GREATER, RANGE_BEGIN_COMPARISON_OPERATORS) \
    XALT(EQUALS, "=") \
    XALT(LESS, "<") \
    XMARK(LESS, RANGE_END_COMPARISON_OPERATORS) \
    X(SGN) \
    XMARK(SGN, RANGE_BEGIN_FUNCTIONS) \
    X(INT) \
    X(ABS) \
    X(USR) \
    X(SQR) \
    X(RND) \
    X(SIN) \
    XMARK(SIN, RANGE_END_FUNCTIONS) \
    XMARK(SIN, RANGE_END)

#define DEFINE_KEYWORD_ID(ID) BASIC_KEYWORD_##ID,
#define DEFINE_KEYWORD_ID_ALT(ID, ALTTEXT) BASIC_KEYWORD_##ID,
#define DEFINE_KEYWORD_ID_FIRST(ID) BASIC_KEYWORD_##ID = 0x80,
#define DEFINE_KEYWORD_MARK(ID, MNAME) BASIC_KEYWORD_##MNAME = BASIC_KEYWORD_##ID,

enum BASIC_KEYWORD_ID
{
    KEYWORDS_INSTANTIATE(DEFINE_KEYWORD_ID, DEFINE_KEYWORD_ID_FIRST, DEFINE_KEYWORD_ID_ALT, DEFINE_KEYWORD_MARK)
};

#undef DEFINE_KEYWORD_RANGE_BEGIN
#undef DEFINE_KEYWORD_ID_FIRST
#undef DEFINE_KEYWORD_ID_ALT
#undef DEFINE_KEYWORD_ID

#define KEYWORD_RANGE_OFFSET(RANGE, ID) (BASIC_KEYWORD_##ID - BASIC_KEYWORD_RANGE_BEGIN_##RANGE)

/* In-place line tokenizer */
void keywords_tokenize_line(char* s);


extern const char* keyword_text_table[];
