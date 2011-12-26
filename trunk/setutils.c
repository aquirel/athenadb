// setutils.c - Set utility functions.

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "set.h"
#include "tuple.h"
#include "dbengine.h"
#include "dbobject.h"
#include "eval.h"

dbObject *setParse(const sds s, size_t *pos, valType *id)
{
    dbObject *newSet = NULL;
    if (NULL == (newSet = eval(s, pos)))
        return NULL;

    if (objectSet != newSet->objectType)
    {
        dbUnregisterObject(newSet->id);
        return NULL;
    }

    *id = newSet->id;
    return newSet;
}

int setPrint(set *s, FILE *f, int lock)
{
    setIterator *iter = NULL;
    valType counter = 0;

    if (NULL == s || NULL == f || -1 == setGetIter(s, &iter))
        return -1;

    fprintf(f, "{ ");

    if (NULL == iter)
    {
        fprintf(f, "}");
        return 0;
    }

    while (0 == setGetNext(iter))
    {
        dbObjectPrint(dbGetObject(iter->val, lock), f, lock);

        counter++;
        if (counter < s->card)
            fprintf(f, ",");

        fprintf(f, " ");
    }

    fprintf(f, "}");

    return 0;
}
