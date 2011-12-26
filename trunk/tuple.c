// tuple.c - Tuple utils.

#include <stdio.h>
#include <stdlib.h>

#include "adlist.h"

#include "athena.h"
#include "setutils.h"
#include "tuple.h"
#include "dbengine.h"
#include "syncengine.h"
#include "dbobject.h"
#include "eval.h"

dbObject *tupleParse(const sds s, size_t *pos, valType *id)
{
    dbObject *newTuple = NULL;
    if (NULL == (newTuple = eval(s, pos)))
        return NULL;

    if (objectTuple != newTuple->objectType)
    {
        dbUnregisterObject(newTuple->id);
        return NULL;
    }

    *id = newTuple->id;
    return newTuple;
}

int tupleCmp(list *a, list *b)
{
    listIter *aIter = NULL, *bIter = NULL;
    listNode *ai = NULL, *bi = NULL;

    if (NULL == a || NULL == b)
        return -1;

    if (listLength(a) != listLength(b))
    {
        return 0;
    }

    if (NULL == (aIter = listGetIterator(a, AL_START_HEAD)))
    {
        return -1;
    }

    if (NULL == (bIter = listGetIterator(b, AL_START_HEAD)))
    {
        listReleaseIterator(aIter);
        return -1;
    }

    while (NULL != (ai = listNext(aIter)) && NULL != (bi = listNext(bIter)))
    {
        if (ai->value != bi->value)
        {
            listReleaseIterator(aIter);
            listReleaseIterator(bIter);
            return 0;
        }
    }

    listReleaseIterator(aIter);
    listReleaseIterator(bIter);
    return 1;
}

int tuplePrint(list *tuple, FILE *f, int lock)
{
    listIter *iter = NULL;
    listNode *node = NULL;
    valType counter = 0;

    if (NULL == tuple || NULL == f)
        return -1;

    fprintf(f, "[ ");
    if (NULL == (iter = listGetIterator(tuple, AL_START_HEAD)))
    {
        fprintf(f, "]");
        return 0;
    }

    while (NULL != (node = listNext(iter)))
    {
        const dbObject *obj = dbGetObject((valType) listNodeValue(node), lock);

        if (NULL == obj)
        {
            fprintf(f, "(null)");
        }
        else
        {
            dbObjectPrint(obj, f, lock);
        }

        counter++;
        if (counter < listLength(tuple))
            fprintf(f, ",");

        fprintf(f, " ");
    }

    fprintf(f, "]");

    listReleaseIterator(iter);
    return 0;
}

set *tupleFlatten(list *t, int lock)
{
    listIter *iter = NULL;
    listNode *node = NULL;
    set *result = NULL;

    if (NULL == t)
        return NULL;

    if (NULL == (result = setCreate()))
        return NULL;

    lockRead(t);
    if (NULL == (iter = listGetIterator(t, AL_START_HEAD)))
    {
        unlockRead(t);
        setDestroy(result);
        return NULL;
    }

    while (NULL != (node = listNext(iter)))
    {
        valType objId = (valType) listNodeValue(node);
        const dbObject *obj = dbGetObject(objId, lock);

        if (NULL == obj)
            continue;

        if (-1 == setAdd(result, objId))
        {
            unlockRead(t);
            listReleaseIterator(iter);
            setDestroy(result);
            return NULL;
        }

        switch (obj->objectType)
        {
            case objectSet:
                {
                    set *flattened = setFlatten(obj->objectPtr.setPtr, lock);
                    set *mergeResult = NULL;

                    if (NULL == flattened)
                        continue;

                    mergeResult = setUnion(result, flattened);
                    setDestroy(flattened);
                    setDestroy(result);
                    if (NULL == mergeResult)
                    {
                        unlockRead(t);
                        listReleaseIterator(iter);
                        return NULL;
                    }
                    result = mergeResult;
                }
                break;

            case objectTuple:
                {
                    set *flattened = tupleFlatten(obj->objectPtr.tuplePtr, lock);
                    set *mergeResult = NULL;

                    if (NULL == flattened)
                        continue;

                    mergeResult = setUnion(result, flattened);
                    setDestroy(flattened);
                    setDestroy(result);
                    if (NULL == mergeResult)
                    {
                        unlockRead(t);
                        listReleaseIterator(iter);
                        return NULL;
                    }
                    result = mergeResult;
                }
                break;
        }
    }

    unlockRead(t);
    listReleaseIterator(iter);

    return result;
}
