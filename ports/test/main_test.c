/*
 * main.c
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

#include <stdio.h>
#include "keywords.h"
#include "basic_errors.h"
#include "basic_parsing.h"
#include "basic_main.h"
#include "program_storage.h"
#include "variable_storage.h"
#include "basic_stdio.h"
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <math.h>

#include "tau/tau.h"


static void print_tokenized_line(const char* s)
{
    while(*s)
    {
        unsigned char c = *s;
        if(!(c & 0x80))
        {
            putchar(c);
        }
        else
        {
            printf("(%02x)",c);
        }
        s++;
    }
}

static void tok_test(const char* s, const char* stok)
{
    char buf[256];
    strcpy(buf, s);
    keywords_tokenize_line(buf);
    print_tokenized_line(buf);
    CHECK(!strcmp(buf, stok));
    putchar('\n');
}

TEST(Tokenization, tokenize)
{
    // Test tokenization of some lines
    tok_test("PRINT PI", "\220 PI");
    tok_test("INPUT B,C,D", "\204 B,C,D");
    tok_test("FOR I=1 TO 20 STEP 4: PRINT A: NEXT I", "\201 I\2351 \225 20 \227 4: \220 A: \202 I");
    tok_test("FOR I=1 TO 20 STEP 4: REM PRINT A: NEXT I", "\201 I\2351 \225 20 \227 4: \216 PRINT A: NEXT I");
    tok_test("PRINT \"hi\": END", "\220 \"hi\": \200");
    tok_test("PRINT \"ho\"", "\220 \"ho\"");
    tok_test("PRINT \"FOR I=1 TO 20 STEP 5\": NEXT I", "\220 \"FOR I=1 TO 20 STEP 5\": \202 I");
    tok_test("PRINT \"ncc", "\220 \"ncc");
}

static void parse_test_uint16_single(const char* str, BASIC_PARSING_RESULT expect_pr, uint16_t expect_res)
{
    BASIC_PARSING_RESULT pr;
    const unsigned char* astr = (const unsigned char*)str;
    unsigned out = 0;
    pr = basic_parsing_uint16(&astr, &out);
    printf("Parse \"%s\" -> %u,%u,%lu\n", str, pr, out, (const char*)astr-str);
    REQUIRE(pr == expect_pr);
    if(pr == BASIC_ERROR_OK)
    {
        CHECK(out == expect_res);
    }
}

TEST(Parsing, parsing_uint16)
{
    parse_test_uint16_single("", BASIC_PARSING_NOT_FOUND, 0);
    parse_test_uint16_single("z", BASIC_PARSING_NOT_FOUND, 0);
    parse_test_uint16_single("0", BASIC_ERROR_OK, 0);
    parse_test_uint16_single("0z", BASIC_ERROR_OK, 0);
    parse_test_uint16_single("4 ", BASIC_ERROR_OK, 4);
    parse_test_uint16_single("32768k", BASIC_ERROR_OK, 32768);
    parse_test_uint16_single("65535", BASIC_ERROR_OK, 65535);
    parse_test_uint16_single("65536", BASIC_ERROR_SYNTAX, 0);
    parse_test_uint16_single("200000", BASIC_ERROR_SYNTAX, 0);
}

struct MainProcFixture
{
    BASIC_MAIN_STATE bs;
};

struct ExprNoRecurseFixture
{
    BASIC_MEM_MGR vars;
};

static unsigned char psbuf[256];
static unsigned char vs_buf[256];
static char out_buf[1024];
static unsigned outbuf_idx;
static char input_injection_buf[512];
static unsigned input_inj_buf_idx;
static unsigned break_injection_level = UINT_MAX;

bool basic_callback_check_break_key(void)
{
    /* Inject a break key press if the output buffer index exceeds a predefined level */
    return outbuf_idx >= break_injection_level;
}

static void main_proc_test(BASIC_MAIN_STATE* bs, const char* str)
{
    outbuf_idx = 0;
    out_buf[0] = '\0';
    printf("Direct: %s\n", str);
    char buf[256];
    if(strlen(str) >= sizeof(buf))
    {
        printf("Buffer overflow\n");
        return;
    }
    strcpy(buf, str);
    basic_main_process_line(bs, buf);
}

static void main_proc_test_progline(BASIC_MAIN_STATE* bs, const char* str)
{
    main_proc_test(bs, str);
    CHECK(!strncmp(out_buf,
            ""
            , sizeof(out_buf)));
}

int basic_printf(const char *restrict format, ... )
{
    va_list v, v2;
    va_start(v, format);
    va_copy(v2, v);
    int retval = vprintf(format, v);
    va_end(v);

    int sres = vsnprintf(out_buf+outbuf_idx, sizeof(out_buf)-outbuf_idx, format, v2);
    if(sres >= 0)
    {
        outbuf_idx += (unsigned)sres;
    }
    va_end(v2);

    return retval;
}

int basic_putchar( int ch )
{
    if(outbuf_idx < sizeof(out_buf) - 1)
    {
        out_buf[outbuf_idx++] = ch;
        out_buf[outbuf_idx] = '\0';
    }
    return putchar(ch);
}

char *basic_fgets_stdin( char *restrict str, int count)
{
    int i = 0;
    while(i < count-1)
    {
        if(input_inj_buf_idx >= sizeof(input_injection_buf))
        {
            /* Input injection buffer was all used up */
            str[i] = '\0';
            if(i == 0)
            {
                return 0; // Signal EOF
            }
            else
            {
                return str;
            }
        }
        char c = input_injection_buf[input_inj_buf_idx];
        str[i] = c;
        if(!c)
        {
            /* End of string in the injection buffer */
            if(i == 0)
            {
                return 0; // Signal EOF
            }
            else
            {
                return str;
            }
        }
        i++;
        input_inj_buf_idx++;
        if(c == '\n')
        {
            /* Newline */
            return str;
        }
    }
    /* Maximum specified size reached - add a null terminator and return */
    str[i] = '\0';
    return str;
}


TEST_F_SETUP(MainProcFixture)
{
    basic_main_initialize(&tau->bs, psbuf, sizeof(psbuf));
}

TEST_F_TEARDOWN(MainProcFixture)
{

}

TEST_F(MainProcFixture, direct_mode)
{
    main_proc_test(&tau->bs, "");
    CHECK(!strncmp(out_buf,
            ""
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "   ");
    CHECK(!strncmp(out_buf,
            ""
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "Z");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT");
    CHECK(!strncmp(out_buf,
            "\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "   PRINT");
    CHECK(!strncmp(out_buf,
            "\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "TAB(");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "STOP");
    CHECK(!strncmp(out_buf,
            "STOP\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "STOP:STOP");
    CHECK(!strncmp(out_buf,
            "STOP\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "STOP 10:STOP");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "STOP  : STOP");
    CHECK(!strncmp(out_buf,
            "STOP\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, list)
{
    main_proc_test_progline(&tau->bs, " 20 PRINT PI");
    main_proc_test_progline(&tau->bs, "1 0FOR I=1 TO 20 STEP 4: PRINT A: NEXT I");
    main_proc_test_progline(&tau->bs, "  3 0 END");
    main_proc_test_progline(&tau->bs, "20   PRINT E");
    main_proc_test(&tau->bs, "LIST");
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 20 STEP 4: PRINT A: NEXT I\n"
            "20 PRINT E\n"
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST 20");
    CHECK(!strncmp(out_buf,
            "20 PRINT E\n"
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST 25");
    CHECK(!strncmp(out_buf,
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST 30");
    CHECK(!strncmp(out_buf,
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST3 0");
    CHECK(!strncmp(out_buf,
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST 3 0  ");
    CHECK(!strncmp(out_buf,
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "LIST 40");
    main_proc_test_progline(&tau->bs, "LIST 65535");
    main_proc_test(&tau->bs, "LIST 65536");
    CHECK(!strncmp(out_buf,
            "Syntax error\n", sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST:LIST");
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 20 STEP 4: PRINT A: NEXT I\n"
            "20 PRINT E\n"
            "30 END\n"
            "10 FOR I=1 TO 20 STEP 4: PRINT A: NEXT I\n"
            "20 PRINT E\n"
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST :  LIST");
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 20 STEP 4: PRINT A: NEXT I\n"
            "20 PRINT E\n"
            "30 END\n"
            "10 FOR I=1 TO 20 STEP 4: PRINT A: NEXT I\n"
            "20 PRINT E\n"
            "30 END\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "STOP:LIST");
    CHECK(!strncmp(out_buf,
            "STOP\n", sizeof(out_buf)));
    main_proc_test(&tau->bs, "LIST:STOP");
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 20 STEP 4: PRINT A: NEXT I\n"
            "20 PRINT E\n"
            "30 END\n"
            "STOP\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, goto_test)
{
    main_proc_test_progline(&tau->bs, "10 GOTO 50");
    main_proc_test_progline(&tau->bs, "20 FOR I=1 TO 20 STEP 4");
    main_proc_test_progline(&tau->bs, "30 PRINT I");
    main_proc_test_progline(&tau->bs, "40 NEXT I");
    main_proc_test_progline(&tau->bs, "50 LIST");
    main_proc_test_progline(&tau->bs, "60 STOP");

    main_proc_test(&tau->bs, "LIST");
    CHECK(!strncmp(out_buf,
            "10 GOTO 50\n"
            "20 FOR I=1 TO 20 STEP 4\n"
            "30 PRINT I\n"
            "40 NEXT I\n"
            "50 LIST\n"
            "60 STOP\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "GOTO");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "GOTO Y");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "GOTO 5");
    CHECK(!strncmp(out_buf,
            "No such line error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "GOTO 10");
    CHECK(!strncmp(out_buf,
            "10 GOTO 50\n"
            "20 FOR I=1 TO 20 STEP 4\n"
            "30 PRINT I\n"
            "40 NEXT I\n"
            "50 LIST\n"
            "60 STOP\n"
            "STOP in line 60\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, rem)
{
    main_proc_test_progline(&tau->bs, "10 REM Some text GOTO");
    main_proc_test_progline(&tau->bs, "20 LIST: REM Print out the program");
    main_proc_test_progline(&tau->bs, "30 REM: STOP");
    main_proc_test(&tau->bs, "GOTO 10");
    CHECK(!strncmp(out_buf,
            "10 REM Some text GOTO\n"
            "20 LIST: REM Print out the program\n"
            "30 REM: STOP\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, run)
{
    main_proc_test_progline(&tau->bs, "10 GOTO 50");
    main_proc_test_progline(&tau->bs, "20 FOR I=1 TO 20 STEP 4");
    main_proc_test_progline(&tau->bs, "30 PRINT I");
    main_proc_test_progline(&tau->bs, "40 NEXT I");
    main_proc_test_progline(&tau->bs, "50 LIST");
    main_proc_test_progline(&tau->bs, "60 STOP");

    main_proc_test(&tau->bs, "LIST");
    CHECK(!strncmp(out_buf,
            "10 GOTO 50\n"
            "20 FOR I=1 TO 20 STEP 4\n"
            "30 PRINT I\n"
            "40 NEXT I\n"
            "50 LIST\n"
            "60 STOP\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RUN");
    CHECK(!strncmp(out_buf,
            "10 GOTO 50\n"
            "20 FOR I=1 TO 20 STEP 4\n"
            "30 PRINT I\n"
            "40 NEXT I\n"
            "50 LIST\n"
            "60 STOP\n"
            "STOP in line 60\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RUN Y");
    CHECK(!strncmp(out_buf,
            "10 GOTO 50\n"
            "20 FOR I=1 TO 20 STEP 4\n"
            "30 PRINT I\n"
            "40 NEXT I\n"
            "50 LIST\n"
            "60 STOP\n"
            "STOP in line 60\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RUN 5");
    CHECK(!strncmp(out_buf,
            "No such line error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RUN 10");
    CHECK(!strncmp(out_buf,
            "10 GOTO 50\n"
            "20 FOR I=1 TO 20 STEP 4\n"
            "30 PRINT I\n"
            "40 NEXT I\n"
            "50 LIST\n"
            "60 STOP\n"
            "STOP in line 60\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RUN 65536");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));

    main_proc_test_progline(&tau->bs, "50 REM"); // Remove the LIST statement to simplify checks
    main_proc_test(&tau->bs, "RUN: LIST");
    CHECK(!strncmp(out_buf,
            "STOP in line 60\n"
            , sizeof(out_buf)));

}

TEST_F(MainProcFixture, data)
{
    main_proc_test_progline(&tau->bs, "10 DATA 50");
    main_proc_test_progline(&tau->bs, "20 DATA I=1 TO 20 STEP 4");
    main_proc_test_progline(&tau->bs, "30 DATA PRINT I");
    main_proc_test_progline(&tau->bs, "40 DATA NEXT I: STOP");
    main_proc_test_progline(&tau->bs, "60 STOP");

    main_proc_test_progline(&tau->bs, "DATA some weird data");
    main_proc_test(&tau->bs, "DATA other weird data: STOP");
    CHECK(!strncmp(out_buf,
            "STOP\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RUN");
    CHECK(!strncmp(out_buf,
            "STOP in line 40\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, print)
{
    main_proc_test(&tau->bs, "PRINT \"Hello World!\"");
    CHECK(!strncmp(out_buf,
            "Hello World!\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"No closing quote - should not fail");
    CHECK(!strncmp(out_buf,
            "No closing quote - should not fail\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"If you see something,\": PRINT \"Say something,\"");
    CHECK(!strncmp(out_buf,
            "If you see something,\n"
            "Say something,\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"No quote prior to colon : PRINT \"Should fail\"");
    CHECK(!strncmp(out_buf,
            "No quote prior to colon : PRINT 0 0 0 0 0 0 0 0 0 0 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"No newline \";: PRINT \"for this line\"");
    CHECK(!strncmp(out_buf,
            "No newline for this line\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT INT(RND(A)+RND(A)+RND(A))");
    CHECK(!strncmp(out_buf,
            "2 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT A, B, C");
    CHECK(!strncmp(out_buf,
            "0 \t0 \t0 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 0, 1, 2");
    CHECK(!strncmp(out_buf,
            "0 \t1 \t2 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 2+3*4+5");
    CHECK(!strncmp(out_buf,
            "19 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT SIN (3.14159265358/4)");
    CHECK(!strncmp(out_buf,
            "0.707107 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 1.23456789e37");
    CHECK(!strncmp(out_buf,
            "1.23457E+37 \n"
            , sizeof(out_buf)));
    /* The test below assumes certain gray-zone number syntax handling
     * and restricted variable name handling, so it may break for future BASIC improvements */
    main_proc_test(&tau->bs, "PRINT 1.23456789ef37");
    CHECK(!strncmp(out_buf,
            "1.23457 0 7 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT TAB(5)\"HI\"");
    CHECK(!strncmp(out_buf,
            "\033[6GHI\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT TAB(/)\"HI\""); // Shound fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT TAB (5)\"TAB( keyword includes the opening bracket, no space allowed\"");
    CHECK(!strncmp(out_buf,
            "0 0 0 TAB( keyword includes the opening bracket, no space allowed\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT TAB(-1)\"Should fail\"");
    CHECK(!strncmp(out_buf,
            "Parameter error\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, let)
{
    main_proc_test(&tau->bs, "LET");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LET A");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "LET A=");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "A");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "=2");
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "A=2");
    main_proc_test_progline(&tau->bs, "LET B=3");
    main_proc_test_progline(&tau->bs, "LETC=4");
    main_proc_test(&tau->bs, "PRINT A, B, C, A+B");
    CHECK(!strncmp(out_buf,
            "2 \t3 \t4 \t5 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "CLEAR"); // Also test if CLEAR works as desired
    main_proc_test(&tau->bs, "PRINT A, B, C, A+B");
    CHECK(!strncmp(out_buf,
            "0 \t0 \t0 \t0 \n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, input)
{
    BASIC_MAIN_STATE bs;
    basic_main_initialize(&bs, psbuf, sizeof(psbuf));

    strcpy(input_injection_buf, "3");
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "INPUT");
    /* Input is only allowed in program */
    CHECK(!strncmp(out_buf,
            "In program only error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&bs, "10 INPUT");
    main_proc_test(&bs, "RUN"); // An input prompt and a syntax error is expected
    CHECK(!strncmp(out_buf,
            "? Syntax error in line 10\n"
            , sizeof(out_buf)));

    input_inj_buf_idx = 0;
    main_proc_test_progline(&bs, "10 INPUT A");
    main_proc_test_progline(&bs, "20 PRINT A");
    main_proc_test(&bs, "RUN"); // An input prompt and a value 3 is expected
    CHECK(!strncmp(out_buf,
            "? 3 \n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "A");
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN"); // An input prompt and a value 0 is expected
    CHECK(!strncmp(out_buf,
            "? 0 \n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "A(1)");
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN"); // An input prompt and a value 0 is expected
    CHECK(!strncmp(out_buf,
            "? 0 \n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "A(1");
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN"); // An input prompt and a syntax error in line 10 is expected
    CHECK(!strncmp(out_buf,
            "? Syntax error in line 10\n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "2,3");
    input_inj_buf_idx = 0;
    main_proc_test_progline(&bs, "10 INPUT A , B");
    main_proc_test_progline(&bs, "20 PRINT A,B");
    main_proc_test(&bs, "RUN");
    CHECK(!strncmp(out_buf,
            "? 2 \t3 \n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "3\n,4");
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN");
    CHECK(!strncmp(out_buf,
            "? ?? 3 \t4 \n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, " 3 \n,4");
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN");
    CHECK(!strncmp(out_buf,
            "? ?? 3 \t4 \n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "3\n ,4"); // This syntax should not be accepted
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN");
    CHECK(!strncmp(out_buf,
            "? ?? Syntax error in line 10\n"
            , sizeof(out_buf)));

    strcpy(input_injection_buf, "3\n,  4"); // But this syntax should be
    input_inj_buf_idx = 0;
    main_proc_test(&bs, "RUN");
    CHECK(!strncmp(out_buf,
            "? ?? 3 \t4 \n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, read)
{
    main_proc_test(&tau->bs, "READ"); // Should fail with a syntax error
    /* TODO: replace this check with a syntax error */
    CHECK(!strncmp(out_buf,
            "Out of DATA error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "READ A"); // Should fail with an out-of-data error
    CHECK(!strncmp(out_buf,
            "Out of DATA error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 DATA 3");
    main_proc_test_progline(&tau->bs, "READ A");
    main_proc_test(&tau->bs, "PRINT A"); // Should print 3
    CHECK(!strncmp(out_buf,
            "3 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "READ B"); // Should fail with an out-of-data error
    CHECK(!strncmp(out_buf,
            "Out of DATA error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "RESTORE");
    main_proc_test_progline(&tau->bs, "READ B"); // Should read 3 again
    main_proc_test(&tau->bs, "PRINT B");
    CHECK(!strncmp(out_buf,
            "3 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "20 DATA 4");
    main_proc_test_progline(&tau->bs, "READ A"); // Should read 3
    main_proc_test_progline(&tau->bs, "READ B"); // Should read 4
    main_proc_test(&tau->bs, "PRINT A,B");
    CHECK(!strncmp(out_buf,
            "3 \t4 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RESTORE: READ A,B: PRINT A,B");
    CHECK(!strncmp(out_buf,
            "3 \t4 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 DATA 3,5,7");
    main_proc_test(&tau->bs, "RESTORE: READ A: READ B,C: PRINT A,B,C");
    CHECK(!strncmp(out_buf,
            "3 \t5 \t7 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 DATA 3+5  , D+1 , 7");
    main_proc_test_progline(&tau->bs, "20 DATA /0");
    main_proc_test(&tau->bs, "RESTORE: READ A,B: READ C: PRINT A,B,C");
    CHECK(!strncmp(out_buf,
            "8 \t1 \t7 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "READ D"); // Should fail with a syntax error in line 20
    CHECK(!strncmp(out_buf,
            "Syntax error in line 20\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT /D"); // Should fail with a normal syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));

    /* Test for errors in expressions in DATA */
    main_proc_test_progline(&tau->bs, "10 DATA SQR(4), SQR(-1)");
    main_proc_test_progline(&tau->bs, "20");
    main_proc_test(&tau->bs, "READ A: PRINT A");
    CHECK(!strncmp(out_buf,
            "2 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "READ A: PRINT A");
    CHECK(!strncmp(out_buf,
            "Parameter error in line 10\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 DATA 1e30*1e30");
    main_proc_test(&tau->bs, "READ A: PRINT A");
    CHECK(!strncmp(out_buf,
            "Overflow error in line 10\n"
            , sizeof(out_buf)));

    /* Test for reading array elements */
    main_proc_test_progline(&tau->bs, "10 DATA 1,2,3");
    main_proc_test(&tau->bs, "READ A(1), A(2), A(3): PRINT A(1), A(2), A(3)");
    CHECK(!strncmp(out_buf,
            "1 \t2 \t3 \n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, gosub_return)
{
    main_proc_test(&tau->bs, "RETURN"); // Should fail with a RG error
    CHECK(!strncmp(out_buf,
            "RETURN without GOSUB error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RETURN 3"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "GOSUB"); // Should fail
    CHECK(!strncmp(out_buf,
            "In program only error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "GOSUB 10"); // Should fail
    CHECK(!strncmp(out_buf,
            "In program only error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "40 RETURN");
    main_proc_test(&tau->bs, "RUN"); // Should fail with a RG error in line 40
    CHECK(!strncmp(out_buf,
            "RETURN without GOSUB error in line 40\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB");
    main_proc_test_progline(&tau->bs, "20 STOP");
    main_proc_test_progline(&tau->bs, "30 PRINT \"Hi\"");
    main_proc_test(&tau->bs, "RUN"); // Should fail in line 10
    CHECK(!strncmp(out_buf,
            "Syntax error in line 10\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 500");
    main_proc_test(&tau->bs, "RUN"); // Should fail with no such line in line 10
    CHECK(!strncmp(out_buf,
            "No such line error in line 10\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 30");
    main_proc_test(&tau->bs, "RUN"); // Should print and stop in line 20
    CHECK(!strncmp(out_buf,
            "Hi\n"
            "STOP in line 20\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 30 gg");
    main_proc_test(&tau->bs, "RUN"); // Should syntax error in line 10
    CHECK(!strncmp(out_buf,
            "Hi\n"
            "Syntax error in line 10\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 30:PRINT \"Lo\"");
    main_proc_test(&tau->bs, "RUN"); // Should print Hi, Lo and stop in line 20
    CHECK(!strncmp(out_buf,
            "Hi\n"
            "Lo\n"
            "STOP in line 20\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 30     :  PRINT \"Lo\"");
    main_proc_test(&tau->bs, "RUN"); // Should print Hi, Lo and stop in line 20
    CHECK(!strncmp(out_buf,
            "Hi\n"
            "Lo\n"
            "STOP in line 20\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "20");
    main_proc_test(&tau->bs, "RUN"); // Should print Hi, Lo, Hi and RG error in line 40
    CHECK(!strncmp(out_buf,
            "Hi\n"
            "Lo\n"
            "Hi\n"
            "RETURN without GOSUB error in line 40\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 10");
    main_proc_test(&tau->bs, "RUN"); // Should OOMEM
    CHECK(!strncmp(out_buf,
            "Out of memory error in line 10\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, for_next)
{
    main_proc_test(&tau->bs, "FOR"); // Should fail
    CHECK(!strncmp(out_buf,
            "In program only error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "NEXT"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "NEXT I"); // Should fail with a NF error
    CHECK(!strncmp(out_buf,
            "NEXT without FOR error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 FOR I=1 TO 5");
    main_proc_test_progline(&tau->bs, "30 PRINT I");
    main_proc_test_progline(&tau->bs, "50 NEXT I");
    main_proc_test(&tau->bs, "RUN"); // Should print 1-5
    CHECK(!strncmp(out_buf,
            "1 \n"
            "2 \n"
            "3 \n"
            "4 \n"
            "5 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "50");
    main_proc_test(&tau->bs, "RUN"); // Should print 1 and halt with a FOR item on the stack
    CHECK(!strncmp(out_buf,
            "1 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "NEXT I"); // Should print 2 and halt
    CHECK(!strncmp(out_buf,
            "2 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "I = -10");
    main_proc_test(&tau->bs, "NEXT I"); // Should print -9 and halt
    CHECK(!strncmp(out_buf,
            "-9 \n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "I = 4");
    main_proc_test(&tau->bs, "NEXT I: PRINT \"X\""); // Should print 5 and halt
    CHECK(!strncmp(out_buf,
            "5 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "NEXT I: PRINT \"X\""); // Should print "X" and halt
    CHECK(!strncmp(out_buf,
            "X\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "NEXT I: PRINT \"X\""); // Should fail with a NF error
    CHECK(!strncmp(out_buf,
            "NEXT without FOR error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "20 FOR J=1 TO 5");
    main_proc_test_progline(&tau->bs, "30 PRINT \"(\";I;J;\")\";");
    main_proc_test_progline(&tau->bs, "40 NEXT J");
    main_proc_test_progline(&tau->bs, "50 PRINT");
    main_proc_test_progline(&tau->bs, "60 NEXT I");
    main_proc_test(&tau->bs, "LIST: RUN"); // Should print a 5x5 mesh
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 5\n"
            "20 FOR J=1 TO 5\n"
            "30 PRINT \"(\";I;J;\")\";\n"
            "40 NEXT J\n"
            "50 PRINT\n"
            "60 NEXT I\n"
            "(1 1 )(1 2 )(1 3 )(1 4 )(1 5 )\n"
            "(2 1 )(2 2 )(2 3 )(2 4 )(2 5 )\n"
            "(3 1 )(3 2 )(3 3 )(3 4 )(3 5 )\n"
            "(4 1 )(4 2 )(4 3 )(4 4 )(4 5 )\n"
            "(5 1 )(5 2 )(5 3 )(5 4 )(5 5 )\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "15 FOR J=1 TO 100"); // Should have no effect, test how stack items are overwritten
    main_proc_test(&tau->bs, "LIST: RUN"); // Should print a 5x5 mesh
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 5\n"
            "15 FOR J=1 TO 100\n"
            "20 FOR J=1 TO 5\n"
            "30 PRINT \"(\";I;J;\")\";\n"
            "40 NEXT J\n"
            "50 PRINT\n"
            "60 NEXT I\n"
            "(1 1 )(1 2 )(1 3 )(1 4 )(1 5 )\n"
            "(2 1 )(2 2 )(2 3 )(2 4 )(2 5 )\n"
            "(3 1 )(3 2 )(3 3 )(3 4 )(3 5 )\n"
            "(4 1 )(4 2 )(4 3 )(4 4 )(4 5 )\n"
            "(5 1 )(5 2 )(5 3 )(5 4 )(5 5 )\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "40 NEXT I"); // Should break the inner loop and delete stack item for J
    main_proc_test_progline(&tau->bs, "60 NEXT J"); // No more stack item here
    main_proc_test(&tau->bs, "LIST: RUN"); // Should fail with a NF error in 60
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 5\n"
            "15 FOR J=1 TO 100\n"
            "20 FOR J=1 TO 5\n"
            "30 PRINT \"(\";I;J;\")\";\n"
            "40 NEXT I\n"
            "50 PRINT\n"
            "60 NEXT J\n"
            "(1 1 )(2 1 )(3 1 )(4 1 )(5 1 )\n"
            "NEXT without FOR error in line 60\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, for_gosub)
{
    main_proc_test_progline(&tau->bs, "10 FOR I=1 TO 5");
    main_proc_test_progline(&tau->bs, "20 GOSUB 100");
    main_proc_test_progline(&tau->bs, "40 NEXT I"); // The FOR loop is protected by a GOSUB entry
    main_proc_test_progline(&tau->bs, "50 STOP");
    main_proc_test_progline(&tau->bs, "100 PRINT I");
    main_proc_test_progline(&tau->bs, "200 RETURN");
    main_proc_test(&tau->bs, "RUN"); // Should print 1-5
    CHECK(!strncmp(out_buf,
            "1 \n"
            "2 \n"
            "3 \n"
            "4 \n"
            "5 \n"
            "STOP in line 50\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "110 NEXT I"); // The FOR stack entry is now inaccessible due to GOSUB
    main_proc_test(&tau->bs, "RUN"); // Should fail with a NF error in line 110
    CHECK(!strncmp(out_buf,
            "1 \n"
            "NEXT without FOR error in line 110\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "110 FOR J=1 TO 5"); // The inner loop should break by RETURN
    main_proc_test(&tau->bs, "LIST: RUN"); // Should print 1-5
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 5\n"
            "20 GOSUB 100\n"
            "40 NEXT I\n"
            "50 STOP\n"
            "100 PRINT I\n"
            "110 FOR J=1 TO 5\n"
            "200 RETURN\n"
            "1 \n"
            "2 \n"
            "3 \n"
            "4 \n"
            "5 \n"
            "STOP in line 50\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "30 NEXT J"); // Test if the inner loop actually breaks
    main_proc_test(&tau->bs, "LIST: RUN"); // Should fail with a NF error in line 30
    CHECK(!strncmp(out_buf,
            "10 FOR I=1 TO 5\n"
            "20 GOSUB 100\n"
            "30 NEXT J\n"
            "40 NEXT I\n"
            "50 STOP\n"
            "100 PRINT I\n"
            "110 FOR J=1 TO 5\n"
            "200 RETURN\n"
            "1 \n"
            "NEXT without FOR error in line 30\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, if_test)
{
    main_proc_test(&tau->bs, "IF"); // Should fail with a syntax error (lhs expression missing)
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF   "); // Should fail with a syntax error (lhs expression missing)
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF A"); // Should fail with a syntax error (comparison operator missing)
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF A>"); // Should fail with a syntax error (rhs expression missing)
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF A>0"); // Should fail with a syntax error (THEN missing)
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "IF A>0 THEN"); // Should succeed and do nothing (condition is false)
    main_proc_test_progline(&tau->bs, "IF A=0 THEN"); // Should succeed and do nothing (condition is true)
    main_proc_test(&tau->bs, "IF A=0 THEN 10"); // Should fail with a no-such-line error
    CHECK(!strncmp(out_buf,
            "No such line error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 PRINT \"Hi\"");
    main_proc_test(&tau->bs, "IF A=0 THEN 10"); // Should print Hi
    CHECK(!strncmp(out_buf,
            "Hi\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF A=0 THEN GOTO 10"); // Should print Hi
    CHECK(!strncmp(out_buf,
            "Hi\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF A=0 THENGOTO 10"); // Should print Hi
    CHECK(!strncmp(out_buf,
            "Hi\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF A=0 THEN PRINT \"Lo\": GOTO 10"); // Should print Lo and Hi
    CHECK(!strncmp(out_buf,
            "Lo\n"
            "Hi\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF 1>0 THEN PRINT \"1>0\""); // Should print "1>0"
    CHECK(!strncmp(out_buf,
            "1>0\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "IF 0>1 THEN PRINT \"0>1\""); // Should do nothing
    main_proc_test_progline(&tau->bs, "IF 1<0 THEN PRINT \"1<0\""); // Should do nothing
    main_proc_test(&tau->bs, "IF 0<1 THEN PRINT \"0<1\""); // Should print "0<1"
    CHECK(!strncmp(out_buf,
            "0<1\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF 1>=0 THEN PRINT \"1>=0\""); // Should print "1>=0"
    CHECK(!strncmp(out_buf,
            "1>=0\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF 0>=0 THEN PRINT \"0>=0\""); // Should print "0>=0"
    CHECK(!strncmp(out_buf,
            "0>=0\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "IF 0>=1 THEN PRINT \"0>=1\""); // Should do nothing
    main_proc_test_progline(&tau->bs, "IF 1<=0 THEN PRINT \"1<=0\""); // Should do nothing
    main_proc_test(&tau->bs, "IF 0<=1 THEN PRINT \"0<=1\""); // Should print "0<=1"
    CHECK(!strncmp(out_buf,
            "0<=1\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF 0<=0 THEN PRINT \"0<=0\""); // Should print "0<=0"
    CHECK(!strncmp(out_buf,
            "0<=0\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "IF 0<>0 THEN PRINT \"0<>0\""); // Should do nothing
    main_proc_test(&tau->bs, "IF 1<>0 THEN PRINT \"1<>0\""); // Should print "1<>0"
    CHECK(!strncmp(out_buf,
            "1<>0\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "IF 0<>1 THEN PRINT \"0<>1\""); // Should print "0<>1"
    CHECK(!strncmp(out_buf,
            "0<>1\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, arrays)
{
    main_proc_test(&tau->bs, "PRINT A("); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));

    main_proc_test(&tau->bs, "PRINT A(/"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT A()"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));

    main_proc_test(&tau->bs, "PRINT A(1)"); // Should print 0 and allocate an array
    CHECK(!strncmp(out_buf,
            "0 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT A(-1)"); // Should fail with a parameter error
    CHECK(!strncmp(out_buf,
            "Parameter error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "A(1) = 1"); // Should succeed
    main_proc_test_progline(&tau->bs, "A(10) = 10"); // Should succeed
    main_proc_test(&tau->bs, "A(11) = 10"); // Should fail with a bad subscript
    CHECK(!strncmp(out_buf,
            "Subscript error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT A(1)"); // Should print 1
    CHECK(!strncmp(out_buf,
            "1 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT A(10)"); // Should print 10
    CHECK(!strncmp(out_buf,
            "10 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "DIM"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "DIM A"); // Original BASIC allows this with OK, but it should better fail with a syntax error
    main_proc_test(&tau->bs, "DIM A(5)"); // Should fail with a Redimension error
    CHECK(!strncmp(out_buf,
            "Redimension error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "DIM B(5)"); // Should succeed
    main_proc_test_progline(&tau->bs, "B(0)=1"); // Should succeed
    main_proc_test_progline(&tau->bs, "B(5)=1"); // Should succeed
    main_proc_test(&tau->bs, "B(6)=1"); // Should fail with a subscript error
    CHECK(!strncmp(out_buf,
            "Subscript error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "DIM C ( 25 )      ,   D ( 3 )   : PRINT \"Hi\""); // Should succeed
    CHECK(!strncmp(out_buf,
            "Hi\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT D(4)"); // Should fail with a subscript error
    CHECK(!strncmp(out_buf,
            "Subscript error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "DIM E ( 32767 )"); // Should fail with OOMEM
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, end_test)
{
    main_proc_test_progline(&tau->bs, "END"); // Should succeed with OK
    main_proc_test(&tau->bs, "END 3"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"A\": END"); // Should print A and succeed
    CHECK(!strncmp(out_buf,
            "A\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"A\": END: PRINT \"B\""); // Should print A and succeed
    CHECK(!strncmp(out_buf,
            "A\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 30");
    main_proc_test_progline(&tau->bs, "20 END");
    main_proc_test_progline(&tau->bs, "30 PRINT \"Hi\"");
    main_proc_test_progline(&tau->bs, "40 RETURN");
    main_proc_test(&tau->bs, "RUN"); // Should print Hi and end with OK
    CHECK(!strncmp(out_buf,
            "Hi\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, new_test)
{
    main_proc_test_progline(&tau->bs, "NEW"); // Should succeed
    main_proc_test(&tau->bs, "NEW 3"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"A\": NEW"); // Should print A and succeed
    CHECK(!strncmp(out_buf,
            "A\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT \"A\": NEW: PRINT \"B\""); // Should print A and succeed
    CHECK(!strncmp(out_buf,
            "A\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "10 GOSUB 30");
    main_proc_test_progline(&tau->bs, "20 END");
    main_proc_test_progline(&tau->bs, "30 PRINT \"Hi\"");
    main_proc_test_progline(&tau->bs, "40 A=2");
    main_proc_test_progline(&tau->bs, "50 NEW");
    main_proc_test(&tau->bs, "RUN"); // Should print Hi and end with OK
    CHECK(!strncmp(out_buf,
            "Hi\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&tau->bs, "LIST"); // Test that there is no more program
    main_proc_test(&tau->bs, "PRINT A"); // Should print zero - variables have been erased
    CHECK(!strncmp(out_buf,
            "0 \n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "RETURN"); // Should fail with an RG error - GOSUB stack has been purged
    CHECK(!strncmp(out_buf,
            "RETURN without GOSUB error\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, expression_errors)
{
    main_proc_test(&tau->bs, "PRINT 1+"); // Should fail with a syntax error
    CHECK(!strncmp(out_buf,
            "Syntax error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 1+A(-1)"); // Should fail with a parameter error
    CHECK(!strncmp(out_buf,
            "Parameter error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT -A(-1)"); // Should fail with a parameter error
    CHECK(!strncmp(out_buf,
            "Parameter error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 1+3*A(-1)"); // Should fail with a parameter error
    CHECK(!strncmp(out_buf,
            "Parameter error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT B(11)"); // Should fail with a subscript error
    CHECK(!strncmp(out_buf,
            "Subscript error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT SQR(-1)"); // Should fail with a parameter error
    CHECK(!strncmp(out_buf,
            "Parameter error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 1/0"); // Should fail with a Div/0 error
    CHECK(!strncmp(out_buf,
            "Division by 0 error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 1e30*1e30"); // Should fail with an overflow
    CHECK(!strncmp(out_buf,
            "Overflow error\n"
            , sizeof(out_buf)));
    main_proc_test(&tau->bs, "PRINT 1e39"); // Should fail with an overflow
    CHECK(!strncmp(out_buf,
            "Overflow error\n"
            , sizeof(out_buf)));
}

TEST_F(MainProcFixture, break_key)
{
    break_injection_level = 30;
    main_proc_test_progline(&tau->bs, "10 PRINT \"123456789\"");
    main_proc_test_progline(&tau->bs, "20 GOTO 10");
    main_proc_test(&tau->bs, "RUN"); // Should eventually stop by Break key press simulation
    CHECK(!strncmp(out_buf,
            "123456789\n"
            "123456789\n"
            "123456789\n"
            "STOP in line 20\n"
            , sizeof(out_buf)));
}

TEST(ProgramOomem, prog_oomem_min)
{
    BASIC_MAIN_STATE bs;

    /* Initialize program storage to a minimum */
    basic_main_initialize(&bs, psbuf, 3);

    main_proc_test_progline(&bs, "10"); // Should always succeed, line deletion
    main_proc_test(&bs, "10P"); // Should fail with oomem
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
}

TEST(ProgramOomem, prog_oomem_some)
{
    BASIC_MAIN_STATE bs;

    /* Initialize program storage to a bare one-line capacity */
    basic_main_initialize(&bs, psbuf, 9);

    main_proc_test_progline(&bs, "10"); // Should always succeed, line deletion
    main_proc_test_progline(&bs, "10 STOP"); // Should just fit
    main_proc_test(&bs, "20 PRINT"); // Should fail with oomem
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
}

TEST(ProgramOomem, prog_line_replacement)
{
    BASIC_MAIN_STATE bs;

    /* Initialize program storage to a bare one-line capacity */
    basic_main_initialize(&bs, psbuf, 9);

    main_proc_test_progline(&bs, "10 STOP"); // Should just fit
    main_proc_test_progline(&bs, "10 PRINT"); // Line replacement, should succeed
    main_proc_test(&bs, "10 PRINT0"); // Should fail with oomem
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
}

TEST(VarOomem, no_var_mem)
{
    BASIC_MAIN_STATE bs;

    /* Initialize memory size to just enough for one program line and
     * 2 bytes expression evaluation stack */
    basic_main_initialize(&bs, psbuf, 11);

    main_proc_test_progline(&bs, "10 STOP"); // Should succeed
    main_proc_test(&bs, "PRINT A"); // Variables are not allocated when reading
    CHECK(!strncmp(out_buf,
            "0 \n"
            , sizeof(out_buf)));
    main_proc_test(&bs, "A=2"); // Should fail with oomem
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
}

TEST(VarOomem, one_var_mem)
{
    BASIC_MAIN_STATE bs;

    /* Initialize memory just enough for two program lines, one variable and 2 bytes
     * for the expression stack */
    basic_main_initialize(&bs, psbuf, 27);

    main_proc_test_progline(&bs, "10 A=2");
    main_proc_test_progline(&bs, "A=2"); // Should succeed
    main_proc_test_progline(&bs, "B=3"); // Should succeed
    main_proc_test(&bs, "C=4"); // Should fail with oomem
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
    main_proc_test_progline(&bs, "20 B=3");
    main_proc_test(&bs, "RUN"); // Should fail with oomem in line 20
    CHECK(!strncmp(out_buf,
            "Out of memory error in line 20\n"
            , sizeof(out_buf)));
}

TEST(VarOomem, array_alloc_in_expression)
{
    BASIC_MAIN_STATE bs;

    /* Initialize memory just enough to fail while allocating the array */
    basic_main_initialize(&bs, psbuf, 16);

    main_proc_test(&bs, "PRINT A(1)"); // Should fail with oomem
    CHECK(!strncmp(out_buf,
            "Out of memory error\n"
            , sizeof(out_buf)));
}

static void expr_test(const char* str, BASIC_MEM_MGR* mem, BASIC_PARSING_RESULT expect_pr, float expect_res)
{
    printf("Expression: %s\n", str);
    const unsigned char* p = (const unsigned char*)str;
    float result = 0.0f;
    BASIC_PARSING_RESULT oc = basic_parsing_expression(&p, &result, mem);
    switch(oc)
    {
    case BASIC_PARSING_NOT_FOUND:
        printf("Expression not found\n");
        break;
    case BASIC_ERROR_OK:
        printf("Result: %f\n", result);
        break;
    default:
        basic_error_print(oc, UINT_MAX);
        break;
    }
    REQUIRE(oc == expect_pr);
    CHECK(result == expect_res);
}


TEST(Expressions, expressions)
{
    BASIC_MEM_MGR vs;

    prog_storage_initialize(&vs, vs_buf, sizeof(vs_buf));

    // Add some variables
    VARIABLE_VALUE* pf = variable_storage_create_var(&vs,
            var_name_add_char(var_name_empty(), 'A'));
    pf->f = 2.0f;

    pf = variable_storage_create_var(&vs,
            var_name_add_char(var_name_empty(), 'B'));
    pf->f = 3.0f;

    pf = variable_storage_create_var(&vs,
            var_name_add_char(var_name_empty(), 'C'));
    pf->f = 4.0f;

    pf = variable_storage_create_var(&vs,
            var_name_add_char(var_name_empty(), 'D'));
    pf->f = 5.0f;

    expr_test("", &vs, BASIC_ERROR_SYNTAX, 0);
    expr_test(" ", &vs, BASIC_ERROR_SYNTAX, 0);
    expr_test("!", &vs, BASIC_ERROR_SYNTAX, 0);
    expr_test("A", &vs, BASIC_ERROR_OK, 2);
    expr_test("\231A", &vs, BASIC_ERROR_OK, -2); //-A
    expr_test("A\230A", &vs, BASIC_ERROR_OK, 4); // A+A
    expr_test("A\230A\230A", &vs, BASIC_ERROR_OK, 6); // A+A+A
    expr_test("A\230B\232C\230D", &vs, BASIC_ERROR_OK, 19); // A+B*C+D
    expr_test("(A\230B)\232C", &vs, BASIC_ERROR_OK, 20); // (A+B)*C
    expr_test("\237(A)", &vs, BASIC_ERROR_OK, 1); // SGN(A)
    expr_test("\237(\231A)", &vs, BASIC_ERROR_OK, -1); // SGN(-A)
    expr_test("\237(A\232E)", &vs, BASIC_ERROR_OK, 0); // SGN(A*E)
    expr_test("B\233A", &vs, BASIC_ERROR_OK, 1.5f); // B/A
    expr_test("\240(B\233A)", &vs, BASIC_ERROR_OK, 1.0f); // INT(B/A)
    expr_test("\241(A)", &vs, BASIC_ERROR_OK, 2); // ABS(A)
    expr_test("\241(\231A)", &vs, BASIC_ERROR_OK, 2); // ABS(-A)
    expr_test("\242(A)", &vs, BASIC_ERROR_OK, 0); // USR(A)
    expr_test("\243(A)", &vs, BASIC_ERROR_OK, sqrtf(2.0f)); // SQR(A)
    // TODO: add a test for RND expr_test("\244(A)", &vs); // RND(A)
    expr_test("\245(A)", &vs, BASIC_ERROR_OK, sinf(2.0f)); // SIN(A)
}

TEST_F_SETUP(ExprNoRecurseFixture)
{
    prog_storage_initialize(&tau->vars, vs_buf, sizeof(vs_buf));
}

TEST_F_TEARDOWN(ExprNoRecurseFixture)
{

}

static void test_expr_nr(struct ExprNoRecurseFixture* tau, const char* expr, BASIC_PARSING_RESULT expect_pr, float expect_val)
{
    char buf[256];
    REQUIRE(strlen(expr) < sizeof(buf));
    strcpy(buf, expr);
    printf("Expression: %s\n", buf);
    keywords_tokenize_line(buf); /* To convert operators into tokens */
    const unsigned char* p = (const unsigned char*)buf;
    float val = 0.0f;
    BASIC_PARSING_RESULT pr = basic_parsing_expression(&p, &val, &tau->vars);
    CHECK(pr == expect_pr);
    if(pr == BASIC_ERROR_OK)
    {
        printf("Result: %f\n", val);
        CHECK(val == expect_val);
    }
    else
    {
        basic_error_print(pr, UINT_MAX);
    }
}

TEST_F(ExprNoRecurseFixture, NumberLiteral)
{
    test_expr_nr(tau, "1.25", BASIC_ERROR_OK, 1.25f);
    test_expr_nr(tau, "-1.25", BASIC_ERROR_OK, -1.25f);
}

TEST_F(ExprNoRecurseFixture, addition)
{
    test_expr_nr(tau, "1+2", BASIC_ERROR_OK, 3.0f);
}

TEST_F(ExprNoRecurseFixture, multiple_addition)
{
    test_expr_nr(tau, "1+2+3", BASIC_ERROR_OK, 6.0f);
}

TEST_F(ExprNoRecurseFixture, add_and_multiply)
{
    test_expr_nr(tau, "1+2*3", BASIC_ERROR_OK, 7.0f);
    test_expr_nr(tau, "2*3+4", BASIC_ERROR_OK, 10.0f);
    test_expr_nr(tau, "1+2*3*4+5", BASIC_ERROR_OK, 30.0f);
}

TEST_F(ExprNoRecurseFixture, unary_minus)
{
    test_expr_nr(tau, "-1+2*-3--4", BASIC_ERROR_OK, -3.0f);
    test_expr_nr(tau, "-1+2*-3---4", BASIC_ERROR_OK, -11.0f);
}

TEST_F(ExprNoRecurseFixture, parentheses)
{
    test_expr_nr(tau, "(1+2)*3", BASIC_ERROR_OK, 9.0f);
    test_expr_nr(tau, "(1+2)+(3+4)*3", BASIC_ERROR_OK, 24.0f);
    test_expr_nr(tau, "-(3+4*2)", BASIC_ERROR_OK, -11.0f);
    test_expr_nr(tau, "--(-3+4*2)", BASIC_ERROR_OK, 5.0f);
}

TEST_F(ExprNoRecurseFixture, functions)
{
    test_expr_nr(tau, "5-SQR(1+2*(3+4)+1)", BASIC_ERROR_OK, 1.0f);
    test_expr_nr(tau, "-SQR(1+2*(3+4)+1)", BASIC_ERROR_OK, -4.0f);
    test_expr_nr(tau, "5-SQR(1+2*(3+4)+1)*2+1", BASIC_ERROR_OK, -2.0f);
}

TEST_F(ExprNoRecurseFixture, array_subscript)
{
    test_expr_nr(tau, "A(1)", BASIC_ERROR_OK, 0.0f);
}

TEST_F(ExprNoRecurseFixture, errors_in_expressions)
{
    test_expr_nr(tau, "", BASIC_ERROR_SYNTAX, 0.0f);
    test_expr_nr(tau, "(", BASIC_ERROR_SYNTAX, 0.0f);
    test_expr_nr(tau, "+", BASIC_ERROR_SYNTAX, 0.0f);
    test_expr_nr(tau, "()", BASIC_ERROR_SYNTAX, 0.0f);
    test_expr_nr(tau, "A()", BASIC_ERROR_SYNTAX, 0.0f);

    test_expr_nr(tau, "-3*(1/0)", BASIC_ERROR_DIVISION_BY_ZERO, 0.0f);
    test_expr_nr(tau, "1+", BASIC_ERROR_SYNTAX, 0.0f);
    test_expr_nr(tau, "1+A(-1)", BASIC_ERROR_PARAMETER, 0.0f);
    test_expr_nr(tau, "-A(-1)", BASIC_ERROR_PARAMETER, 0.0f);
    test_expr_nr(tau, "1+3*A(-1)", BASIC_ERROR_PARAMETER, 0.0f);
    test_expr_nr(tau, "SQR(-1)", BASIC_ERROR_PARAMETER, 0.0f);
    test_expr_nr(tau, "1e30*1e30", BASIC_ERROR_OVERFLOW, 0.0f);
    test_expr_nr(tau, "A(B(C(1))+11)", BASIC_ERROR_SUBSCRIPT, 0.0f);
}

TEST(ExprNoRecurseOomem, oomem_in_expressions)
{
    struct ExprNoRecurseFixture tau;

    /* Initialize the stack with no free space */
    prog_storage_initialize(&tau.vars, vs_buf, 3);

    /* Try to compute a simple expression */
    test_expr_nr(&tau, "0", BASIC_ERROR_OUT_OF_MEMORY, 0.0f);

    /* Add a minimal free space */
    prog_storage_initialize(&tau.vars, vs_buf, 4);

    test_expr_nr(&tau, "0", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // This one should fail as well

    /* Add just enough space for a simple expression */
    prog_storage_initialize(&tau.vars, vs_buf, 5);

    test_expr_nr(&tau, "1", BASIC_ERROR_OK, 1.0f);
    test_expr_nr(&tau, "1+2", BASIC_ERROR_OK, 3.0f);
    test_expr_nr(&tau, "2*3+4", BASIC_ERROR_OK, 10.0f);
    test_expr_nr(&tau, "2+3*4", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // This one needs more space and should fail

    /* Add almost enough space for the last case */
    prog_storage_initialize(&tau.vars, vs_buf, 7+3);
    test_expr_nr(&tau, "2+3*4", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // Should still fail

    /* Another "almost" - still a byte is missing */
    prog_storage_initialize(&tau.vars, vs_buf, 8+3);
    test_expr_nr(&tau, "2+3*4", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // Should still fail

    /* This should be just enough */
    prog_storage_initialize(&tau.vars, vs_buf, 9+3);
    test_expr_nr(&tau, "2+3*4", BASIC_ERROR_OK, 14.0f);
    test_expr_nr(&tau, "2*(1+3)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // But not enough for parentheses

    /* Add a little more space */
    prog_storage_initialize(&tau.vars, vs_buf, 10+3);
    test_expr_nr(&tau, "2*(1+3)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // Still not enough
    test_expr_nr(&tau, "3+SQR(4)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // For a function call either

    /* This should be just enough */
    prog_storage_initialize(&tau.vars, vs_buf, 11+3);
    test_expr_nr(&tau, "2*(1+3)", BASIC_ERROR_OK, 8.0f);
    test_expr_nr(&tau, "3+SQR(4)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // But not enough for a function call
    test_expr_nr(&tau, "3+A(1)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // Or for an array reference


    /* Add enough space to almost be able to parse the array index */
    prog_storage_initialize(&tau.vars, vs_buf, 12+3);
    test_expr_nr(&tau, "3+SQR(4)", BASIC_ERROR_OK, 5.0f);
    test_expr_nr(&tau, "3+A(1)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // Should still fail

    /* Add enough space to parse the array index but not for allocating the array */
    prog_storage_initialize(&tau.vars, vs_buf, 13+3);
    test_expr_nr(&tau, "3+A(1)", BASIC_ERROR_OUT_OF_MEMORY, 0.0f); // Should still fail

    /* This should be just enough for everything */
    prog_storage_initialize(&tau.vars, vs_buf, 5+48);
    test_expr_nr(&tau, "3+A(1)", BASIC_ERROR_OK, 3.0f); // Now, finally, should succeed
}

static void test_float_parser_exact(const char* s, BASIC_PARSING_RESULT expect_pr, float expect_val)
{
    char buf[256];
    REQUIRE(strlen(s) < sizeof(buf));
    strcpy(buf, s);
    printf("Number: %s\n", buf);
    keywords_tokenize_line(buf); /* To convert operators into tokens */
    const unsigned char* p = (const unsigned char*)buf;
    float val;
    BASIC_PARSING_RESULT pr = basic_parsing_float(&p, &val);
    if(pr == BASIC_ERROR_OK)
    {
        printf("Parsed: %.10e (%A)", val, val);
    }
    else
    {
        basic_error_print(pr, UINT_MAX);
    }
    if(expect_pr == BASIC_ERROR_OK)
    {
        printf(" Expected: %.10e (%A)", expect_val, expect_val);
    }
    printf("\n");
    CHECK(pr == expect_pr);
    CHECK(p-(const unsigned char*)buf == strlen(buf));
    if(pr == BASIC_ERROR_OK)
    {
        CHECK(val == expect_val);
    }
}

TEST(CustomFloatParser, empty)
{
    test_float_parser_exact("", BASIC_ERROR_OK, 0.0f);
}

TEST(CustomFloatParser, just_dot)
{
    test_float_parser_exact(".", BASIC_ERROR_OK, 0.0f);
}

TEST(CustomFloatParser, zero)
{
    test_float_parser_exact("0", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("00", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("0.", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("00.", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact(".0", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact(".00", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("0.0", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("00.00", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("0E", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("00E", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("0E0", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact("0E00", BASIC_ERROR_OK, 0.0f);
    test_float_parser_exact(".0E0", BASIC_ERROR_OK, 0.0f);
}

TEST(CustomFloatParser, one)
{
    test_float_parser_exact("1", BASIC_ERROR_OK, 1.0f);
    test_float_parser_exact("1.", BASIC_ERROR_OK, 1.0f);
    test_float_parser_exact("1.0", BASIC_ERROR_OK, 1.0f);
    test_float_parser_exact("1.e", BASIC_ERROR_OK, 1.0f);
    test_float_parser_exact("1.E0", BASIC_ERROR_OK, 1.0f);
    test_float_parser_exact("1.0E0", BASIC_ERROR_OK, 1.0f);
}

TEST(CustomFloatParser, integer)
{
    test_float_parser_exact("123", BASIC_ERROR_OK, 123.0f);
    test_float_parser_exact("123e1", BASIC_ERROR_OK, 1230.0f);
    test_float_parser_exact("123e+1", BASIC_ERROR_OK, 1230.0f);
}

TEST(CustomFloatParser, fractions)
{
    test_float_parser_exact("12.5", BASIC_ERROR_OK, 12.5f);
    test_float_parser_exact("125e-1", BASIC_ERROR_OK, 12.5f);
    test_float_parser_exact("0.0625", BASIC_ERROR_OK, 0.0625f);
}

TEST(CustomFloatParser, exponents)
{
    test_float_parser_exact("10.5e+14", BASIC_ERROR_OK, 10.5e14f);
    test_float_parser_exact("123.25e+20", BASIC_ERROR_OK, 123.25e+20f);
    test_float_parser_exact("123.25e-4", BASIC_ERROR_OK, 123.25e-4f);
}

TEST(CustomFloatParser, overflow)
{
    test_float_parser_exact("12345e38", BASIC_ERROR_OVERFLOW, 0.0f);
}

TEST(Keywords, print_table)
{
    int i=0;
    for(const char** p = keyword_text_table; *p; p++)
    {
        printf("0x%2X - %s\n", i+128, *p);
        i++;
    }

    CHECK(BASIC_KEYWORD_END == 128);
    CHECK(BASIC_KEYWORD_FOR == 129);
}

TEST(Errors, print_table)
{
    for(unsigned i=0; i<=BASIC_ERROR_MAX; i++)
    {
        basic_error_print(i, 0);
    }
    CHECK(BASIC_ERROR_OK == 0);
    CHECK(BASIC_ERROR_SYNTAX == 2);
}

TAU_MAIN()
