// eval.h - Set expression evaluation.

#ifndef __EVAL_H__
#define __EVAL_H__

#include "sds.h"

#include "set.h"

// Return NULL on error.
dbObject *eval(const sds s, size_t *pos);

#endif /* __EVAL_H__ */
