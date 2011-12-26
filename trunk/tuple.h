// tuple.h - Tuple utils.

#ifndef __TUPLE_H__
#define __TUPLE_H__

#include <stdlib.h>

#include "adlist.h"
#include "sds.h"

#include "dbobject.h"
#include "set.h"

// Parses tuple from string s and registers it in object index.
// Returns NULL on error.
dbObject *tupleParse(const sds s, size_t *pos, valType *id);

// Compares given tuples. Returns 1 on eq, 0 on not eq, -1 on error.
int tupleCmp(list *a, list *b);

// Returns flattened list.
set *tupleFlatten(list *t, int lock);

// Returns -1 on error.
int tuplePrint(list *tuple, FILE *f, int lock);

#endif /* __TUPLE_H__ */
