// dbengine.h - Database engine.

#ifndef __DBENGINE_H__
#define __DBENGINE_H__

#include "adlist.h"
#include "sds.h"

#include "athena.h"
#include "set.h"
#include "dbobject.h"

// Returns: 0 on ok, -1 on error.
int initDbEngine(void);

void cleanupDbEngine(void);

// Returns pointer to set or NULL.
const set *dbGet(const sds setName);

// Returns pointer to set name or NULL.
const sds dbGetRandSet(void);

// Returns -1 on error.
int dbSet(const sds setName, const set *s);

// Returns DICT_OK or DICT_ERR.
int dbRemove(const sds setName);

// Return -1 on error.
int dbFlushAll(void);

// Returns -1 on error.
int dbRename(const sds oldSetName, const sds newSetName);

// Returns pointer to new set or NULL.
const set *dbCreate(const sds setName);

// Returns NULL on error.
const dbObject *dbGetObject(valType id, int lock);

// Returns 0 on ok, -1 on error.
int dbRegisterObject(dbObject **object, valType *id);

// Returns 0 on ok, -1 on error.
int dbUnregisterObject(valType id);

// Returns 1 if object is found, 0 otherwise.
int dbFindObject(const dbObject *object, valType *index, int lock);

// Retuns 1 if set is found, 0 otherwise.
int dbFindSet(const set *s, valType *index, int lock);

// Retutns number of collected objects.
valType dbGC(void);

// Returns bytes freed.
valType dbSetTrunc(void);

void dbPrintIndex(FILE *f);
void dbPrintSets(FILE *f);

void dbPrintStatus(void *serverPtr);

#endif /* __DBENGINE_H__ */
