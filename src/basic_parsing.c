/*
 * basic_parsing.c
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

#include "basic_parsing.h"
#include "keywords.h"
#include <math.h>
#include <stdlib.h>
#include <fenv.h>

#define IS_DIGIT(c) (c >= '0' && c <= '9')
#define IS_ALPHA(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))

/* This one skips over white spaces */
const unsigned char* basic_parsing_skipws(const unsigned char* parse_ptr)
{
    while(*parse_ptr == ' ')
    {
        parse_ptr++;
    }
    return parse_ptr;
}

/* Skip over to the end of statement or the end of the line, whichever comes first */
const unsigned char* basic_parsing_skip_to_end_statement(const unsigned char* parse_ptr)
{
    /* TODO: handle string literals that may contain a ':' properly!
     * Handle REM statements properly. Use algorithms from the tokenizer */
    unsigned char c;
    while((c=*parse_ptr), c && c!=':')
    {
        parse_ptr++;
    }
    return parse_ptr;
}


BASIC_PARSING_RESULT basic_parsing_uint16(const unsigned char** parse_ptr, unsigned* out)
{
    const unsigned char* p = *parse_ptr;
    unsigned result = 0;
    BASIC_PARSING_RESULT r = BASIC_PARSING_NOT_FOUND;
    unsigned char c;
    while((p=basic_parsing_skipws(p)), (c = *p), IS_DIGIT(c))
    {
        if(result >= 10000)
        {
            /* 16-bit number overflow */
            return BASIC_ERROR_SYNTAX;
        }
        result = result * 10;
        c -= '0';
        if(65535 - c < result)
        {
            /* 16-bit number overflow */
            return BASIC_ERROR_SYNTAX;
        }
        result += c;
        p++;
        r = BASIC_ERROR_OK;
    }
    if(r == BASIC_ERROR_OK)
    {
        *parse_ptr = p;
        *out = result;
    }
    return r;
}

static enum BASIC_ERROR_ID except_to_basic_error(void)
{
    if(fetestexcept(FE_DIVBYZERO))
    {
        return BASIC_ERROR_DIVISION_BY_ZERO;
    }
    if(fetestexcept(FE_INVALID))
    {
        return BASIC_ERROR_PARAMETER;
    }
    if(fetestexcept(FE_OVERFLOW))
    {
        return BASIC_ERROR_OVERFLOW;
    }
    return BASIC_ERROR_OK;
}

/* We need a custom floating-point parsing routine without relying on the
 * standard-library strtof function, for 2 reasons:
 * 1) Whitespace is allowed in numbers
 * 2) The exponent sign (+ or -) is scrambled by the tokenizer
 *
 * The algorithm here is rather inexact, but it is taken from the original BASIC
 * and should therefore at least keep the same performance.
 * An exact algorithm like the one in strtof() would involve
 * multi-precision arithmetic. TODO: do it!
 */
BASIC_PARSING_RESULT basic_parsing_float(const unsigned char** parse_ptr, float* out)
{
    unsigned char c;
    const unsigned char* p = *parse_ptr;

    /* No need to parse the sign because it is handled in the term-parsing */
    float val = 0;
    int decimal_scaling = 0;

    /* Prepare for detecting overflows */
    feclearexcept(FE_ALL_EXCEPT);

    /* Parse the integer part */
    while((c=*p), c >= '0' && c <= '9')
    {
        val = val*10 + (c - '0');
        p++;
        p = basic_parsing_skipws(p);
    }

    /* Parse the fractional part */
    if(c == '.')
    {
        p++;
        p = basic_parsing_skipws(p);

        while((c=*p), c >= '0' && c <= '9')
        {
            val = val*10 + (c - '0');
            decimal_scaling--;
            p++;
            p = basic_parsing_skipws(p);
        }
    }

    /* Parse the exponent part */
    if(c=='e' || c=='E')
    {
        p++;
        p = basic_parsing_skipws(p);
        /* Parse the exponent sign */
        int exponent_sign = 1;
        c = *p;
        if(c == BASIC_KEYWORD_PLUS)
        {
            p++;
            p = basic_parsing_skipws(p);
        }
        else if(c == BASIC_KEYWORD_MINUS)
        {
            exponent_sign = -1;
            p++;
            p = basic_parsing_skipws(p);
        }
        unsigned e = 0;
        BASIC_PARSING_RESULT r = basic_parsing_uint16(&p, &e);
        if(r != BASIC_PARSING_NOT_FOUND && r != BASIC_ERROR_OK)
        {
            return r;
        }
        decimal_scaling += exponent_sign*(int)e;
    }

    val *= powf(10.0f, (float)decimal_scaling);

    BASIC_PARSING_RESULT r = except_to_basic_error();
    if(r == BASIC_ERROR_OK)
    {
        *out = val;
    }
    *parse_ptr = p;
    return r;
}

BASIC_PARSING_RESULT basic_parsing_varname(const unsigned char** parse_ptr, var_name_packed* out)
{
    const unsigned char* p = *parse_ptr;
    unsigned char c = *p;
    if(!IS_ALPHA(c))
        return BASIC_ERROR_SYNTAX;
    var_name_packed vn = var_name_add_char(var_name_empty(), c);
    p++;
    p=basic_parsing_skipws(p);
    c = *p;
    if(IS_DIGIT(c))
    {
        vn = var_name_add_char(vn, c);
        p++;
        p=basic_parsing_skipws(p);
    }
    *parse_ptr = p;
    *out = vn;
    return BASIC_ERROR_OK;
}

static BASIC_PARSING_RESULT get_variable(const unsigned char** parse_ptr, var_name_packed* pvn, void* out, BASIC_MEM_MGR* mem, bool create, bool dim)
{
    const unsigned char* p = *parse_ptr;
    /* Parse the variable name */
    var_name_packed vn;
    BASIC_PARSING_RESULT pr;
    if((pr = basic_parsing_varname(&p, &vn)) != BASIC_ERROR_OK)
    {
        return pr;
    }
    *pvn = vn;
    if(*p == '(')
    {
        /* An array element */
        unsigned subscript = 0;
        pr = basic_parsing_arrayindex(&p, &subscript, mem);
        if(pr == BASIC_PARSING_NOT_FOUND)
        {
            return BASIC_ERROR_SYNTAX;
        }
        else if(pr != BASIC_ERROR_OK)
        {
            return pr;
        }
        /* Get a reference to the array element */
        VARIABLE_VALUE* pval;
        pr = variable_storage_create_array_var(mem, vn, &pval, subscript, dim);
        if(pr != BASIC_ERROR_OK)
        {
            return pr;
        }
        if(create)
        {
            *(VARIABLE_VALUE**)out = pval;
        }
        else
        {
            *(float*)out = pval->f;
        }
    }
    else if(create)
    {
        /* A normal variable, creation mode */
        VARIABLE_VALUE* pval = variable_storage_create_var(mem, vn);
        if(!pval)
        {
            return BASIC_ERROR_OUT_OF_MEMORY;
        }
        *(VARIABLE_VALUE**)out = pval;
    }
    else
    {
        /* A normal variable, read mode */
        float val = variable_storage_read_var(mem, vn);
        *(float*)out = val;
    }
    p = basic_parsing_skipws(p);
    *parse_ptr = p;
    return BASIC_ERROR_OK;
}

BASIC_PARSING_RESULT basic_parsing_variable_ref(const unsigned char** parse_ptr, var_name_packed* pvn, VARIABLE_VALUE** out, BASIC_MEM_MGR* mem)
{
    return get_variable(parse_ptr, pvn, out, mem, true, false);
}

BASIC_PARSING_RESULT basic_parsing_variable_dim(const unsigned char** parse_ptr, BASIC_MEM_MGR* mem)
{
    var_name_packed dummy1;
    VARIABLE_VALUE* dummy2;

    return get_variable(parse_ptr, &dummy1, &dummy2, mem, true, true);
}


BASIC_PARSING_RESULT basic_parsing_variable_val(const unsigned char** parse_ptr, var_name_packed* pvn, float* out, BASIC_MEM_MGR* mem)
{
    return get_variable(parse_ptr, pvn, out, mem, false, false);
}

static float eval_function(float x, unsigned char fn)
{
    switch(fn)
    {
    case BASIC_KEYWORD_SGN:
        if(x > 0.0f)
        {
            return 1.0f;
        }
        else if(x < 0.0f)
        {
            return -1.0f;
        }
        else
        {
            return 0.0f;
        }
    case BASIC_KEYWORD_INT:
        return floor(x);
    case BASIC_KEYWORD_ABS:
        return fabsf(x);
    case BASIC_KEYWORD_USR:
        /* TODO: implement USR */
        return 0.0f;
    case BASIC_KEYWORD_SQR:
        return sqrtf(x);
    case BASIC_KEYWORD_RND:
        return rand() / (RAND_MAX+1.0f);
    case BASIC_KEYWORD_SIN:
        return sinf(x);
    default:
        /* Unknown function */
        /* TODO: report error */
        return 0.0f;
    }
}

static const unsigned operator_precedence_table[KEYWORD_RANGE_OFFSET(OPERATORS, RANGE_END_OPERATORS)+1] =
{
    [KEYWORD_RANGE_OFFSET(OPERATORS, PLUS)]      = 1,
    [KEYWORD_RANGE_OFFSET(OPERATORS, MINUS)]     = 1,
    [KEYWORD_RANGE_OFFSET(OPERATORS, MULTIPLY) ] = 2,
    [KEYWORD_RANGE_OFFSET(OPERATORS, DIVIDE) ]   = 2,
    /* Temporarily disabled relation operators in logic expressions */
#if 0
    [KEYWORD_RANGE_OFFSET(OPERATORS, GREATER)]   = 0,
    [KEYWORD_RANGE_OFFSET(OPERATORS, EQUALS)]    = 0,
    [KEYWORD_RANGE_OFFSET(OPERATORS, LESS)]      = 0
#endif
};

static float apply_operator(float a, float b, unsigned char op)
{
    switch(op)
    {
    case BASIC_KEYWORD_PLUS:
        return a+b;
    case BASIC_KEYWORD_MINUS:
        return a-b;
    case BASIC_KEYWORD_MULTIPLY:
        return a*b;
    case BASIC_KEYWORD_DIVIDE:
        return a/b;
        /* Temporarily disabled relation operators in logic expressions */
#if 0
    case BASIC_KEYWORD_GREATER:
        return a>b;
    case BASIC_KEYWORD_EQUALS:
        return a=b;
    case BASIC_KEYWORD_LESS:
        return a<b;
#endif
    default:
        /* Unknown operator */
        /* TODO: report error */
        return 0.0f;
    }
}


enum PARSE_EXPR_STATE
{
    PARSE_EXPR_STATE_EXPRESSION = 0,
    PARSE_EXPR_STATE_TERM,
    PARSE_EXPR_STATE_SUBEXPR_RET,
    PARSE_EXPR_STATE_FUNCTIONARG_RET,
    PARSE_EXPR_STATE_SUBSCRIPT_RET,
    PARSE_EXPR_STATE_FIRST_OPERATOR,
    PARSE_EXPR_STATE_EXPR_1,
    PARSE_EXPR_STATE_SECOND_OPERATOR,
    PARSE_EXPR_STATE_PRECEDENCE_DOWN,
    PARSE_EXPR_STATE_APPLY_OPERATOR,
    PARSE_EXPR_STATE_EXITING
};

/* We implement the Precedence Climbing method
 * https://en.wikipedia.org/wiki/Operator-precedence_parser
 * with an explicit stack to avoid recursive calls
 */
static BASIC_PARSING_RESULT expression_engine_norecurse(const unsigned char** parse_ptr, float* out, BASIC_MEM_MGR* mem)
{
    float lhs = 0.0f;
    float rhs = 0.0f;
    float val = 0.0f;
    unsigned char c;
    const unsigned char* p = *parse_ptr;
    unsigned char lookahead;
    unsigned char op;
    unsigned char min_precedence = 0;
    bool negate = false;
    uint8_t state;
    BASIC_PARSING_RESULT r = BASIC_PARSING_NOT_FOUND;

    /* Set up an exit stack entry for the parse_expression() procedure */
    state = PARSE_EXPR_STATE_EXITING;
    if(!fgstack_push_expression(mem, &state, sizeof(state)))
    {
        return BASIC_ERROR_OUT_OF_MEMORY;
    }
    /* Make the first call to parse_expression */
    state = PARSE_EXPR_STATE_EXPRESSION;

    while(state != PARSE_EXPR_STATE_EXITING)
    {
        switch(state)
        {
        case PARSE_EXPR_STATE_EXPRESSION:
            /* Reset the current precedence */
            min_precedence = 0;
            /* Set up a return point for the first parse_primary() call in parse_expression() */
            state = PARSE_EXPR_STATE_FIRST_OPERATOR;
            if(!fgstack_push_expression(mem, &state, sizeof(state)))
            {
                return BASIC_ERROR_OUT_OF_MEMORY;
            }
            /* Make the first call to parse_primary() */
            state = PARSE_EXPR_STATE_TERM;
            break;
        case PARSE_EXPR_STATE_TERM:
            negate = false;
            while((p=basic_parsing_skipws(p)), (c = *p))
            {
                if(c == BASIC_KEYWORD_PLUS)
                {
                    /* Unary "+", ignore it */
                }
                else if(c == BASIC_KEYWORD_MINUS)
                {
                    /* Unary "-" - toggle the negation flag */
                    negate = !negate;
                }
                else
                {
                    /* Everyting else - process it outside of the loop */
                    break;
                }
                p++;
            }
            if(IS_ALPHA(c))
            {
                /* Variable */
                var_name_packed vn;
                if((r = basic_parsing_varname(&p, &vn)) != BASIC_ERROR_OK)
                {
                    if(r == BASIC_PARSING_NOT_FOUND)
                    {
                        r = BASIC_ERROR_SYNTAX;
                    }
                    return r;
                }
                if(*p == '(')
                {
                    /* An array element, subscript comes as expression in parentheses. */
                    /* Set up a "recursive" call to
                     * the parse_expression routine. We need to save
                     * lhs, op, min_precedence, negation flag,
                     * and the variable name on the stack */
                    p++;
                    if(!fgstack_check_space(mem, sizeof(vn)+sizeof(negate)+sizeof(min_precedence)+
                            sizeof(op)+sizeof(lhs)+sizeof(state)))
                    {
                        return BASIC_ERROR_OUT_OF_MEMORY;
                    }
                    fgstack_push_expression(mem, &vn, sizeof(vn));
                    fgstack_push_expression_byte_nocheck(mem, negate);
                    fgstack_push_expression_byte_nocheck(mem, min_precedence);
                    fgstack_push_expression_byte_nocheck(mem, op);
                    fgstack_push_expression(mem, &lhs, sizeof(lhs));
                    fgstack_push_expression_byte_nocheck(mem, PARSE_EXPR_STATE_SUBSCRIPT_RET);
                    r = BASIC_PARSING_NOT_FOUND;
                    state = PARSE_EXPR_STATE_EXPRESSION;
                }
                else
                {
                    /* A normal variable, read mode */
                    val = variable_storage_read_var(mem, vn);
                    p = basic_parsing_skipws(p);
                    if(negate)
                    {
                        val = -val;
                    }
                    fgstack_pop_expression(mem, &state, sizeof(state));
                }
            }
            else if(IS_DIGIT(c) || c == '.')
            {
                /* A floating-point number literal */
                r = basic_parsing_float(&p, &val);
                if(r != BASIC_ERROR_OK)
                {
                    return r;
                }
                if(negate)
                {
                    val = -val;
                }
                p = basic_parsing_skipws(p);
                fgstack_pop_expression(mem, &state, sizeof(state));
            }
            else if(c >= BASIC_KEYWORD_RANGE_BEGIN_FUNCTIONS && c <= BASIC_KEYWORD_RANGE_END_FUNCTIONS)
            {
                /* A built-in function */
                unsigned char fn = c;
                p++;
                /* Evaluate bracketed argument */
                p=basic_parsing_skipws(p);
                if(*p != '(')
                {
                    return BASIC_ERROR_SYNTAX;
                }
                p++;
                p=basic_parsing_skipws(p);
                /* Set up a "recursive" call to the parse_expression routine.
                 * We need to save lhs, op, min_precedence, negation flag,
                 * and the function identifier on the stack */
                if(!fgstack_check_space(mem, sizeof(fn)+sizeof(negate)+sizeof(min_precedence)+
                        sizeof(op)+sizeof(lhs)+sizeof(state)))
                {
                    return BASIC_ERROR_OUT_OF_MEMORY;
                }
                fgstack_push_expression_byte_nocheck(mem, fn);
                fgstack_push_expression_byte_nocheck(mem, negate);
                fgstack_push_expression_byte_nocheck(mem, min_precedence);
                fgstack_push_expression_byte_nocheck(mem, op);
                fgstack_push_expression(mem, &lhs, sizeof(lhs));
                fgstack_push_expression_byte_nocheck(mem, PARSE_EXPR_STATE_FUNCTIONARG_RET);
                r = BASIC_PARSING_NOT_FOUND;
                state = PARSE_EXPR_STATE_EXPRESSION;
            }
            else if(c == '(')
            {
                /* Parenthesized subexpression - set up a "recursive" call to
                 * the parse_expression routine. We need to save
                 * lhs, op, min_precedence, and the negation flag on the stack */
                p++;
                if(!fgstack_check_space(mem, sizeof(negate)+sizeof(min_precedence)+
                        sizeof(op)+sizeof(lhs)+sizeof(state)))
                {
                    return BASIC_ERROR_OUT_OF_MEMORY;
                }
                fgstack_push_expression_byte_nocheck(mem, negate);
                fgstack_push_expression_byte_nocheck(mem, min_precedence);
                fgstack_push_expression_byte_nocheck(mem, op);
                fgstack_push_expression(mem, &lhs, sizeof(lhs));
                fgstack_push_expression_byte_nocheck(mem, PARSE_EXPR_STATE_SUBEXPR_RET);
                r = BASIC_PARSING_NOT_FOUND;
                state = PARSE_EXPR_STATE_EXPRESSION;
            }
            else
            {
                /* Nothing else (including end-of-line) remains valid */
                return BASIC_ERROR_SYNTAX;
            }
            break;
        case PARSE_EXPR_STATE_SUBEXPR_RET:
            /* This is the return point from parenthesized sub-expression parsing.
             * Pop our states and check balance of parentheses */
            val = lhs; /* Store the return value of the sub-expression as a value of the term */
            fgstack_pop_expression(mem, &lhs, sizeof(lhs));
            fgstack_pop_expression(mem, &op, sizeof(op));
            fgstack_pop_expression(mem, &min_precedence, sizeof(min_precedence));
            fgstack_pop_expression(mem, &negate, sizeof(negate));
            if(negate)
            {
                val = -val;
            }
            if(*p != ')')
            {
                /* Imbalanced parentheses */
                return BASIC_ERROR_SYNTAX;
            }
            p++;
            p = basic_parsing_skipws(p);
            /* Return to our caller */
            fgstack_pop_expression(mem, &state, sizeof(state));
            break;
        case PARSE_EXPR_STATE_FUNCTIONARG_RET:
        {
            /* This is the return point from parenthesized function argument parsing.
             * Pop our states and check balance of parentheses */
            val = lhs; /* Temporarily store the argument value there */
            fgstack_pop_expression(mem, &lhs, sizeof(lhs));
            fgstack_pop_expression(mem, &op, sizeof(op));
            fgstack_pop_expression(mem, &min_precedence, sizeof(min_precedence));
            fgstack_pop_expression(mem, &negate, sizeof(negate));
            unsigned char fn;
            fgstack_pop_expression(mem, &fn, sizeof(fn));
            if(*p != ')')
            {
                /* Imbalanced parentheses */
                return BASIC_ERROR_SYNTAX;
            }
            p++;
            p = basic_parsing_skipws(p);
            /* Evaluate the actual function */
            feclearexcept(FE_ALL_EXCEPT);
            val = eval_function(val, fn); /* Store the result as the value of the term */
            r = except_to_basic_error();
            if(r != BASIC_ERROR_OK)
            {
                return r;
            }
            if(negate)
            {
                val = -val;
            }
            /* Return to our caller */
            fgstack_pop_expression(mem, &state, sizeof(state));
            break;
        }
        case PARSE_EXPR_STATE_SUBSCRIPT_RET:
        {
            /* This is the return point from parenthesized array subscript parsing.
             * Validate and round the floating-point value of the subscript expression down */
            if(lhs < 0.0f || lhs > 32767.0f)
            {
                return BASIC_ERROR_PARAMETER;
            }
            unsigned subscript = floorf(lhs);
            /* Pop our states and check balance of parentheses */
            fgstack_pop_expression(mem, &lhs, sizeof(lhs));
            fgstack_pop_expression(mem, &op, sizeof(op));
            fgstack_pop_expression(mem, &min_precedence, sizeof(min_precedence));
            fgstack_pop_expression(mem, &negate, sizeof(negate));
            var_name_packed vn;
            fgstack_pop_expression(mem, &vn, sizeof(vn));
            if(*p != ')')
            {
                /* Imbalanced parentheses */
                return BASIC_ERROR_SYNTAX;
            }
            p++;
            p = basic_parsing_skipws(p);
            /* Get a reference to the array element */
            VARIABLE_VALUE* pval;
            r = variable_storage_create_array_var(mem, vn, &pval, subscript, false /* dim */);
            if(r != BASIC_ERROR_OK)
            {
                return r;
            }
            /* Read the value */
            val = pval->f;
            if(negate)
            {
                val = -val;
            }
            /* Return to our caller */
            fgstack_pop_expression(mem, &state, sizeof(state));
            break;
        }
        case PARSE_EXPR_STATE_FIRST_OPERATOR:
            /* This is the return point from the first call to parse_primary()
             * with the term value in val. We tail-call parse_expression_1 here,
             * setting up its lhs argument as val */
            lhs = val;
            state = PARSE_EXPR_STATE_EXPR_1;
            break;
        case PARSE_EXPR_STATE_EXPR_1:
            /* This is the entry point to parse_expression_1 with arguments:
             * lhs and min_precedence */
            lookahead = *p;
            if(lookahead >= BASIC_KEYWORD_RANGE_BEGIN_OPERATORS &&
                    lookahead <= BASIC_KEYWORD_RANGE_END_OPERATORS &&
                    operator_precedence_table[lookahead - BASIC_KEYWORD_RANGE_BEGIN_OPERATORS] >= min_precedence)
            {
                op = lookahead;
                p++;
                p = basic_parsing_skipws(p);
                /* Set up a call to parse_primary() to parse the RHS term */
                state = PARSE_EXPR_STATE_SECOND_OPERATOR;
                if(!fgstack_push_expression(mem, &state, sizeof(state)))
                {
                    return BASIC_ERROR_OUT_OF_MEMORY;
                }
                state = PARSE_EXPR_STATE_TERM;
            }
            else
            {
                /* Either not a valid operator, or precedence is smaller than allowed before returning.
                 * Return from parse_expression_1 with lhs as the return value */
                fgstack_pop_expression(mem, &state, sizeof(state));
            }

            break;
        case PARSE_EXPR_STATE_SECOND_OPERATOR:
            /* After we parsed the second term, store its value in rhs */
            rhs = val;
            /* Peek next operator */
            lookahead = *p;
            if(lookahead >= BASIC_KEYWORD_RANGE_BEGIN_OPERATORS &&
                lookahead <= BASIC_KEYWORD_RANGE_END_OPERATORS &&
                operator_precedence_table[lookahead - BASIC_KEYWORD_RANGE_BEGIN_OPERATORS] >
                operator_precedence_table[op - BASIC_KEYWORD_RANGE_BEGIN_OPERATORS])
            {
                /* Set up a "recursive" call to parse_expression_1(rhs, precedence(op)+1).
                 * We need to push lhs, op, and min_precedence on the stack here */
                if(!fgstack_check_space(mem, sizeof(min_precedence)+
                        sizeof(op)+sizeof(lhs)+sizeof(state)))
                {
                    return BASIC_ERROR_OUT_OF_MEMORY;
                }

                fgstack_push_expression_byte_nocheck(mem, min_precedence);
                fgstack_push_expression_byte_nocheck(mem, op);
                fgstack_push_expression(mem, &lhs, sizeof(lhs));
                fgstack_push_expression_byte_nocheck(mem, PARSE_EXPR_STATE_PRECEDENCE_DOWN);
                lhs = rhs;
                min_precedence = operator_precedence_table[op - BASIC_KEYWORD_RANGE_BEGIN_OPERATORS]+1;
                state = PARSE_EXPR_STATE_EXPR_1;
            }
            else
            {
                /* Jump to applying the operator */
                state = PARSE_EXPR_STATE_APPLY_OPERATOR;
            }
            break;
        case PARSE_EXPR_STATE_PRECEDENCE_DOWN:
            /* This is the return point of parse_expression_1() recursive call.
             * the result of the call is contained in lhs, need to move it to rhs. */
            rhs = lhs;
            /* Pop our state variables from the stack */
            fgstack_pop_expression(mem, &lhs, sizeof(lhs));
            fgstack_pop_expression(mem, &op, sizeof(op));
            fgstack_pop_expression(mem, &min_precedence, sizeof(min_precedence));
            /* And jump to applying the operator */
            state = PARSE_EXPR_STATE_APPLY_OPERATOR;
            break;
        case PARSE_EXPR_STATE_APPLY_OPERATOR:
            /* Apply our currently fetched operator to both operands */
            feclearexcept(FE_ALL_EXCEPT);
            lhs = apply_operator(lhs, rhs, op);
            r = except_to_basic_error();
            if(r != BASIC_ERROR_OK)
            {
                return r;
            }
            /* Loop to the outer loop header */
            state = PARSE_EXPR_STATE_EXPR_1;
            break;
        case PARSE_EXPR_STATE_EXITING:
            /* Do nothing - let the enclosing loop exit by itself */
            break;
        default:
            /* Defensive programming - this should never happen */
            return BASIC_ERROR_INTERNAL;
        }
    }

    *out = lhs;
    *parse_ptr = p;
    return r;
}

BASIC_PARSING_RESULT basic_parsing_expression(const unsigned char** parse_ptr, float* out, BASIC_MEM_MGR* mem)
{
    basic_mem_idx_t save_stack_idx = fgstack_get_top(mem);
    BASIC_PARSING_RESULT r = expression_engine_norecurse(parse_ptr, out, mem);
    fgstack_set_top(mem, save_stack_idx);
    return r;
}

BASIC_PARSING_RESULT basic_parsing_arrayindex(const unsigned char** parse_ptr, unsigned* out, BASIC_MEM_MGR* mem)
{
    const unsigned char* p = *parse_ptr;
    p++; /* Skip over the "TAB(" keyword or array index opening brace */
    float val = 0.0f;
    BASIC_PARSING_RESULT r = basic_parsing_expression(&p, &val, mem);
    if(r != BASIC_ERROR_OK)
    {
        return r;
    }
    if(val < 0.0f || val > 32767.0f)
    {
        return BASIC_ERROR_PARAMETER;
    }
    /* Check for a closing bracket */
    p = basic_parsing_skipws(p);
    if(*p != ')')
    {
        return BASIC_ERROR_SYNTAX;
    }
    p++;
    *parse_ptr = p;
    *out = floorf(val);
    return BASIC_ERROR_OK;
}
