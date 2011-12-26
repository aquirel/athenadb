// dbobject.h - Database object management.

#ifndef __DBOBJECT_H__
#define __DBOBJECT_H__

#include <stdio.h>

#include "adlist.h"

#include "athena.h"
#include "set.h"

// Database object description.
typedef enum dbObjectType
{
    objectSet, objectTuple, objectVal
} dbObjectType;

typedef struct dbObject
{
    dbObjectType objectType;

    union objectPtr
    {
        set *setPtr;
        list *tuplePtr;
        valType val;
    } objectPtr;

    valType id;
} dbObject;

// Returns 1 if a equals b, 0 otherwise, -1 on error.
int dbObjectCompare(const dbObject *a, const dbObject *b);

// Returns -1 on error.
int dbObjectPrint(const dbObject *obj, FILE *f, int lock);

void dbObjectRelease(dbObject *obj);

// Parses integer value from string s and registers it in object index.
// Returns NULL on error.
dbObject *valParse(const char *tokenPtr, valType *id);

// Parses db object value (tuple, set, value) from string s and registers it in object index.
// Returns NULL on error.
dbObject *dbObjectParse(const sds s, valType *id);

#endif /* __DBOBJECT_H__ */
