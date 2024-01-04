/*
 * keywords.c
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

#include "keywords.h"
#include <string.h>


#define KEYWORD_TEXT(ID) #ID,
#define KEYWORD_TEXT_ALT(ID, ALTTEXT) ALTTEXT,
#define KEYWORD_TEXT_MARK_NOP(ID, MNAME)

const char* keyword_text_table[] =
{
    KEYWORDS_INSTANTIATE(KEYWORD_TEXT, KEYWORD_TEXT, KEYWORD_TEXT_ALT, KEYWORD_TEXT_MARK_NOP)
    0 /* End marker */
};

/* In-place line tokenizer */
void keywords_tokenize_line(char* s)
{
    char* so = s; /* Begin writing to where we are reading. The line could only get shorter */
    unsigned char c;
    while((c = *s))
    {
        if(c == '\"')
        {
            /* Copy string literals verbatim until a closing quote */
            do
            {
                *so++ = c;
                c = *++s;
                if(c == '\"')
                {
                    *so++ = c;
                    s++;
                    break;
                }
            } while(c);
        }
        else
        {
            /* Try to match a keyword */
            for(unsigned i=0; keyword_text_table[i]; i++)
            {
                if(!strncmp(s, keyword_text_table[i], strlen(keyword_text_table[i])))
                {
                    /* A token has been found! */
                    c = i + BASIC_KEYWORD_RANGE_BEGIN; /* Replace char with a token ID */
                    s += strlen(keyword_text_table[i]) - 1;
                    break;
                }
            }
            *so = c;
            so++;
            s++;
            if(c == BASIC_KEYWORD_REM)
            {
                /* Special handling for REM: do not tokenize the remainder
                 * of the line, copy everything verbatim */
                size_t len = strlen(s);
                memmove(so, s, len);
                so += len;
                break;
            }
        }
    }
    /* Null-terminate the output */
    *so = '\0';
}
