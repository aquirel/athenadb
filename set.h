// set.h - Set API and implementation.

#ifndef __SET_H__
#define __SET_H__

#include "athena.h"

// Set.
typedef struct set
{
    char *data;
    valType length; // In bytes.
    valType card;
    int registered;
} set;

// Set iterator.
typedef struct setIterator
{
    const set *s;
    valType i, val;
} setIterator;

// Public API.
// Creates new set. Returns set pointer or null on error.
set *setCreate(void);
// Creates copy of set s. Returns pointer to new set or null on error.
set *setCopy(const set *s);
// Destroys set.
void setDestroy(set *s);

// Adds element to the set. Returns 0 on ok, 1 if element already exists, -1 on error.
int setAdd(set *s, valType val);
// Removes element from the set. Returns 0 on ok, 1 if element wasn't present, -1 on error.
int setRemove(set *s, valType val);

// Returns set cardinality.
valType setCard(const set *s);
// Gets random element from the set. Returns 0 on ok, -1 on error.
int setGetRand(const set *s, valType *val);
// Returns 1 if val is in set, 0 otherwise.
int setIsMember(const set *s, valType val);

// Returns set a - b or null on error.
set *setDiff(const set *a, const set *b);
// Returns set (a - b) + (b - a) or null on error.
set *setSymDiff(const set *a, const set *b);
// Returns set a * b or null on error.
set *setInter(const set *a, const set *b);
// Returns set a + b or null on error.
set *setUnion(const set *a, const set *b);
// Returns set A x B or null on error.
set *setCartProd(const set *a, const set *b);
// Returns boolean set of a or null on error.
set *setBoolean(const set *a);
// Returns flattened set.
set *setFlatten(const set *s, int lock);

// Returns 1 if a = b or -1 on error.
int setCmpE(const set *a, const set *b);
// Returns 1 if a is subset of b (A c B) or -1 on error.
int setCmpSubset(const set *a, const set *b);
// Returns 1 if a is subset of b (A c B) or a = b. Returns -1 on error
int setCmpSubsetOrEq(const set *a, const set *b);

// Returns number of bytes freed.
unsigned setTrunc(set *s);

// Gets set iterator pointing to the first set element. *iter is set to null if set is empty. Return 0 on ok or -1 on error.
int setGetIter(const set *s, setIterator **iter);
// Destroys set iterator.
void setDestroyIter(setIterator *iter);
// Gets next element from the set pointed by iter. Returns 0 on ok or -1 on error.
int setGetNext(setIterator *iter);

// Private API.
// Returns 1 if s can hold val, 0 otherwise.
int setCanHold(set *s, valType val);
// Grows s to make it able to hold val. Returns -1 on error.
static int setGrow(set *s, valType val);
// Sets bit number "bit" in s to val. Returns val or -1 on error.
static int setSetBit(set *s, valType bit, unsigned val);
// Gets bit number "bit" in s. Returns bit value or -1 on error.
static int setGetBit(const set *s, valType bit);
// Count set bits in byte;
__inline static int byteBitCount(unsigned byte);

#endif /* __SET_H__ */
