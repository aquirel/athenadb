// dbobject.c - Database object management.

#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>

#include "set.h"
#include "setutils.h"
#include "tuple.h"
#include "dbengine.h"
#include "dbobject.h"
#include "eval.h"

int dbObjectCompare(const dbObject *a, const dbObject *b)
{
    if (NULL == a || NULL == b)
        return 0;

    if (a == b)
        return 1;

    if (a->objectType != b->objectType)
        return 0;

    switch (a->objectType)
    {
        case objectSet:
            return setCmpE(a->objectPtr.setPtr, b->objectPtr.setPtr);

        case objectTuple:
            return tupleCmp(a->objectPtr.tuplePtr, b->objectPtr.tuplePtr);

        case objectVal:
            return a->objectPtr.val == b->objectPtr.val;
    }

    return 0;
}

void dbObjectRelease(dbObject *obj)
{
    if (NULL == obj)
        return;

    switch (obj->objectType)
    {
        case objectSet:
            setDestroy(obj->objectPtr.setPtr);
            break;

        case objectTuple:
            listRelease(obj->objectPtr.tuplePtr);
            break;

        case objectVal:
            break;
    }
}

int dbObjectPrint(const dbObject *obj, FILE *f, int lock)
{
    if (NULL == obj || NULL == f)
        return -1;

    switch (obj->objectType)
    {
        case objectSet:
            return setPrint(obj->objectPtr.setPtr, f, lock);

        case objectTuple:
            return tuplePrint(obj->objectPtr.tuplePtr, f, lock);

        case objectVal:
            return 0 > fprintf(f, "%u", obj->objectPtr.val);
    }

    return -1;
}

dbObject *valParse(const char *tokenPtr, valType *id)
{
    valType newVal = 0;
    dbObject *newValObject = NULL;

    if (1 != sscanf(tokenPtr, "%u", &newVal))
        return NULL;

    if (NULL == (newValObject = (dbObject *) calloc(1, sizeof(dbObject))))
        return NULL;

    newValObject->objectType = objectVal;
    newValObject->objectPtr.val = newVal;

    if (0 != dbRegisterObject(&newValObject, id))
    {
        free(newValObject);
        return NULL;
    }

    return newValObject;
}

dbObject *dbObjectParse(const sds s, valType *id)
{
    dbObject *result = NULL;
    size_t pos = 0;

    if (NULL == s || 0 == strlen(s) || NULL == id)
        return NULL;

    while (isspace(s[pos]))
        pos++;

    if (NULL == (result = eval(s, &pos)))
    {
        return NULL;
    }

    *id = result->id;

    return result;
}
