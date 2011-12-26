// dbengine.c - Database engine.

#include <memory.h>
#include <stdlib.h>

#include "dict.h"
#include "sds.h"

#include "dbengine.h"
#include "dbobject.h"
#include "syncengine.h"
#include "setutils.h"

static dict *sets;
static const dbObject **objectIndex;
static valType objectIndexLength, objectIndexFreeId, objectIndexCount;

static dictType dictSetType;

int initDbEngine(void)
{
    objectIndexLength = 256;
    objectIndexFreeId = 0;
    objectIndexCount = 0;
    if (NULL == (objectIndex = (const dbObject **) calloc(objectIndexLength, (sizeof(const dbObject *)))))
        return -1;

    if (NULL == (sets = dictCreate(&dictSetType, NULL)))
    {
        free((void *) objectIndex);
        return -1;
    }

    if (0 != registerSyncObject(sets))
    {
        free((void *) objectIndex);
        dictRelease(sets);
        return -1;
    }

    if (0 != registerSyncObject(objectIndex))
    {
        unregisterSyncObject(sets);
        free((void *) objectIndex);
        dictRelease(sets);
        return -1;
    }

    return 0;
}

void cleanupDbEngine(void)
{
    dictIterator *i = NULL;
    dictEntry *de = NULL;
    valType c;

    unregisterSyncObject(sets);
    unregisterSyncObject(objectIndex);

    if (NULL == sets)
        return;

    i = dictGetIterator(sets);

    if (NULL != i)
    {
        while (NULL != (de = dictNext(i)))
        {
            set *setToDelete = (set *) dictGetEntryVal(de);

            if (!setToDelete->registered)
                setDestroy(setToDelete);
        }
    }

    dictReleaseIterator(i);
    dictRelease(sets);

    for (c = 0; c < objectIndexLength; c++)
        if (NULL != objectIndex[c])
        {
            dbObjectRelease((dbObject *) objectIndex[c]);
            free((void *) objectIndex[c]);
        }

    free((void *) objectIndex);
}

const set *dbGet(const sds setName)
{
    const set *result = NULL;
    lockRead(sets);
    result = (const set *) dictFetchValue(sets, setName);
    unlockRead(sets);
    return result;
}

const sds dbGetRandSet(void)
{
    dictEntry *entry = NULL;
    sds result = NULL;
    lockRead(sets);
    if (NULL != (entry = dictGetRandomKey(sets)))
    {
        result = (sds) entry->key;
    }
    unlockRead(sets);
    return result;
}

int dbRemove(const sds setName)
{
    int result = DICT_OK;
    set *s = NULL;
    lockWrite(sets);
    s = (set *) dictFetchValue(sets, setName);
    if (NULL != s)
    {
        unregisterSyncObject(s);
        if (!s->registered)
            setDestroy(s);
        result = dictDelete(sets, setName);
    }
    unlockWrite(sets);
    return result;
}

int dbFlushAll(void)
{
    dictIterator *iter = NULL;
    dictEntry *entry = NULL;

    lockWrite(sets);

    if (NULL == (iter = dictGetIterator(sets)))
    {
        unlockWrite(sets);
        return -1;
    }

    while (NULL != (entry = dictNext(iter)))
    {
        set *s = (set *) dictGetEntryVal(entry);
        lockWrite(s);
        if (!s->registered)
            setDestroy(s);
        unregisterSyncObject(s);
    }

    dictReleaseIterator(iter);
    dictEmpty(sets);
    unlockWrite(sets);
    return 0;
}

int dbRename(const sds oldSetName, const sds newSetName)
{
    const set *oldSet = NULL;

    if (NULL == oldSetName || NULL == newSetName ||
        0 == strlen(oldSetName) || 0 == strlen(newSetName))
    {
        return -1;
    }

    if (NULL == (oldSet = dbGet(oldSetName)) ||
        NULL != dbGet(newSetName))
    {
        return -1;
    }

    lockWrite(sets);
    dictDelete(sets, oldSetName);
    dictAdd(sets, newSetName, (void *) oldSet);
    unlockWrite(sets);
    return 0;
}

const set *dbCreate(const sds setName)
{
    set *newSet = NULL;

    if (NULL == setName || 0 == strlen(setName))
    {
        return NULL;
    }

    if (NULL == (newSet = setCreate()))
        return NULL;

    lockWrite(sets);

    if (DICT_OK != dictAdd(sets, setName, newSet))
    {
        unlockWrite(sets);
        setDestroy(newSet);
        return NULL;
    }

    if (0 != registerSyncObject(newSet) && 1 != syncObjectIsRegistered(newSet))
    {
        unlockWrite(sets);
        dictDelete(sets, setName);
        setDestroy(newSet);
        return NULL;
    }

    unlockWrite(sets);
    return newSet;
}

int dbSet(const sds setName, const set *s)
{
    set *prevSet = NULL;

    if (NULL == setName || 0 == strlen(setName) || NULL == s)
        return -1;

    lockWrite(sets);
    if (NULL != (prevSet = (set *) dictFetchValue(sets, setName)))
    {
        lockWrite(prevSet);
        if (!prevSet->registered)
            setDestroy(prevSet);
        unregisterSyncObject(prevSet);
    }

    if (0 != registerSyncObject(s) && 1 != syncObjectIsRegistered(s))
    {
        unlockWrite(sets);
        return -1;
    }

    dictReplace(sets, setName, (void *) s);
    unlockWrite(sets);
    return 0;
}

const dbObject *dbGetObject(valType id, int lock)
{
    const dbObject *result = NULL;

    if (lock)
        lockRead(objectIndex);

    if (id >= objectIndexLength)
    {
        unlockRead(objectIndex);
        return NULL;
    }

    result = objectIndex[id];

    if (lock)
        unlockRead(objectIndex);
    return result;
}

int dbRegisterObject(dbObject **object, valType *id)
{
    valType prevObjectIndexFreeId = 0;

    if (NULL == id || NULL == object || NULL == *object)
    {
        return -1;
    }

    lockWrite(objectIndex);

    prevObjectIndexFreeId = objectIndexFreeId;

    if (1 == dbFindObject(*object, id, 0))
    {
        dbObjectRelease(*object);
        free(*object);
        *object = (dbObject *) objectIndex[*id];
        unlockWrite(objectIndex);
        return 0;
    }

    while (objectIndexFreeId < objectIndexLength && NULL != objectIndex[objectIndexFreeId])
        objectIndexFreeId++;

    if (objectIndexFreeId < objectIndexLength)
    {
        objectIndex[objectIndexFreeId] = *object;
        *id = objectIndexFreeId;
        objectIndexFreeId++;
        objectIndexCount++;
        unlockWrite(objectIndex);

        if (objectSet == (*object)->objectType)
        {
            (*object)->objectPtr.setPtr->registered = 1;
        }

        // Debug
        printf("Registered object %u = ", *id);
        dbObjectPrint(*object, stdout, 0);
        printf("\r\n");

        (*object)->id = *id;

        return 0;
    }

    objectIndexLength += 256;
    if (NULL == (objectIndex = (const dbObject **) realloc((void *) objectIndex, objectIndexLength * sizeof(dbObject *))))
    {
        objectIndexLength = prevObjectIndexFreeId;
        unlockWrite(objectIndex);
        return -1;
    }

    memset((void *) (objectIndex + prevObjectIndexFreeId), 0, 256);

    objectIndex[objectIndexFreeId] = *object;
    *id = objectIndexFreeId;
    objectIndexFreeId++;
    objectIndexCount++;
    unlockWrite(objectIndex);

    if (objectSet == (*object)->objectType)
    {
        (*object)->objectPtr.setPtr->registered = 1;
    }

    (*object)->id = *id;

    return 0;
}

int dbFindObject(const dbObject *object, valType *index, int lock)
{
    valType i;

    if (NULL == object)
        return 0;

    if (lock)
        lockRead(objectIndex);

    for (i = 0; i < objectIndexLength; i++)
    {
        if (NULL != objectIndex[i] &&
            1 == dbObjectCompare(object, objectIndex[i]))
        {
            if (index)
                *index = i;

            if (lock)
                unlockRead(objectIndex);
            return 1;
        }
    }

    if (lock)
        unlockRead(objectIndex);
    return 0;
}

int dbFindSet(const set *s, valType *index, int lock)
{
    valType i;

    if (NULL == s)
        return 0;

    if (lock)
        lockRead(objectIndex);

    for (i = 0; i < objectIndexLength; i++)
    {
        if (NULL != objectIndex[i] &&
            objectSet == objectIndex[i]->objectType &&
            1 == setCmpE(s, objectIndex[i]->objectPtr.setPtr))
        {
            if (index)
                *index = i;

            if (lock)
                unlockRead(objectIndex);
            return 1;
        }
    }

    if (lock)
        unlockRead(objectIndex);
    return 0;
}

valType dbSetTrunc(void)
{
    valType freed = 0, i;
    dictIterator *iter = NULL;
    dictEntry *entry = NULL;

    lockWrite(objectIndex);
    lockRead(sets);

    if (NULL == (iter = dictGetIterator(sets)))
    {
        unlockRead(sets);
        unlockWrite(objectIndex);
        return 0;
    }

    while (NULL != (entry = dictNext(iter)))
    {
        lockWrite(entry);
        freed += setTrunc((set *) dictGetEntryVal(entry));
        unlockWrite(entry);
    }

    for (i = 0; i < objectIndexLength; i++)
    {
        if (NULL != objectIndex[i] &&
            objectSet == objectIndex[i]->objectType)
        {
            freed += setTrunc(objectIndex[i]->objectPtr.setPtr);
        }
    }

    dictReleaseIterator(iter);

    unlockRead(sets);
    unlockWrite(objectIndex);

    return freed;
}

valType dbGC(void)
{
    valType collected = 0, i;
    dictIterator *iter = NULL;
    dictEntry *entry = NULL;
    set *acc = NULL;

    lockWrite(objectIndex);
    lockRead(sets);

    // Step 1. Union all sets in db.
    if (NULL == (iter = dictGetIterator(sets)))
    {
        unlockRead(sets);
        unlockWrite(objectIndex);
        return 0;
    }

    if (NULL == (acc = setCreate()))
    {
        unlockRead(sets);
        unlockWrite(objectIndex);
        return 0;
    }

    while (NULL != (entry = dictNext(iter)))
    {
        valType currentSetId = 0;
        set *tmp = NULL, *currentSet = (set *) dictGetEntryVal(entry);
        set *flattened = setFlatten(currentSet, 0);

        if (NULL == flattened)
        {
            continue;
        }

        if (1 == dbFindSet(currentSet, &currentSetId, 0))
        {
            if (-1 == setAdd(acc, currentSetId))
            {
                setDestroy(acc);
                dictReleaseIterator(iter);
                unlockRead(sets);
                unlockWrite(objectIndex);
                return 0;
            }
        }

        if (NULL == (tmp = setUnion(acc, flattened)))
        {
            setDestroy(flattened);
            setDestroy(acc);
            dictReleaseIterator(iter);
            unlockRead(sets);
            unlockWrite(objectIndex);
            return 0;
        }

        setDestroy(flattened);
        setDestroy(acc);
        acc = tmp;
    }

    dictReleaseIterator(iter);

    // Step 2. Find objects not present in grand total union.
    for (i = 0; i < objectIndexLength; i++)
    {
        if (NULL != objectIndex[i] && !setIsMember(acc, i))
        {
            dbObjectRelease((dbObject *) objectIndex[i]);
            free((dbObject *) objectIndex[i]);
            objectIndex[i] = NULL;

            if (i < objectIndexFreeId)
            {
                objectIndexFreeId = i;
            }

            collected++;
        }
    }

    setDestroy(acc);

    unlockRead(sets);
    unlockWrite(objectIndex);

    return collected;
}

void dbPrintIndex(FILE *f)
{
    valType i;

    if (NULL == f)
        return;

    fprintf(f, "Object index dump. Total: %u\r\n", objectIndexCount);

    lockWrite(objectIndex);

    for (i = 0; i < objectIndexLength; i++)
    {
        if (NULL != objectIndex[i])
        {
            const dbObject *obj = objectIndex[i];
            fprintf(f, "%10u: ", i);

            switch (obj->objectType)
            {
                case objectSet:
                    fprintf(f, "set = ");
                    break;

                case objectTuple:
                    fprintf(f, "tuple = ");
                    break;

                case objectVal:
                    fprintf(f, "val = ");
                    break;
            }

            dbObjectPrint(obj, f, 0);
            fprintf(f, "\r\n");
        }
    }

    unlockWrite(objectIndex);
}

void dbPrintSets(FILE *f)
{
    dictIterator *iter = NULL;
    dictEntry *entry = NULL;

    if (NULL == f)
        return;

    lockRead(sets);

    if (0 == dictSize(sets))
    {
        unlockRead(sets);
        fprintf(f, "No sets in db.\r\n");
        return;
    }

    if (NULL == (iter = dictGetIterator(sets)))
    {
        unlockRead(sets);
        return;
    }

    while (NULL != (entry = dictNext(iter)))
    {
        fprintf(f, "%s\r\n", dictGetEntryKey(entry));
        lockRead(dictGetEntryVal(entry));
        setPrint((set *) dictGetEntryVal(entry), f, 1);
        unlockRead(dictGetEntryVal(entry));
        fprintf(f, "\r\n");
    }

    dictReleaseIterator(iter);
    unlockRead(sets);
}

void dbPrintStatus(void *serverPtr)
{
    server *s = (server *) serverPtr;

    if (NULL == s)
        return;

    while (s->working)
    {
        printf("Sets: %u. Objects: %u. Clients: %u\n", dictSize(sets), objectIndexCount, listLength(s->clients));
        Sleep(BG_STATUS_SLEEP);
    }
}

int dbUnregisterObject(valType id)
{
    lockWrite(objectIndex);
    if (id >= objectIndexLength || NULL == objectIndex[id])
    {
        unlockWrite(objectIndex);
        return -1;
    }

    dbObjectRelease((dbObject *) objectIndex[id]);
    free((void *) objectIndex[id]);
    objectIndex[id] = NULL;
    objectIndexCount++;
    unlockWrite(objectIndex);

    return 1;
}

// Private api.
unsigned int dictSdsHash(const void *key);
void *dictSdsKeyDup(void *privdata, const void *key);
int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2);
void dictSdsDestructor(void *privdata, void *val);

/* Db->dict, keys are sds strings, vals are Redis objects. */
dictType dictSetType =
{
    dictSdsHash,                 /* hash function */
    dictSdsKeyDup,               /* key dup */
    NULL,                        /* val dup */
    dictSdsKeyCompare,           /* key compare */
    dictSdsDestructor,           /* key destructor */
    NULL                         /* val destructor */
};

void *dictSdsKeyDup(void *privdata, const void *key)
{
    return (void *) sdsdup((sds) key);
}

int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = sdslen((sds) key1);
    l2 = sdslen((sds) key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

unsigned int dictSdsHash(const void *key)
{
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}

void dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    sdsfree((sds) val);
}
