// set.c - Set API and implementation.

#include <malloc.h>
#include <stdlib.h>
#include <memory.h>
#include <stdlib.h>

#include "adlist.h"

#include "dbengine.h"
#include "syncengine.h"
#include "dbobject.h"
#include "set.h"
#include "tuple.h"

set *setCreate(void)
{
    set *s = (set *) malloc(sizeof(set));

    if (s)
    {
        s->card = 0;
        s->data = 0;
        s->length = 0;
        s->registered = 0;
    }

    return s;
}

set *setCopy(const set *s)
{
    set *result;

    if (NULL == (result = setCreate()))
        return NULL;

    if (NULL == (result->data = (char *) malloc(s->length * sizeof(char))))
    {
        free(result);
        return NULL;
    }

    result->length = s->length;
    result->card = s->card;
    memcpy(result->data, s->data, s->length);

    return result;
}

void setDestroy(set *s)
{
    if (s)
    {
        if (s->data)
            free(s->data);
        free(s);
    }
}

void setDestroyIter(setIterator *iter)
{
    if (NULL != iter)
        free(iter);
}

int setAdd(set *s, valType val)
{
    if (NULL == s)
        return -1;

    if (!setCanHold(s, val))
    {
        if (0 != setGrow(s, val))
            return -1;
    }

    if (1 == setGetBit(s, val))
    {
        return 1;
    }
    else if (-1 == setSetBit(s, val, 1))
    {
        return -1;
    }

    s->card++;

    return 0;
}

int setRemove(set *s, valType val)
{
    if (NULL == s)
        return -1;

    if (!setCanHold(s, val) || 0 == setGetBit(s, val))
        return 1;

    if (-1 == setSetBit(s, val, 0))
        return -1;

    s->card--;

    return 0;
}

valType setCard(const set *s)
{
    if (NULL == s)
        return 0;

    return s->card;
}

int setIsMember(const set *s, valType val)
{
    return 1 == setGetBit(s, val);
}

set *setDiff(const set *a, const set *b)
{
    set *result;
    valType byte, bytesCount;

    if (NULL == a || NULL == b || NULL == (result = setCopy(a)))
        return NULL;

    bytesCount = __min(a->length, b->length);
    for (byte = 0; byte < bytesCount; byte++)
    {
        result->data[byte] &= ~b->data[byte];
        result->card -= byteBitCount(a->data[byte] & b->data[byte]);
    }

    return result;
}

set *setSymDiff(const set *a, const set *b)
{
    return setUnion(setDiff(a, b), setDiff(b, a));
}

set *setInter(const set *a, const set *b)
{
    set *result;
    valType byte;

    if (NULL == a || NULL == b || NULL == (result = setCreate()))
        return NULL;

    result->length = __min(a->length, b->length);
    result->card = 0;

    if (0 == result->length ||
        0 == a->card ||
        0 == b->card)
    {
        return result;
    }

    if (NULL == (result->data = (char *) malloc(result->length * sizeof(char))))
    {
        setDestroy(result);
        return NULL;
    }

    for (byte = 0; byte < result->length; byte++)
    {
        result->data[byte] = a->data[byte] & b->data[byte];
        result->card += byteBitCount(result->data[byte]);
    }

    return result;
}

set *setUnion(const set *a, const set *b)
{
    set *result;
    valType byte; 

    if (NULL == a || NULL == b || NULL == (result = setCreate()))
        return NULL;

    result->length = __max(a->length, b->length);
    result->card = 0;

    if (0 == result->length ||
        (0 == a->card && 0 == b->card))
    {
        return result;
    }

    if (NULL == (result->data = (char *) calloc(result->length, sizeof(char))))
    {
        setDestroy(result);
        return NULL;
    }

    for (byte = 0; byte < a->length; byte++)
    {
        result->data[byte] = a->data[byte];
    }

    for (byte = 0; byte < b->length; byte++)
    {
        result->data[byte] |= b->data[byte];
    }

    for (byte = 0; byte < result->length; byte++)
    {
        result->card += byteBitCount(result->data[byte]);
    }

    return result;
}

set *setCartProd(const set *a, const set *b)
{
    set *result = NULL;
    setIterator *aIter = NULL, *bIter = NULL, bIterStart;

    if (NULL == a || NULL == b || NULL == (result = setCreate()))
        return NULL;

    if (0 != setGetIter(a, &aIter) || NULL == aIter)
    {
        setDestroy(result);
        return NULL;
    }

    if (0 != setGetIter(b, &bIter) || NULL == bIter)
    {
        setDestroyIter(aIter);
        setDestroy(result);
        return NULL;
    }

    memcpy(&bIterStart, bIter, sizeof(setIterator));

    while (0 == setGetNext(aIter))
    {
        memcpy(bIter, &bIterStart, sizeof(setIterator));

        while (0 == setGetNext(bIter))
        {
            dbObject *newTuple = NULL;
            valType newTupleId = 0;
            list *tup = NULL;
            const dbObject *aObject = NULL, *bObject = NULL;

            if (NULL == (tup = listCreate()))
            {
                setDestroyIter(aIter);
                setDestroyIter(bIter);
                setDestroy(result);
                return NULL;
            }

            aObject = dbGetObject(aIter->val, 1);
            bObject = dbGetObject(bIter->val, 1);

            if (NULL == aObject || NULL == bObject)
            {
                setDestroyIter(aIter);
                setDestroyIter(bIter);
                setDestroy(result);
                listRelease(tup);
                return NULL;
            }

            if (NULL == listAddNodeTail(tup, (void *) aObject->id) ||
                NULL == listAddNodeTail(tup, (void *) bObject->id))
            {
                setDestroyIter(aIter);
                setDestroyIter(bIter);
                setDestroy(result);
                listRelease(tup);
                return NULL;
            }

            if (NULL == (newTuple = (dbObject *) calloc(1, sizeof(dbObject))))
            {
                setDestroyIter(aIter);
                setDestroyIter(bIter);
                setDestroy(result);
                listRelease(tup);
                return NULL;
            }

            newTuple->objectType = objectTuple;
            newTuple->objectPtr.tuplePtr = tup;

            if (0 != dbRegisterObject(&newTuple, &newTupleId))
            {
                setDestroyIter(aIter);
                setDestroyIter(bIter);
                setDestroy(result);
                listRelease(tup);
                free(newTuple);
                return NULL;
            }

            if (-1 == setAdd(result, newTupleId))
            {
                setDestroyIter(aIter);
                setDestroyIter(bIter);
                setDestroy(result);
                listRelease(tup);
                dbUnregisterObject(newTupleId);
                free(newTuple);
                return NULL;
            }
        }
    }

    setDestroyIter(aIter);
    setDestroyIter(bIter);
    return result;
}

set *setBoolean(const set *a)
{
    valType *contents = NULL;
    valType contentsIndex = 0;
    valType booleanCard = 1;
    valType i, step, pos;
    setIterator *iter = NULL;
    set **subsets = NULL;
    set *result;

    if (NULL == a || 0 != setGetIter(a, &iter))
        return NULL;

    if (NULL == (result = setCreate()))
    {
        setDestroyIter(iter);
    }

    // Calculate boolean cardinality.
    for (i = 0; i < a->card; i++)
        booleanCard <<= 1;

    if (NULL == (subsets = (set **) calloc(booleanCard, sizeof(set *))))
    {
        setDestroyIter(iter);
        setDestroy(result);
        return NULL;
    }

    if (NULL == (contents = (valType *) calloc(a->card, sizeof(valType))))
    {
        free(subsets);
        setDestroyIter(iter);
        setDestroy(result);
        return NULL;
    }

    // Create subsets.
    for (i = 0; i < booleanCard; i++)
    {
        valType j;

        if (NULL == (subsets[i] = setCreate()))
        {
            for (j = 0; j < i; j++)
                setDestroy(subsets[i]);

            free(subsets);
            free(contents);
            setDestroyIter(iter);
            setDestroy(result);
            return NULL;
        }
    }

    // Fetch objects.
    while (0 == setGetNext(iter))
    {
        contents[contentsIndex++] = iter->val;
    }
    setDestroyIter(iter);

    // Fill sets.
    step = booleanCard;
    for (i = 0; i < a->card; i++) // a->card is contentsLength.
    {
        step >>= 1;
        pos = 0;

        while (pos < booleanCard)
        {
            valType j;

            pos += step; // First part - don't include.

            for (j = 0; j < step; j++)
            {
                setAdd(subsets[pos + j], contents[i]);
            }

            pos += step;
        }
    }

    // Register sets.
    for (i = 0; i < booleanCard; i++)
    {
        dbObject *newSetObject = NULL;
        valType newObjectId;

        if (NULL == (newSetObject = (dbObject *) calloc(1, sizeof(dbObject))))
        {
            valType j;
            for (j = 0; j < i; j++)
                setDestroy(subsets[i]);

            free(subsets);
            free(contents);
            setDestroy(result);
            return NULL;
        }

        newSetObject->objectType = objectSet;
        newSetObject->objectPtr.setPtr = subsets[i];

        dbRegisterObject(&newSetObject, &newObjectId);

        setAdd(result, newObjectId);
    }

    free(subsets);
    free(contents);

    return result;
}

set *setFlatten(const set *s, int lock)
{
    setIterator *iter = NULL;
    set *result = NULL;

    if (NULL == s)
        return NULL;

    lockRead(s);

    if (NULL == (result = setCopy(s)))
    {
        unlockRead(s);
        return NULL;
    }

    if (0 != setGetIter(s, &iter))
    {
        unlockRead(s);
        setDestroy(result);
        return NULL;
    }

    while (0 == setGetNext(iter))
    {
        const dbObject *obj = dbGetObject(iter->val, lock);

        if (NULL == obj)
            continue;

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
                        unlockRead(s);
                        setDestroyIter(iter);
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
                        unlockRead(s);
                        setDestroyIter(iter);
                        return NULL;
                    }
                    result = mergeResult;
                }
                break;
        }
    }

    unlockRead(s);
    setDestroyIter(iter);
    return result;
}

int setCmpE(const set *a, const set *b)
{
    valType byte, bytesCount;

    if (NULL == a || NULL == b)
        return -1;

    if (a == b)
        return 1;

    if (a->card != b->card)
        return 0;

    bytesCount = __min(a->length, b->length);
    for (byte = 0; byte < bytesCount; byte++)
    {
        if (a->data[byte] != b->data[byte])
            return 0;
    }

    return 1;
}

int setCmpSubset(const set *a, const set *b)
{
    valType byte, bytesCount, aBytes;

    if (NULL == a || NULL == b)
        return -1;

    if (0 == a->card)
        return 1;

    if (a->card > b->card)
        return 0;

    bytesCount = __min(a->length, b->length);
    aBytes = 0;
    for (byte = 0; byte < bytesCount; byte++)
    {
        aBytes += byteBitCount(a->data[byte]);
        if ((a->data[byte] | b->data[byte]) != b->data[byte])
            return 0;
    }

    if (aBytes != a->card)
        return 0;

    return 1;
}

int setCmpSubsetOrEq(const set *a, const set *b)
{
    return setCmpSubset(a, b) || setCmpE(a, b);
}

int setGetRand(const set *s, valType *val)
{
    setIterator *i;
    valType *values, valuesIndex = 0;

    if (NULL == s || -1 == setGetIter(s, &i) || NULL == i)
        return -1;

    if (NULL == (values = (valType *) malloc(s->card * sizeof(valType))))
    {
        setDestroyIter(i);
        return -1;
    }

    while (0 == setGetNext(i))
    {
        values[valuesIndex++] = i->val;
    }

    valuesIndex = (rand() * rand()) % s->card;

    *val = values[valuesIndex];

    setDestroyIter(i);
    free(values);

    return 0;
}

unsigned setTrunc(set *s)
{
    unsigned i, freed;

    if (NULL == s)
        return 0;

    if (0 == s->length)
        return 0;

    if (0 == s->card)
    {
        unsigned freed = s->length;
        s->data = (char *) realloc(s->data, 0);
        s->length = 0;
        return freed;
    }

    i = s->length - 1;
    while (0 == s->data[i])
    {
        if (0 == i)
            break;

        i--;
    }

    freed = s->length - i - 1;
    s->length = i + 1;

    s->data = (char *) realloc(s->data, s->length);

    return freed;
}

int setGetIter(const set *s, setIterator **iter)
{
    setIterator *i;
    valType bit;

    if (NULL == s)
        return -1;

    if (0 == s->length || 0 == s->card)
    {
        *iter = NULL;
        return 0;
    }

    for (bit = 0; bit < 8 * s->length; bit++)
    {
        if (1 == setGetBit(s, bit))
        {
            if (NULL == (i = (setIterator *) malloc(sizeof(setIterator))))
                return -1;

            i->s = s;
            i->i = bit;

            *iter = i;

            return 0;
        }
    }

    *iter = NULL;

    return 0;
}

int setGetNext(setIterator *iter)
{
    if (NULL == iter || NULL == iter->s || 0 == iter->s->length || 0 == iter->s->card)
        return -1;

    for (; iter->i < 8 * iter->s->length; )
    {
        if (1 == setGetBit(iter->s, iter->i))
        {
            iter->val = iter->i;
            iter->i++;
            return 0;
        }

        iter->i++;
    }

    return -1;
}

int setCanHold(set *s, valType val)
{
    if (NULL == s)
        return 0;

    if (s->length * 8 >= val + 1)
        return 1;

    return 0;
}

int setGrow(set *s, valType val)
{
    char *t;
    valType newLen = 0;

    if (setCanHold(s, val))
        return 0;

    if (NULL == s)
        return -1;

    newLen = 1 + val / 8;
    t = (char *) realloc(s->data, newLen);

    if (NULL != t)
    {
        memset(t + s->length, 0, newLen - s->length);
        s->length = newLen;
        s->data = t;
        return 0;
    }

    return -1;
}

int setSetBit(set *s, valType bit, unsigned val)
{
    valType byte = bit / 8;
    char v = 1 << (bit % 8);

    if (byte >= s->length * 8)
        return -1;

    // Make val 0 or 1;
    if (val = val ? 1 : 0)
        s->data[byte] |= v;
    else
        s->data[byte] &= ~v;

    return val;
}

int setGetBit(const set *s, valType bit)
{
    valType byte = bit / 8;
    char v = 1 << (bit % 8);

    if (byte >= s->length)
        return 0;

    return 0 != (s->data[byte] & v);
}

unsigned char bitCountMatrix[256] =
{
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3,
    4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4,
    4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2,
    3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5,
    4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4,
    5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
    3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
    3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

__inline static int byteBitCount(unsigned byte)
{
    return bitCountMatrix[byte & 0xff];
}
