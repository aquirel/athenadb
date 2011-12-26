// tokenizer.h - Fetches tokens from input stream.

#include <stdlib.h>
#include <ctype.h>

#include "sds.h"

#include "athena.h"
#include "tokenizer.h"

tokenType fetchToken(const sds s, size_t *pos, char **tokenPtr, size_t *tokenLen)
{
    if (NULL == s || 0 == strlen(s) || NULL == pos || NULL == tokenPtr || NULL == tokenLen)
        return tokenError;

    if (*pos >= strlen(s))
    {
        *tokenLen = 0;
        return tokenEnd;
    }

    while (*pos < strlen(s) && isspace(s[*pos]))
        (*pos)++;

    if (*pos >= strlen(s))
    {
        *tokenLen = 0;
        return tokenEnd;
    }

    switch (s[*pos])
    {
        case '{':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenSetStart;

        case '[':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenTupleStart;

        case '}':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenSetEnd;

        case ']':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenTupleEnd;

        case '(':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenLeftBrace;

        case ')':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenRightBrace;

        case ',':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenDelim;

        case '+':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenPlus;

        case '-':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenMinus;

        case '*':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenMultiply;

        case '~':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenSymDiff;

        case '@':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenCartProd;

        case '^':
            *tokenPtr = &(s[*pos]);
            *tokenLen = 1;
            (*pos)++;
            return tokenBoolean;

        default:
            if (isdigit(s[*pos]))
            {
                *tokenPtr = &(s[*pos]);
                *tokenLen = 0;

                while (*pos < strlen(s) && isdigit(s[*pos]))
                {
                    (*pos)++;
                    (*tokenLen)++;
                }

                return tokenVal;
            }
            else if (isalpha(s[*pos]))
            {
                *tokenPtr = &(s[*pos]);
                *tokenLen = 0;

                while (*pos < strlen(s) && (isalpha(s[*pos]) || isdigit(s[*pos])))
                {
                    (*pos)++;
                    (*tokenLen)++;
                }

                return tokenIdentifier;
            }

            return tokenError;
    }
}
