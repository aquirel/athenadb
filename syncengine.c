// syncengine.h - synchronization engine for shared objects.

#include <Windows.h>

#include "dict.h"
#include "syncengine.h"

typedef struct syncObject
{
    SRWLOCK lock;
} syncObject;

static dict *registeredObjects;
static CRITICAL_SECTION syncCS;

dictType dictSyncEngineType;
unsigned int dictSyncObjectHash(const void *key);

int initSyncEngine(void)
{
    InitializeCriticalSection(&syncCS);

    if (NULL == (registeredObjects = dictCreate(&dictSyncEngineType, NULL)))
    {
        return -1;
    }

    return 0;
}

void cleanupSyncEngine(void)
{
    dictIterator *i = NULL;
    dictEntry *de = NULL;

    if (NULL == registeredObjects)
    {
        DeleteCriticalSection(&syncCS);
        return;
    }

    i = dictGetIterator(registeredObjects);

    if (NULL != i)
    {
        while (NULL != (de = dictNext(i)))
        {
            unregisterSyncObject(dictGetEntryKey(de));
        }
    }

    DeleteCriticalSection(&syncCS);

    dictReleaseIterator(i);
    dictRelease(registeredObjects);
}

int registerSyncObject(const void *object)
{
    syncObject *so = NULL;

    EnterCriticalSection(&syncCS);

    if (NULL == object || NULL == registeredObjects)
    {
        LeaveCriticalSection(&syncCS);
        return -1;
    }

    if (NULL != dictFetchValue(registeredObjects, object))
    {
        LeaveCriticalSection(&syncCS);
        return -2;
    }

    if (NULL == (so = (syncObject *) calloc(1, sizeof(syncObject))))
    {
        LeaveCriticalSection(&syncCS);
        return -3;
    }

    InitializeSRWLock(&(so->lock));

    if (DICT_OK != dictAdd(registeredObjects, (void *) object, so))
    {
        LeaveCriticalSection(&syncCS);
        free(so);
        return -1;
    }

    LeaveCriticalSection(&syncCS);
    return 0;
}

int syncObjectIsRegistered(const void *object)
{
    int result = 0;

    if (NULL == object)
        return 0;

    EnterCriticalSection(&syncCS);
    result = NULL != dictFetchValue(registeredObjects, object);
    LeaveCriticalSection(&syncCS);

    return result;
}

void unregisterSyncObject(const void * object)
{
    syncObject *so = NULL;

    EnterCriticalSection(&syncCS);

    if (NULL == object || NULL == registeredObjects)
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (NULL == (so = (syncObject *) dictFetchValue(registeredObjects, object)))
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    /*ReleaseSRWLockExclusive(&(so->lock));
    ReleaseSRWLockShared(&(so->lock));*/

    dictDelete(registeredObjects, object);
    LeaveCriticalSection(&syncCS);
}

void lockRead(const void * object)
{
    syncObject *so = NULL;

    EnterCriticalSection(&syncCS);

    if (NULL == object || NULL == registeredObjects)
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (NULL == (so = (syncObject *) dictFetchValue(registeredObjects, object)))
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    LeaveCriticalSection(&syncCS);
    AcquireSRWLockShared(&(so->lock));
}

void lockWrite(const void * object)
{
    syncObject *so = NULL;

    EnterCriticalSection(&syncCS);

    if (NULL == object || NULL == registeredObjects)
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (NULL == (so = (syncObject *) dictFetchValue(registeredObjects, object)))
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    LeaveCriticalSection(&syncCS);
    AcquireSRWLockExclusive(&(so->lock));
}

void unlockRead(const void * object)
{
    syncObject *so = NULL;

    EnterCriticalSection(&syncCS);

    if (NULL == object || NULL == registeredObjects)
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (NULL == (so = (syncObject *) dictFetchValue(registeredObjects, object)))
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (0 != so->lock.Ptr)
        ReleaseSRWLockShared(&(so->lock));
    LeaveCriticalSection(&syncCS);
}

void unlockWrite(const void * object)
{
    syncObject *so = NULL;

    EnterCriticalSection(&syncCS);

    if (NULL == object || NULL == registeredObjects)
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (NULL == (so = (syncObject *) dictFetchValue(registeredObjects, object)))
    {
        LeaveCriticalSection(&syncCS);
        return;
    }

    if (0 != so->lock.Ptr)
        ReleaseSRWLockExclusive(&(so->lock));
    LeaveCriticalSection(&syncCS);
}

dictType dictSyncEngineType =
{
    dictSyncObjectHash, /* hash function */
    NULL,               /* key dup */
    NULL,               /* val dup */
    NULL,               /* key compare */
    NULL,               /* key destructor */
    NULL                /* val destructor */
};

unsigned int dictSyncObjectHash(const void *key)
{
    return (unsigned int) key;
}
