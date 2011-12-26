// stack.c - Stack implementation.

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include "stack.h"

stack *stackCreate(void)
{
    stack *result = (stack *) calloc(1, sizeof(stack));

    if (result)
    {
        result->data = NULL;
        result->size = 0;
        result->sp = 0;
    }

    return result;
}

void stackDestroy(stack *s)
{
    if (NULL != s)
    {
        //assert(0 == s->sp);

        free(s->data);
        free(s);
    }
}

int stackPush(stack *s, void *val)
{
    if (NULL == s)
        return -1;

    if (s->sp == s->size) 
    {
        if (NULL == (s->data = (void **) realloc(s->data, (s->size + 1) * sizeof(void *))))
        {
            return -1;
        }

        s->size++;
    }

    s->data[s->sp++] = val;
    return 0;
}

int stackPop(stack *s, void **val)
{
    if (NULL == s || NULL == val || 0 == s->sp)
        return -1;

    *val = s->data[--s->sp];
    return 0;
}

int stackPeek(stack *s, void **val)
{
    if (NULL == s || NULL == val || 0 == s->sp)
        return -1;

    *val = s->data[s->sp - 1];
    return 0;
}

int stackLookup(stack *s, unsigned i, void **val)
{
    if (NULL == s || NULL == val || 0 == s->sp || i >= s->sp)
        return -1;

    *val = s->data[i - 1];
    return 0;
}

int stackReplaceTop(stack *s, void *val)
{
    if (NULL == s || 0 == s->sp)
        return 0;

    s->data[s->sp - 1] = val;
    return 0;
}

size_t stackSize(const stack *s)
{
    if (NULL == s)
        return 0;

    return s->sp;
}
