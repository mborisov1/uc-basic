/*
 * basic_main.c
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

#include "basic_main.h"
#include "basic_parsing.h"
#include "basic_errors.h"
#include "keywords.h"
#include "program_storage.h"
#include <limits.h>
#include <string.h>
#include "basic_stdio.h"
#include <math.h>

static enum BASIC_ERROR_ID handler_data(BASIC_MAIN_STATE* bs)
{
    /* Skip over to the end of statement or the end of the line, whichever comes first */
    bs->parse_ptr = basic_parsing_skip_to_end_statement(bs->parse_ptr);
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID input_line(BASIC_MAIN_STATE* bs)
{
    char* lptr = basic_fgets_stdin(bs->input_buf, sizeof(bs->input_buf));
    if(!lptr)
    {
        /* I/O error or EOF - a good occasion for stopping the program */
        return BASIC_ERROR_STOP;
    }
    /* Delete the newline character at the end */
    lptr = strrchr(bs->input_buf, '\n');
    if(lptr)
    {
        *lptr = '\0';
    }
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID read_input_common(BASIC_MAIN_STATE* bs, bool read)
{
    const unsigned char* input_ptr;
    if(read)
    {
        input_ptr = bs->data_ptr;
    }
    else
    {
        input_ptr = (const unsigned char*)bs->input_buf;
    }
    bool first_input = true;
    bool first_data = !read;
    do
    {
        /* If the input line is empty, ask for another one */
        while(!*input_ptr || (read && *input_ptr == ':'))
        {
            if(read)
            {
                /* Look up for the next DATA statement */
                if(!*input_ptr)
                {
                    input_ptr = prog_storage_advance_line(&bs->prog, input_ptr, &bs->data_line);
                    if(bs->data_line == UINT_MAX)
                    {
                        return BASIC_ERROR_OUT_OF_DATA;
                    }
                }
                else
                {
                    /* Skip over the statement separator */
                    input_ptr++;
                }
                input_ptr = basic_parsing_skipws(input_ptr);
                /* DATA? */
                if(*input_ptr == BASIC_KEYWORD_DATA)
                {
                    input_ptr++;
                    input_ptr = basic_parsing_skipws(input_ptr);
                    first_data = true;
                    /* Stay within this DATA statement */
                }
                else
                {
                    /* Skip a non-DATA statement */
                    input_ptr = basic_parsing_skip_to_end_statement(input_ptr);
                }
            }
            else
            {
                basic_printf("?? ");
                enum BASIC_ERROR_ID status = input_line(bs);
                if(status != BASIC_ERROR_OK)
                {
                    return status;
                }
                input_ptr = (const unsigned char*)bs->input_buf;
                /* We should set the "first_data" flag to true here,
                 * but don't do it for compatibility with the original BASIC bug */
            }
        }
        if(first_input)
        {
            /* Next time, require commas */
            first_input = false;
        }
        else
        {
            /* Require a comma between variables in the INPUT statement */
            if(*bs->parse_ptr != ',')
            {
                return BASIC_ERROR_SYNTAX;
            }
            bs->parse_ptr++;
            bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
        }
        if(first_data)
        {
            /* Next time, require commas */
            first_data = false;
        }
        else
        {
            /* Also require a comma between expressions in the input line */
            if(*input_ptr != ',')
            {
                bs->error_in_data |= read;
                return BASIC_ERROR_SYNTAX;
            }
            input_ptr++;
        }
        /* Parse the expression */
        float val = 0.0f;
        BASIC_PARSING_RESULT pr = basic_parsing_expression(&input_ptr, &val, &bs->prog);
        if(pr == BASIC_PARSING_NOT_FOUND)
        {
            bs->error_in_data |= read;
            return BASIC_ERROR_SYNTAX;
        }
        else if(pr != BASIC_ERROR_OK)
        {
            bs->error_in_data |= read;
            return pr;
        }
        /* Skip possibly trailing whitespace in the input line */
        input_ptr = basic_parsing_skipws(input_ptr);
        var_name_packed vn;
        VARIABLE_VALUE* pv;
        pr = basic_parsing_variable_ref(&bs->parse_ptr, &vn, &pv, &bs->prog);
        if(pr == BASIC_PARSING_NOT_FOUND)
        {
            return BASIC_ERROR_SYNTAX;
        }
        else if(pr != BASIC_ERROR_OK)
        {
            return pr;
        }
        pv->f = val;
        /* Check if there is anything more to input */
    } while(*bs->parse_ptr && *bs->parse_ptr != ':');

    if(read)
    {
        bs->data_ptr = input_ptr;
    }

    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_input(BASIC_MAIN_STATE* bs)
{
    if(bs->current_line == UINT_MAX)
    {
        return BASIC_ERROR_IN_PROGRAM_ONLY;
    }

    basic_printf("? ");
    {
        enum BASIC_ERROR_ID status = input_line(bs);
        if(status != BASIC_ERROR_OK)
        {
            return status;
        }
    }

    return read_input_common(bs, false);
}

static enum BASIC_ERROR_ID handler_dim(BASIC_MAIN_STATE* bs)
{
    BASIC_PARSING_RESULT pr;
    const unsigned char* p = bs->parse_ptr;
    do
    {
        pr = basic_parsing_variable_dim(&p, &bs->prog);
        if(pr == BASIC_PARSING_NOT_FOUND)
        {
            return BASIC_ERROR_SYNTAX;
        }
        else if(pr != BASIC_ERROR_OK)
        {
            return pr;
        }
        if(*p == ',')
        {
            /* Another array to dimension follows, keep looping */
            p++;
            p = basic_parsing_skipws(p);
        }
        else
        {
            /* No more arrays */
            bs->parse_ptr = p;
            return BASIC_ERROR_OK;
        }
    } while(true);
}

static enum BASIC_ERROR_ID handler_read(BASIC_MAIN_STATE* bs)
{
    return read_input_common(bs, true);
}

static enum BASIC_ERROR_ID let_for_common(BASIC_MAIN_STATE* bs, var_name_packed* pvn, VARIABLE_VALUE** ppv)
{
    /* Parse the variable name and allocate a scalar/array variable if needed */
    VARIABLE_VALUE* pvar;
    BASIC_PARSING_RESULT pr = basic_parsing_variable_ref(&bs->parse_ptr, pvn, &pvar, &bs->prog);
    if(pr != BASIC_ERROR_OK)
    {
        return pr;
    }
    /* Also return pointer to value to our caller (needed for FOR) */
    *ppv = pvar;

    /* Parse the assignment operator */
    bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
    if(*bs->parse_ptr != BASIC_KEYWORD_EQUALS)
    {
        return BASIC_ERROR_SYNTAX;
    }
    bs->parse_ptr++;
    /* Parse the expression */
    float val = 0.0f;
    pr = basic_parsing_expression(&bs->parse_ptr, &val, &bs->prog);
    if(pr == BASIC_PARSING_NOT_FOUND)
    {
        return BASIC_ERROR_SYNTAX;
    }
    else if(pr != BASIC_ERROR_OK)
    {
        return pr;
    }
    /* Set the variable's value */
    pvar->f = val;

    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_let(BASIC_MAIN_STATE* bs)
{
    var_name_packed dummy1;
    VARIABLE_VALUE* dummy2;
    return let_for_common(bs, &dummy1, &dummy2);
}

static enum BASIC_ERROR_ID handler_for(BASIC_MAIN_STATE* bs)
{
    if(bs->current_line == UINT_MAX)
    {
        /* Only allow FOR in program to not attempt to loop to the
         * interactive command buffer that may get overwritten */
        return BASIC_ERROR_IN_PROGRAM_ONLY;
    }

    FGS_ENTRY_FOR fe;
    VARIABLE_VALUE* vval;
    enum BASIC_ERROR_ID status = let_for_common(bs, &fe.vn, &vval);
    if(status != BASIC_ERROR_OK)
    {
        return status;
    }
    /* Check if we already have a FOR loop for the given variable on the stack.
     * If yes, discard the entry and all FOR entries created after it. */
    fgstack_lookup_for(&bs->prog, &fe, fe.vn);

    const unsigned char* p = bs->parse_ptr;
    /* Parse the TO part of the FOR statement */
    p = basic_parsing_skipws(p);
    if(*p != BASIC_KEYWORD_TO)
    {
        return BASIC_ERROR_SYNTAX;
    }
    p++;
    p = basic_parsing_skipws(p);
    BASIC_PARSING_RESULT pr = basic_parsing_expression(&p, &fe.to_val, &bs->prog);
    if(pr == BASIC_PARSING_NOT_FOUND)
    {
        return BASIC_ERROR_SYNTAX;
    }
    else if(pr != BASIC_ERROR_OK)
    {
        return pr;
    }

    /* Parse the optional STEP part. Initialize the default step to 1 */
    fe.step = 1.0f;
    if(*p == BASIC_KEYWORD_STEP)
    {
        p++;
        p = basic_parsing_skipws(p);
        pr = basic_parsing_expression(&p, &fe.step, &bs->prog);
        if(pr == BASIC_PARSING_NOT_FOUND)
        {
            return BASIC_ERROR_SYNTAX;
        }
        else if(pr != BASIC_ERROR_OK)
        {
            return pr;
        }
    }
    /* Complete the FOR entry and push it up the stack */
    bs->parse_ptr = p;
    fe.parse_idx = prog_storage_ptr_to_idx(&bs->prog, p);
    fe.line = bs->current_line;
    if(!fgstack_push_for(&bs->prog, &fe))
    {
        return BASIC_ERROR_OUT_OF_MEMORY;
    }
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_next(BASIC_MAIN_STATE* bs)
{
    /* Parse the variable name */
    var_name_packed vn;
    if(basic_parsing_varname(&bs->parse_ptr, &vn) != BASIC_ERROR_OK)
    {
        return BASIC_ERROR_SYNTAX;
    }
    /* Look up for the corresponding FOR stack entry, breaking inner loops that
     * may have started but not completed in between */
    FGS_ENTRY_FOR fe;
    if(!fgstack_lookup_for(&bs->prog, &fe, vn))
    {
        return BASIC_ERROR_NEXT_WITHOUT_FOR;
    }
    /* Look up the loop variable to increment it */
    VARIABLE_VALUE* pval = variable_storage_create_var(&bs->prog, fe.vn);
    if(!pval)
    {
        /* OOMEM shouldn't really happen here because the variable must be already
         * allocated before it can appear on the FOR stack.
         * Still check and report, there's nothing better we can do here */
        return BASIC_ERROR_OUT_OF_MEMORY;
    }
    if((fe.step > 0 && pval->f < fe.to_val) ||
            (fe.step < 0 && pval->f > fe.to_val))
    {
        /* The loop has not yet completed - push the FOR entry back to the stack */
        if(!fgstack_push_for(&bs->prog, &fe))
        {
            /* Shouldn't normally OOMEM here because we just freed a stack entry.
             * But just in case */
            return BASIC_ERROR_OUT_OF_MEMORY;
        }
        /* Increment the loop variable */
        pval->f += fe.step;
        /* And jump to the point behind FOR */
        bs->current_line = fe.line;
        bs->parse_ptr = prog_storage_idx_to_ptr(&bs->prog, fe.parse_idx);
    }
    else
    {
        /* The FOR loop has completed - no need to do anything else here.
         * The stack entry has already been popped */
    }
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID goto_run_common(BASIC_MAIN_STATE* bs, unsigned line, bool line_must_exist)
{
    FIND_LINE_RESULT fr = prog_storage_find_line(&bs->prog, line);
    if(!fr.found && line_must_exist)
    {
        return BASIC_ERROR_NO_SUCH_LINE;
    }

    /* Set current program line and update the parse pointer.
     * This can trigger program execution from interactive mode! */
    bs->current_line = line;
    bs->parse_ptr = prog_storage_get_line_parse_ptr(&bs->prog, fr.idx);

    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_goto(BASIC_MAIN_STATE* bs)
{
    /* A line number argument is required */
    unsigned line = 0;
    BASIC_PARSING_RESULT pr = basic_parsing_uint16(&bs->parse_ptr, &line);
    if(pr != BASIC_ERROR_OK)
    {
        /* Turn a possible BASIC_PARSING_NOT_FOUND into a syntax error */
        return BASIC_ERROR_SYNTAX;
    }
    return goto_run_common(bs, line, true);
}

static void restore0(BASIC_MAIN_STATE* bs)
{
    /* Reset the DATA pointer (for READ) */
    FIND_LINE_RESULT fr = prog_storage_find_line(&bs->prog, 0);
    bs->data_ptr = prog_storage_get_line_parse_ptr(&bs->prog, fr.idx);
    bs->data_line = 0;
}

static enum BASIC_ERROR_ID handler_run(BASIC_MAIN_STATE* bs)
{
    /* An starting line number may be provided (in which case the line must exist) */
    unsigned line = 0;
    BASIC_PARSING_RESULT pr = basic_parsing_uint16(&bs->parse_ptr, &line);
    if(pr == BASIC_ERROR_SYNTAX)
    {
        return BASIC_ERROR_SYNTAX;
    }
    /* Clear all variables */
    variable_storage_clear(&bs->prog);
    /* Reset the FOR/GOSUB stack */
    fgstack_clear(&bs->prog);
    /* Reset the DATA pointer */
    restore0(bs);

    return goto_run_common(bs, line, pr == BASIC_ERROR_OK);
}

static enum BASIC_ERROR_ID handler_new(BASIC_MAIN_STATE* bs)
{
    /* Verify that no parameters follow */
    unsigned char c = *bs->parse_ptr;
    if(c && c != ':')
    {
        return BASIC_ERROR_SYNTAX;
    }

    /* Erase the program */
    prog_storage_clear(&bs->prog);
    /* Clear all variables */
    variable_storage_clear(&bs->prog);
    /* Reset the FOR/GOSUB stack */
    fgstack_clear(&bs->prog);
    /* Reset the DATA pointer */
    restore0(bs);

    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_rem(BASIC_MAIN_STATE* bs)
{
    /* Skip over to the end of the line */
    bs->parse_ptr += strlen((const char*)bs->parse_ptr);
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_if(BASIC_MAIN_STATE* bs)
{
    const unsigned char* p = bs->parse_ptr;

    /* Parse the left hand side expression */
    float lhs = 0.0f;
    BASIC_PARSING_RESULT pr = basic_parsing_expression(&p, &lhs, &bs->prog);
    if(pr == BASIC_PARSING_NOT_FOUND)
    {
        return BASIC_ERROR_SYNTAX;
    }
    else if(pr != BASIC_ERROR_OK)
    {
        return pr;
    }
    /* Parse the comparison operator that may be any combination of LESS, EQUALS, and GREATER */
    unsigned char op_bitmap = 0;
    unsigned char c;
    while((p = basic_parsing_skipws(p)), (c=*p),
            c>=BASIC_KEYWORD_RANGE_BEGIN_COMPARISON_OPERATORS &&
            c<= BASIC_KEYWORD_RANGE_END_COMPARISON_OPERATORS)
    {
        op_bitmap |= 1 << (c - BASIC_KEYWORD_RANGE_BEGIN_COMPARISON_OPERATORS);
        p++;
    }
    if(!op_bitmap)
    {
        /* If no operator is provided */
        return BASIC_ERROR_SYNTAX;
    }
    /* Parse the right hand side expression */
    float rhs = 0.0f;
    pr = basic_parsing_expression(&p, &rhs, &bs->prog);
    if(pr == BASIC_PARSING_NOT_FOUND)
    {
        return BASIC_ERROR_SYNTAX;
    }
    else if(pr != BASIC_ERROR_OK)
    {
        return pr;
    }

    /* Check syntax for a THEN keyword and skip it */
    p = basic_parsing_skipws(p);
    if(*p != BASIC_KEYWORD_THEN)
    {
        return BASIC_ERROR_SYNTAX;
    }
    p++;

    /* Store the updated parse pointer */
    bs->parse_ptr = p;

    /* Do the actual comparison and put its results into a bitmap */
    unsigned char cmp_bitmap =
            (lhs >  rhs) << KEYWORD_RANGE_OFFSET(COMPARISON_OPERATORS, GREATER) |
            (lhs == rhs) << KEYWORD_RANGE_OFFSET(COMPARISON_OPERATORS, EQUALS) |
            (lhs <  rhs) << KEYWORD_RANGE_OFFSET(COMPARISON_OPERATORS, LESS);

    /* The comparison result if true if at least one of the given relations holds */
    bool cmp_result = (op_bitmap & cmp_bitmap) != 0;

    if(cmp_result)
    {
        /* The condition is evaluated to true. Check if THEN is followed
         * by a line number, in which case a GOTO to that line is performed */
        unsigned line = 0;
        pr = basic_parsing_uint16(&bs->parse_ptr, &line);
        if(pr == BASIC_ERROR_OK)
        {
            return goto_run_common(bs, line, true);
        }
        else
        {
            /* General case. Just allow the execution to continue from here */
            return BASIC_ERROR_OK;
        }
    }
    else
    {
        /* The condition is evaluated to false - skip over until the end of the line */
        return handler_rem(bs);
    }
}

static enum BASIC_ERROR_ID handler_restore(BASIC_MAIN_STATE* bs)
{
    /* TODO: allow RESTORE to have a line number parameter */
    restore0(bs);
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_gosub(BASIC_MAIN_STATE* bs)
{
    if(bs->current_line == UINT_MAX)
    {
        /* Only allow GOSUB in program to not attempt to return to the
         * interactive command buffer that may get overwritten */
        return BASIC_ERROR_IN_PROGRAM_ONLY;
    }
    /* A line number argument is required */
    unsigned line = 0;
    BASIC_PARSING_RESULT pr = basic_parsing_uint16(&bs->parse_ptr, &line);
    if(pr != BASIC_ERROR_OK)
    {
        return BASIC_ERROR_SYNTAX;
    }
    /* Push the current line number and parse pointer onto the GOSUB stack,
     * so that we can return to it later */
    FGS_ENTRY_GOSUB eg =
    {
        .line = bs->current_line,
        .parse_idx = prog_storage_ptr_to_idx(&bs->prog, bs->parse_ptr)
    };
    if(!fgstack_push_gosub(&bs->prog, &eg))
    {
        return BASIC_ERROR_OUT_OF_MEMORY;
    }
    return goto_run_common(bs, line, true);
}

static enum BASIC_ERROR_ID handler_return(BASIC_MAIN_STATE* bs)
{
    /* Verify that no parameters follow */
    unsigned char c = *bs->parse_ptr;
    if(c && c != ':')
    {
        return BASIC_ERROR_SYNTAX;
    }

    FGS_ENTRY_GOSUB ge;
    if(!fgstack_pop_gosub(&bs->prog, &ge))
    {
        return BASIC_ERROR_RETURN_WITHOUT_GOSUB;
    }

    /* Jump to the return location */
    bs->current_line = ge.line;
    bs->parse_ptr = prog_storage_idx_to_ptr(&bs->prog, ge.parse_idx);
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_end(BASIC_MAIN_STATE* bs)
{
    /* Verify that no parameters follow */
    unsigned char c = *bs->parse_ptr;
    if(c && c != ':')
    {
        return BASIC_ERROR_SYNTAX;
    }

    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_stop(BASIC_MAIN_STATE* bs)
{
    /* Verify that no parameters follow */
    unsigned char c = *bs->parse_ptr;
    if(c && c != ':')
    {
        return BASIC_ERROR_SYNTAX;
    }

    return BASIC_ERROR_STOP;
}

static enum BASIC_ERROR_ID handler_print(BASIC_MAIN_STATE* bs)
{
    unsigned char c;
    while((c = *bs->parse_ptr), c && c != ':')
    {
        if(c == '\"')
        {
            /* A literal string */
            bs->parse_ptr++;
            while((c = *bs->parse_ptr), c && c != '\"')
            {
                basic_putchar(c);
                bs->parse_ptr++;
            }
            if(c == '\"')
            {
                /* Optionally skip the closing quotes. Not required before line-end in
                 * the prototype BASIC */
                bs->parse_ptr++;
            }
        }
        else if(c == BASIC_KEYWORD_TAB)
        {
            /* An expression is allowed as a TAB argument */
            unsigned tab = 0;
            BASIC_PARSING_RESULT pr = basic_parsing_arrayindex(&bs->parse_ptr, &tab, &bs->prog);
            if(pr == BASIC_PARSING_NOT_FOUND)
            {
                return BASIC_ERROR_SYNTAX;
            }
            else if(pr != BASIC_ERROR_OK)
            {
                return pr;
            }
            /* Print an ANSI code to set cursor horizontal position
             * and let the terminal take care of tracking its current position */
            basic_printf("\033[%dG", tab+1);
        }
        else if(c == ',')
        {
            /* Print a tab character */
            bs->parse_ptr++;
            basic_putchar('\t');
        }
        else if(c == ';')
        {
            /* Semicolon: skip it and don't print anything,
             * but if a statement ends here, return without printing a new-line */
            bs->parse_ptr++;
            bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
            c = *bs->parse_ptr;
            if(!c || c == ':')
            {
                return BASIC_ERROR_OK;
            }
        }
        else
        {
            /* If nothing else, an expression is expected */
            const unsigned char* p = bs->parse_ptr;
            float val = 0.0f;
            BASIC_PARSING_RESULT pr = basic_parsing_expression(&p, &val, &bs->prog);
            if(pr == BASIC_PARSING_NOT_FOUND)
            {
                return BASIC_ERROR_SYNTAX;
            }
            else if(pr != BASIC_ERROR_OK)
            {
                return pr;
            }
            /* Print the value out and a trailing space, as in the base version */
            basic_printf("%G ", val);
            bs->parse_ptr = p;
        }

        bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
    }
    /* Exit loop via newline */
    basic_printf("\n");
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_list(BASIC_MAIN_STATE* bs)
{
    unsigned line = 0;
    BASIC_PARSING_RESULT pr = basic_parsing_uint16(&bs->parse_ptr, &line);
    if(pr != BASIC_ERROR_OK && pr != BASIC_PARSING_NOT_FOUND)
    {
        return pr;
    }

    prog_storage_list(&bs->prog, line);
    return BASIC_ERROR_OK;
}

static enum BASIC_ERROR_ID handler_clear(BASIC_MAIN_STATE* bs)
{
    /* Verify that no parameters follow */
    unsigned char c = *bs->parse_ptr;
    if(c && c != ':')
    {
        return BASIC_ERROR_SYNTAX;
    }

    /* Clear all variables */
    variable_storage_clear(&bs->prog);
    /* Reset the FOR/GOSUB stack */
    fgstack_clear(&bs->prog);

    return BASIC_ERROR_OK;
}

typedef enum BASIC_ERROR_ID (*command_handler_fcn_t)(BASIC_MAIN_STATE* bs);

static command_handler_fcn_t cmd_handlers[KEYWORD_RANGE_OFFSET(GENERAL, RANGE_END_GENERAL)+1] =
{
    [KEYWORD_RANGE_OFFSET(GENERAL, END)    ] = handler_end,
    [KEYWORD_RANGE_OFFSET(GENERAL, FOR)    ] = handler_for,
    [KEYWORD_RANGE_OFFSET(GENERAL, NEXT)   ] = handler_next,
    [KEYWORD_RANGE_OFFSET(GENERAL, DATA)   ] = handler_data,
    [KEYWORD_RANGE_OFFSET(GENERAL, INPUT)  ] = handler_input,
    [KEYWORD_RANGE_OFFSET(GENERAL, DIM)    ] = handler_dim,
    [KEYWORD_RANGE_OFFSET(GENERAL, READ)   ] = handler_read,
    [KEYWORD_RANGE_OFFSET(GENERAL, LET)    ] = handler_let,
    [KEYWORD_RANGE_OFFSET(GENERAL, GOTO)   ] = handler_goto,
    [KEYWORD_RANGE_OFFSET(GENERAL, RUN)    ] = handler_run,
    [KEYWORD_RANGE_OFFSET(GENERAL, IF)     ] = handler_if,
    [KEYWORD_RANGE_OFFSET(GENERAL, RESTORE)] = handler_restore,
    [KEYWORD_RANGE_OFFSET(GENERAL, GOSUB)  ] = handler_gosub,
    [KEYWORD_RANGE_OFFSET(GENERAL, RETURN) ] = handler_return,
    [KEYWORD_RANGE_OFFSET(GENERAL, REM)    ] = handler_rem,
    [KEYWORD_RANGE_OFFSET(GENERAL, STOP)   ] = handler_stop,
    [KEYWORD_RANGE_OFFSET(GENERAL, PRINT)  ] = handler_print,
    [KEYWORD_RANGE_OFFSET(GENERAL, LIST)   ] = handler_list,
    [KEYWORD_RANGE_OFFSET(GENERAL, CLEAR)  ] = handler_clear,
    [KEYWORD_RANGE_OFFSET(GENERAL, NEW)    ] = handler_new
};

static enum BASIC_ERROR_ID exec_line(BASIC_MAIN_STATE* bs)
{
    /* Whitespace must be already skipped in either direct mode or on line entry */
    unsigned char c;
    /* The outer do-while loop runs over program lines */
    do
    {
        /* The inner while loop runs over statements in a line */
        while((c = *bs->parse_ptr))
        {
            bs->error_in_data = false; /* Error messages are associated with parse line, not DATA line by default */
            if(basic_callback_check_break_key())
            {
                /* Stop if the break key is pressed */
                return BASIC_ERROR_STOP;
            }
            if(c > BASIC_KEYWORD_RANGE_END_GENERAL)
            {
                /* Only general keywords are allowed at the first position */
                return BASIC_ERROR_SYNTAX;
            }
            if(c >= BASIC_KEYWORD_RANGE_BEGIN)
            {
                /* Skip over the keyword and prepare to call its handler */
                bs->parse_ptr++;
            }
            else
            {
                /* Must be a LET statement with the LET keyword omitted */
                c = BASIC_KEYWORD_LET; /* Override the token, but do not advance the parse pointer */
            }

            /* Skip any white space that may precede parameters or end-statement */
            bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
            command_handler_fcn_t handler = cmd_handlers[c - BASIC_KEYWORD_RANGE_BEGIN_GENERAL];
            if(handler)
            {
                enum BASIC_ERROR_ID eid = handler(bs);
                if(eid != BASIC_ERROR_OK)
                {
                    return eid;
                }
                if(c == BASIC_KEYWORD_END || c == BASIC_KEYWORD_NEW)
                {
                    /* These two statements cause a silent program termination */
                    return BASIC_ERROR_OK;
                }
            }
            else
            {
                return BASIC_ERROR_INTERNAL;
            }
            /* Special handling for IF to not require a statement separator
             * in case that the IF condition is true */
            bool if_executed = c == BASIC_KEYWORD_IF;
            /* Move on to the next statement */
            c = *bs->parse_ptr;
            if(c)
            {
                if(!if_executed)
                {
                    if(c != ':')
                    {
                        /* Statement separator is expected here */
                        return BASIC_ERROR_SYNTAX;
                    }
                    bs->parse_ptr++; /* Skip the ':' separator */
                }
                /* Also skip whitespace that may precede the next statement */
                bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
            }
        }

        if(bs->current_line != UINT_MAX)
        {
            /* If we are running the program, advance to the next program line */
            bs->parse_ptr = prog_storage_advance_line(&bs->prog, bs->parse_ptr, &bs->current_line);
        }
    } while(bs->current_line != UINT_MAX);
    return BASIC_ERROR_OK; /* Reached the end of line or program */
}

bool basic_main_process_line(BASIC_MAIN_STATE* bs, char* str)
{
    bs->error_in_data = false; /* Error messages are associated with parse line, not DATA line by default */
    bs->current_line = UINT_MAX; /* Mark that no program is running and we are in interactive mode */
    bs->parse_ptr = (const unsigned char*)str;
    /* Skip over any leading spaces */
    bs->parse_ptr = basic_parsing_skipws(bs->parse_ptr);
    if(!*bs->parse_ptr)
    {
        /* Silently ignore empty input lines */
        return false;
    }
    /* Tokenize the input */
    keywords_tokenize_line((char*)bs->parse_ptr);

    /* See if the input begins with a number */
    unsigned line;
    BASIC_PARSING_RESULT pr = basic_parsing_uint16(&bs->parse_ptr, &line);
    if(pr == BASIC_ERROR_SYNTAX)
    {
        basic_error_print(BASIC_ERROR_SYNTAX, UINT_MAX);
        return true;
    }

    if(pr == BASIC_ERROR_OK)
    {
        /* Drop all variables */
        variable_storage_clear(&bs->prog);
        /* Reset the FOR/GOSUB stack */
        fgstack_clear(&bs->prog);

        /* Add/update a program line */
        if(!prog_storage_store_line(&bs->prog, line, (const char*)bs->parse_ptr))
        {
            basic_error_print(BASIC_ERROR_OUT_OF_MEMORY, UINT_MAX);
            return false;
        }

        /* Reset the DATA pointer */
        restore0(bs);
        return false;
    }
    /* Direct execution */
    enum BASIC_ERROR_ID eid = exec_line(bs);
    unsigned error_line = bs->current_line;
    if(bs->error_in_data)
    {
        error_line = bs->data_line;
    }
    basic_error_print(eid, error_line);
    return true;
}

void basic_main_interactive_prompt(BASIC_MAIN_STATE* bs)
{
    bool print_ok = true;
    while(true)
    {
        if(print_ok)
        {
            basic_printf("OK\n");
        }
        if(input_line(bs) != BASIC_ERROR_OK)
        {
            return;
        }
        print_ok = basic_main_process_line(bs, bs->input_buf);
    }
}

void basic_main_initialize(BASIC_MAIN_STATE* bs, void* prog_base, unsigned prog_size)
{
    prog_storage_initialize(&bs->prog, prog_base, prog_size);
    restore0(bs);
}
