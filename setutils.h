// setutils.h - Set utility functions.

#ifndef __SETUTILS_H__
#define __SETUTILS_H__

#include <stdio.h>

#include "sds.h"

#include "dbobject.h"

// Parses set from string s and registers it in object index.
// Returns NULL on error.
dbObject *setParse(sds s, size_t *pos, valType *id);

// Returns -1 on error.
int setPrint(set *s, FILE *f, int lock);

#endif /* __SETUTILS_H__ */
