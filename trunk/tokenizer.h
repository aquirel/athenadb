// tokenizer.h - Fetches tokens from input stream.

#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

typedef enum tokenType
{
    tokenSetStart, tokenSetEnd, tokenTupleStart, tokenTupleEnd, tokenVal, tokenError, tokenEnd,
    tokenLeftBrace, tokenRightBrace, tokenDelim,
    tokenPlus, tokenMinus, tokenMultiply, tokenSymDiff, tokenCartProd, tokenBoolean, tokenIdentifier
} tokenType;

// Fetches next token from s and advances pos to next one.
// Returns tokenType or error and sets tokenPtr to fetched token.
tokenType fetchToken(const sds s, size_t *pos, char **tokenPtr, size_t *tokenLen);

#endif /* __TOKENIZER_H__ */
