// commands.c - Command implementations.

#include <stddef.h>

#include "dict.h"

#include "dbengine.h"
#include "syncengine.h"
#include "athena.h"
#include "eval.h"
#include "setutils.h"

void setCommand(FILE *f, int argc, sds *argv)
{
    sds newSet = NULL, expr = NULL;
    dbObject *newSetValue = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (0 == argc)
    {
        fprintf(f, "Expected 1 or 2 arguments.\r\n");
        return;
    }

    newSet = argv[0];
    expr = 2 == argc ? argv[1] : NULL;

    if (NULL == newSet || 0 == strlen(newSet))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == expr)
    {
        valType newSetId;

        if (NULL == (newSetValue = (dbObject *) calloc(1, sizeof(dbObject))))
        {
            fprintf(f, "ERROR.\r\n");
            return;
        }

        if (NULL == (newSetValue->objectPtr.setPtr = setCreate()))
        {
            free(newSetValue);
            fprintf(f, "ERROR.\r\n");
            return;
        }

        newSetValue->objectType = objectSet;

        if (0 != dbRegisterObject(&newSetValue, &newSetId))
        {
            dbObjectRelease(newSetValue);
            free(newSetValue);
            fprintf(f, "ERROR.\r\n");
            return;
        }
    }
    else
    {
        size_t pos = 0;
        if (NULL == (newSetValue = eval(expr, &pos)))
        {
            fprintf(f, "ERROR.\r\n");
            return;
        }
    }

    if (objectSet != newSetValue->objectType)
    {
        fprintf(f, "Wrong initializer type.\r\n");
        return;
    }

    if (0 == dbSet(newSet, newSetValue->objectPtr.setPtr))
    {
        fprintf(f, "OK.\r\n");
    }
    else
    {
        fprintf(f, "ERROR.\r\n");
    }
}

void delCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == dbGet(setName))
    {
        fprintf(f, "ERROR.\r\n");
        return;
    }

    if (DICT_OK != dbRemove(setName))
    {
        fprintf(f, "ERROR.\r\n");
        return;
    }

    fprintf(f, "OK.\r\n");
}

void existsCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == dbGet(setName))
    {
        fprintf(f, "0\r\n");
        return;
    }

    fprintf(f, "1\r\n");
}

void containsCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL, member = NULL;
    valType memberId = 0;
    const set *container = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (member = argv[1]) || 0 == strlen(member))
    {
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    if (NULL == (container = dbGet(setName)))
    {
        fprintf(f, "0\r\n");
        return;
    }

    lockRead(container);

    if (NULL == dbObjectParse(member, &memberId))
    {
        unlockRead(container);
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    if (!setIsMember(container, memberId))
    {
        unlockRead(container);
        fprintf(f, "0\r\n");
        return;
    }

    unlockRead(container);
    fprintf(f, "1\r\n");
}

void renameCommand(FILE *f, int argc, sds *argv)
{
    sds oldSetName = NULL, newSetName = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (oldSetName = argv[0]) || 0 == strlen(oldSetName) ||
        NULL == (newSetName = argv[1]) || 0 == strlen(newSetName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == dbGet(oldSetName))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (NULL != dbGet(newSetName))
    {
        fprintf(f, "Set with desired new name already exists.\r\n");
        return;
    }

    if (0 != dbRename(oldSetName, newSetName))
    {
        fprintf(f, "ERROR.\r\n");
        return;
    }

    fprintf(f, "OK.\r\n");
}

void randsetCommand(FILE *f, int argc, sds *argv)
{
    sds randSetName = NULL;
    set *randSet = NULL;

    if (NULL == f)
        return;

    if (NULL != (randSetName = dbGetRandSet()))
    {
        fprintf(f, "%s\r\n", randSetName);
        randSet = (set *) dbGet(randSetName); 
        lockRead(randSet);
        setPrint(randSet, f, 1);
        unlockRead(randSet);
        fprintf(f, "\r\n");
    }
    else
    {
        fprintf(f, "No any sets in db.\r\n");
    }
}

void randCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;
    const set *targetSet = NULL;
    valType randMemberId = 0;
    const dbObject *targetObject = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (targetSet = dbGet(setName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    lockRead(targetSet);
    if (0 != setGetRand(targetSet, &randMemberId))
    {
        unlockRead(targetSet);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    if (NULL == (targetObject = dbGetObject(randMemberId, 1)))
    {
        unlockRead(targetSet);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    if (0 != dbObjectPrint(targetObject, f, 1))
    {
        unlockRead(targetSet);
        fprintf(f, "ERROR.\r\n");
    }

    fprintf(f, "\r\n");

    unlockRead(targetSet);
}

void evalCommand(FILE *f, int argc, sds *argv)
{
    sds expr = NULL;
    size_t pos = 0;
    dbObject *result = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (expr = argv[0]) || 0 == strlen(expr))
    {
        fprintf(f, "Bad expression.\r\n");
        return;
    }

    if (NULL == (result = eval(expr, &pos)))
    {
        fprintf(f, "ERROR.\r\n");
        return;
    }

    if (0 != dbObjectPrint(result, f, 1))
    {
        fprintf(f, "ERROR.\r\n");
        return;
    }

    fprintf(f, "\r\n");
}

void addCommand(FILE *f, int argc,  sds *argv)
{
    sds setName = NULL, member = NULL;
    valType memberId = 0;
    set *container = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (member = argv[1]) || 0 == strlen(member))
    {
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    if (NULL == (container = (set *) dbGet(setName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (NULL == dbObjectParse(member, &memberId))
    {
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    lockWrite(container);
    if (-1 == setAdd(container, memberId))
    {
        unlockWrite(container);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    unlockWrite(container);
    fprintf(f, "OK.\r\n");
}

void remCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL, member = NULL;
    valType memberId = 0;
    set *container = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (member = argv[1]) || 0 == strlen(member))
    {
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    if (NULL == (container = (set *) dbGet(setName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (NULL == dbObjectParse(member, &memberId))
    {
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    lockWrite(container);
    if (-1 == setRemove(container, memberId))
    {
        unlockWrite(container);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    unlockWrite(container);
    fprintf(f, "OK.\r\n");
}

void cardCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;
    dbObject *targetSet = NULL;
    size_t pos = 0;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (targetSet = eval(setName, &pos)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (objectSet != targetSet->objectType)
    {
        fprintf(f, "Not a set result.\r\n");
        return;
    }

    fprintf(f, "%u\r\n", setCard(targetSet->objectPtr.setPtr));
}

void movCommand(FILE *f, int argc, sds *argv)
{
    sds fromSetName = NULL, toSetName = NULL, member = NULL;
    set *fromSet = NULL, *toSet = NULL;
    valType memberId = 0;

    if (NULL == f || NULL == argv)
        return;

    if (3 != argc)
    {
        fprintf(f, "Expected 3 arguments.\r\n");
        return;
    }

    if (NULL == (fromSetName = argv[0]) || 0 == strlen(fromSetName) ||
        NULL == (toSetName = argv[1]) || 0 == strlen(toSetName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (member = argv[2]) || 0 == strlen(member))
    {
        fprintf(f, "Bad member.\r\n");
        return;
    }

    if (NULL == (fromSet = (set *) dbGet(fromSetName)) ||
        NULL == (toSet = (set *) dbGet(toSetName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (NULL == dbObjectParse(member, &memberId))
    {
        fprintf(f, "Bad set member.\r\n");
        return;
    }

    lockWrite(fromSet);
    if (!setIsMember(fromSet, memberId))
    {
        unlockWrite(fromSet);
        fprintf(f, "From set doesn't contain member.\r\n");
        return;
    }

    lockWrite(toSet);
    if (-1 == setAdd(toSet, memberId) ||
        -1 == setRemove(fromSet, memberId))
    {
        unlockWrite(fromSet);
        unlockWrite(toSet);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    unlockWrite(fromSet);
    unlockWrite(toSet);
    fprintf(f, "OK.\r\n");
}

void popCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;
    set *targetSet = NULL;
    valType randMemberId = 0;
    const dbObject *targetObject = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (targetSet = (set *) dbGet(setName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    lockWrite(targetSet);
    if (0 != setGetRand(targetSet, &randMemberId))
    {
        unlockWrite(targetSet);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    if (NULL == (targetObject = dbGetObject(randMemberId, 1)))
    {
        unlockWrite(targetSet);
        fprintf(f, "ERROR.\r\n");
        return;
    }

    if (0 != dbObjectPrint(targetObject, f, 1))
    {
        unlockWrite(targetSet);
        fprintf(f, "ERROR.\r\n");
    }

    fprintf(f, "\r\n");

    setRemove(targetSet, randMemberId);

    unlockWrite(targetSet);
}

void lockCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;
    set *targetSet = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (targetSet = (set *) dbGet(setName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    lockWrite(targetSet);
    fprintf(f, "OK.\r\n");
}

void unlockCommand(FILE *f, int argc, sds *argv)
{
    sds setName = NULL;
    set *targetSet = NULL;

    if (NULL == f || NULL == argv)
        return;

    if (1 != argc)
    {
        fprintf(f, "Expected 1 argument.\r\n");
        return;
    }

    if (NULL == (setName = argv[0]) || 0 == strlen(setName))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (targetSet = (set *) dbGet(setName)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    unlockWrite(targetSet);
    fprintf(f, "OK.\r\n");
}

void pingCommand(FILE *f, int argc, sds *argv)
{
    if (NULL == f)
        return;

    fprintf(f, "PONG.\r\n");
}

void flushallCommand(FILE *f, int argc, sds *argv)
{
    if (NULL == f)
        return;

    if (0 != dbFlushAll())
    {
        fprintf(f, "ERROR.\r\n");
    }

    fprintf(f, "OK.\r\n");
}

void gcCommand(FILE *f, int argc, sds *argv)
{
    valType result = 0;

    if (NULL == f)
        return;

    result = dbGC();
    fprintf(f, "Collected %u.\r\n", result);
}

void truncCommand(FILE *f, int argc, sds *argv)
{
    valType freed = 0;

    if (NULL == f)
        return;

    freed = dbSetTrunc();
    fprintf(f, "Freed %u.\r\n", freed);
}

void indexCommand(FILE *f, int argc, sds *argv)
{
    if (NULL == f)
        return;

    dbPrintIndex(f);
}

void setsCommand(FILE *f, int argc, sds *argv)
{
    if (NULL == f)
        return;

    dbPrintSets(f);
}

void eqCommand(FILE *f, int argc, sds *argv)
{
    sds setNameA = NULL, setNameB = NULL;
    dbObject *setA = NULL, *setB = NULL;
    size_t posA = 0, posB = 0;
    int result = 0;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (setNameA = argv[0]) || 0 == strlen(setNameA) ||
        NULL == (setNameB = argv[1]) || 0 == strlen(setNameB))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (setA = eval(setNameA, &posA)) ||
        NULL == (setB = eval(setNameB, &posB)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (objectSet != setA->objectType)
    {
        fprintf(f, "Expected set result.\r\n");
        dbUnregisterObject(setA->id);
    }

    if (objectSet != setB->objectType)
    {
        fprintf(f, "Expected set result.\r\n");
        dbUnregisterObject(setA->id);
    }

    if (!(objectSet == setA->objectType &&
          objectSet == setB->objectType))
    {
        return;
    }

    result = setCmpE(setA->objectPtr.setPtr, setB->objectPtr.setPtr);
    switch (result)
    {
        case -1:
            fprintf(f, "ERROR.\r\n");
            return;

        case 0:
            fprintf(f, "0\r\n");
            return;

        case 1:
            fprintf(f, "1\r\n");
            return;
    }
}

void subeCommand(FILE *f, int argc, sds *argv)
{
    sds setNameA = NULL, setNameB = NULL;
    dbObject *setA = NULL, *setB = NULL;
    size_t posA = 0, posB = 0;
    int result = 0;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (setNameA = argv[0]) || 0 == strlen(setNameA) ||
        NULL == (setNameB = argv[1]) || 0 == strlen(setNameB))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (setA = eval(setNameA, &posA)) ||
        NULL == (setB = eval(setNameB, &posB)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (objectSet != setA->objectType)
    {
        fprintf(f, "Expected set result.\r\n");
        dbUnregisterObject(setA->id);
    }

    if (objectSet != setB->objectType)
    {
        fprintf(f, "Expected set result.\r\n");
        dbUnregisterObject(setA->id);
    }

    if (!(objectSet == setA->objectType &&
          objectSet == setB->objectType))
    {
        return;
    }

    result = setCmpSubsetOrEq(setA->objectPtr.setPtr, setB->objectPtr.setPtr);
    switch (result)
    {
        case -1:
            fprintf(f, "ERROR.\r\n");
            return;

        case 0:
            fprintf(f, "0\r\n");
            return;

        case 1:
            fprintf(f, "1\r\n");
            return;
    }
}

void subCommand(FILE *f, int argc, sds *argv)
{
        sds setNameA = NULL, setNameB = NULL;
    dbObject *setA = NULL, *setB = NULL;
    size_t posA = 0, posB = 0;
    int result = 0;

    if (NULL == f || NULL == argv)
        return;

    if (2 != argc)
    {
        fprintf(f, "Expected 2 arguments.\r\n");
        return;
    }

    if (NULL == (setNameA = argv[0]) || 0 == strlen(setNameA) ||
        NULL == (setNameB = argv[1]) || 0 == strlen(setNameB))
    {
        fprintf(f, "Bad set name.\r\n");
        return;
    }

    if (NULL == (setA = eval(setNameA, &posA)) ||
        NULL == (setB = eval(setNameB, &posB)))
    {
        fprintf(f, "Set doesn't exist.\r\n");
        return;
    }

    if (objectSet != setA->objectType)
    {
        fprintf(f, "Expected set result.\r\n");
        dbUnregisterObject(setA->id);
    }

    if (objectSet != setB->objectType)
    {
        fprintf(f, "Expected set result.\r\n");
        dbUnregisterObject(setA->id);
    }

    if (!(objectSet == setA->objectType &&
          objectSet == setB->objectType))
    {
        return;
    }

    result = setCmpSubset(setA->objectPtr.setPtr, setB->objectPtr.setPtr);
    switch (result)
    {
        case -1:
            fprintf(f, "ERROR.\r\n");
            return;

        case 0:
            fprintf(f, "0\r\n");
            return;

        case 1:
            fprintf(f, "1\r\n");
            return;
    }
}
