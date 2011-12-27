// eval.c - Set expression evaluation.

#include <stdlib.h>
#include <malloc.h>

#include "dbengine.h"
#include "syncengine.h"
#include "setutils.h"
#include "tokenizer.h"
#include "eval.h"
#include "stack.h"

static int compareOperatorsPriority(tokenType a, tokenType b);
static int operatorIsLeftAssoc(tokenType oper);
static int tokenIsOperator(tokenType tt);
static dbObject *performSetOperation(tokenType oper, set *a, set *b);
static dbObject *executeTopOperator(stack *operands, stack *operators);

dbObject *eval(const sds s, size_t *pos)
{
    dbObject *result = NULL;
    stack *operands = NULL, *operators = NULL, *container = NULL;

    if (NULL == s || 0 == strlen(s) || NULL == pos ||
        *pos >= strlen(s))
    {
        return NULL;
    }

    if (NULL == (operands = stackCreate()))
    {
        return NULL;
    }

    if (NULL == (operators = stackCreate()))
    {
        stackDestroy(operands);
        return NULL;
    }

    if (NULL == (container = stackCreate()))
    {
        stackDestroy(operands);
        stackDestroy(operators);
        return NULL;
    }

    while (1)
    {
        char *tokenPtr = NULL;
        size_t tokenLen = 0;
        tokenType tt = fetchToken(s, pos, &tokenPtr, &tokenLen);

        if (tokenIdentifier == tt)
        {
            sds operandKey = sdsnewlen(tokenPtr, tokenLen);
            const set *operand = NULL;
            valType objId;

            if (NULL == operandKey)
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (NULL == (operand = dbGet(operandKey)))
            {
                sdsfree(operandKey);
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            sdsfree(operandKey);

            if (1 != dbFindSet(operand, &objId, 1))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (0 != stackPush(operands, (void *) dbGetObject(objId, 1)))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }
        }
        else if (tokenVal == tt)
        {
            valType valObjectId;
            dbObject *valObject = valParse(tokenPtr, &valObjectId);

            if (NULL == valObject)
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (0 != stackPush(operands, valObject))
            {
                dbObjectRelease(valObject);
                free(valObject);
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }
        }
        else if (tokenSetStart == tt)
        {
            valType setOperandId = 0;
            dbObject *setOperand = NULL;
            if (NULL == (setOperand = (dbObject *) calloc(1, sizeof(dbObject))))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (NULL == (setOperand->objectPtr.setPtr = setCreate()))
            {
                free(setOperand);
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            setOperand->objectType = objectSet;

            if (0 != stackPush(operands, setOperand))
            {
                stackDestroy(operators);
                dbUnregisterObject(setOperandId);
                dbObjectRelease(setOperand);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (0 != stackPush(container, (void *) stackSize(operands)))
            {
                stackDestroy(operators);
                dbUnregisterObject(setOperandId);
                dbObjectRelease(setOperand);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }
        }
        else if (tokenTupleStart == tt)
        {
            valType tupleOperandId = 0;
            dbObject *tupleOperand = NULL;
            if (NULL == (tupleOperand = (dbObject *) calloc(1, sizeof(dbObject))))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (NULL == (tupleOperand->objectPtr.tuplePtr = listCreate()))
            {
                free(tupleOperand);
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            tupleOperand->objectType = objectTuple;

            if (0 != stackPush(operands, tupleOperand))
            {
                stackDestroy(operators);
                dbUnregisterObject(tupleOperandId);
                dbObjectRelease(tupleOperand);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (0 != stackPush(container, (void *) stackSize(operands)))
            {
                stackDestroy(operators);
                dbUnregisterObject(tupleOperandId);
                dbObjectRelease(tupleOperand);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }
        }
        else if (tokenDelim == tt)
        {
            valType topContainerPosition;
            dbObject *top = NULL, *topContainer = NULL;

            if (0 == stackSize(container))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            stackPeek(container, (void **) &topContainerPosition);

            stackLookup(operands, topContainerPosition, (void **) &topContainer);

            while (stackSize(operands) - topContainerPosition > 1)
            {
                if (NULL == executeTopOperator(operands, operators))
                {
                    stackDestroy(container);
                    return NULL;
                }
            }

            if (stackSize(operands) - 1 != topContainerPosition || 0 == stackSize(operands))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            stackPop(operands, (void **) &top);

            switch (topContainer->objectType)
            {
                case objectSet:
                    setAdd(topContainer->objectPtr.setPtr, top->id);
                    break;

                case objectTuple:
                    listAddNodeTail(topContainer->objectPtr.tuplePtr, (void *) top->id);
                    break;
            }
        }
        else if (tokenTupleEnd == tt ||
                 tokenSetEnd == tt)
        {
            valType topContainerPosition;
            dbObject *top = NULL, *topContainer = NULL;

            if (0 == stackSize(container))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            stackPeek(container, (void **) &topContainerPosition);

            if (stackSize(operands) == topContainerPosition)
            {
                stackPop(container, (void **) &topContainerPosition);
                break;
            }

            stackLookup(operands, topContainerPosition, (void **) &topContainer);

            if ((tokenTupleEnd == tt && objectTuple != topContainer->objectType) ||
                (tokenSetEnd == tt && objectSet != topContainer->objectType))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            while (stackSize(operands) - topContainerPosition > 1)
            {
                if (NULL == executeTopOperator(operands, operators))
                {
                    stackDestroy(container);
                    return NULL;
                }
            }

            if (stackSize(operands) - 1 != topContainerPosition || 0 == stackSize(operands))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            stackPop(operands, (void **) &top);

            switch (topContainer->objectType)
            {
                case objectSet:
                    setAdd(topContainer->objectPtr.setPtr, top->id);
                    break;

                case objectTuple:
                    listAddNodeTail(topContainer->objectPtr.tuplePtr, (void *) top->id);
                    break;
            }

            stackPop(container, (void **) &topContainerPosition);

            {
                valType topContainerId;
                if (0 != dbRegisterObject(&topContainer, &topContainerId))
                {
                    dbObjectRelease(topContainer);
                    stackDestroy(operators);
                    stackDestroy(operands);
                    stackDestroy(container);
                    return NULL;
                }

                stackReplaceTop(operands, topContainer);
            }
        }
        else if (tokenIsOperator(tt))
        {
            while (0 != stackSize(operators))
            {
                tokenType topOperator;

                if (0 != stackPeek(operators, (void **) &topOperator))
                {
                    stackDestroy(operators);
                    stackDestroy(operands);
                    stackDestroy(container);
                    return NULL;
                }

                if (!((operatorIsLeftAssoc(tt) && (-1 == compareOperatorsPriority(tt, topOperator) ||
                                                  0 == compareOperatorsPriority(tt, topOperator))) ||
                      (!operatorIsLeftAssoc(tt) && -1 == compareOperatorsPriority(tt, topOperator))))
                {
                    break;
                }

                // Perform operation.
                if (NULL == executeTopOperator(operands, operators))
                {
                    stackDestroy(container);
                    return NULL;
                }
            }

            if (0 != stackPush(operators, (void *) tt))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }
        }
        else if (tokenLeftBrace == tt)
        {
            dbObject *subResult = eval(s, pos);
            if (NULL == subResult)
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }

            if (0 != stackPush(operands, subResult))
            {
                stackDestroy(operators);
                stackDestroy(operands);
                stackDestroy(container);
                return NULL;
            }
        }
        else if (tokenRightBrace == tt ||
                 tokenEnd == tt)
        {
            break;
        }
        else
        {
            stackDestroy(operators);
            stackDestroy(operands);
            stackDestroy(container);
            return NULL;
        }
    }

    // Unwind stacks.
    while (0 != stackSize(operators))
    {
        // Perform operation.
        if (NULL == executeTopOperator(operands, operators))
        {
            stackDestroy(container);
            return NULL;
        }
    }

    if (1 != stackSize(operands))
    {
        stackDestroy(operators);
        stackDestroy(operands);
        stackDestroy(container);
        return NULL;
    }

    stackPop(operands, (void **) &result);

    stackDestroy(operators);
    stackDestroy(operands);
    stackDestroy(container);
    return result;
}

static dbObject *executeTopOperator(stack *operands, stack *operators)
{
    tokenType topOperator;
    dbObject *op1 = NULL, *op2 = NULL, *subResult = NULL;

    if (0 != stackPop(operators, (void **) &topOperator))
    {
        stackDestroy(operators);
        stackDestroy(operands);
        return NULL;
    }

    if (0 != stackPop(operands, (void **) &op1))
    {
        stackDestroy(operators);
        stackDestroy(operands);
        return NULL;
    }

    if (operatorIsLeftAssoc(topOperator))
        stackPop(operands, (void **) &op2);

    if (NULL == op2)
    {
        op2 = op1;
        op1 = NULL;
    }

    if ((NULL != op1 && objectSet != op1->objectType) ||
        (NULL != op2 && objectSet != op2->objectType))
    {
        stackDestroy(operators);
        stackDestroy(operands);
        return NULL;
    }

    if (NULL == (subResult = performSetOperation(topOperator, op2->objectPtr.setPtr, op1 ? op1->objectPtr.setPtr : NULL)))
    {
        stackDestroy(operators);
        stackDestroy(operands);
        return NULL;
    }

    if (0 != stackPush(operands, (void *) subResult))
    {
        dbUnregisterObject(subResult->id);
        free(subResult);
        stackDestroy(operators);
        stackDestroy(operands);
        return NULL;
    }

    return subResult;
}

// Returns NULL on error.
static dbObject *performSetOperation(tokenType oper, set *a, set *b)
{
    set *result = NULL;
    dbObject *resultObject = NULL;
    valType resultObjectId;

    if (NULL != a)
        lockRead(a);

    if (NULL != b)
        lockRead(b);

    switch (oper)
    {
        case tokenBoolean:
            result = setBoolean(a);
            break;

        case tokenMultiply:
            result = setInter(a, b);
            break;

        case tokenCartProd:
            result = setCartProd(a, b);
            break;

        case tokenPlus:
            result = setUnion(a, b);
            break;

        case tokenMinus:
            result = setDiff(a, b);
            break;

        case tokenSymDiff:
            result = setSymDiff(a, b);
            break;
    }

    if (NULL != a)
        unlockRead(a);

    if (NULL != b)
        unlockRead(b);

    if (NULL == (resultObject = (dbObject *) calloc(1, sizeof(dbObject))))
    {
        setDestroy(result);
        return NULL;
    }

    resultObject->objectType = objectSet;
    resultObject->objectPtr.setPtr = result;

    if (0 != dbRegisterObject(&resultObject, &resultObjectId))
    {
        setDestroy(result);
        free(resultObject);
        return NULL;
    }

    return resultObject;
}

// If token isn't an operator then the result is unpredictable.
// Returns: 1 - a > b, 0 - a = b, -1 - a < b.
static int compareOperatorsPriority(tokenType a, tokenType b)
{
    // Priority: ^ @ *, + - ~
    switch (a)
    {
        case tokenBoolean:
        case tokenMultiply:
        case tokenCartProd:
            if (tokenMultiply == b || tokenCartProd == b || tokenBoolean == b)
            {
                return 0;
            }

            return 1;

        case tokenPlus:
        case tokenMinus:
        case tokenSymDiff:
            if (tokenCartProd == b || tokenMultiply == b || tokenBoolean == b)
            {
                return -1;
            }

            return 0;
    }

    return 0;
}

static int tokenIsOperator(tokenType tt)
{
    return tokenBoolean == tt ||
           tokenMultiply == tt ||
           tokenCartProd == tt ||
           tokenPlus == tt ||
           tokenMinus == tt ||
           tokenSymDiff == tt;
}

// Return -1 on error.
static int operatorIsLeftAssoc(tokenType oper)
{
    switch (oper)
    {
        case tokenMultiply:
        case tokenCartProd:
        case tokenPlus:
        case tokenMinus:
        case tokenSymDiff:
            return 1;

        case tokenBoolean:
            return 0;
    }

    return -1;
}
