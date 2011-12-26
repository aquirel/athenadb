// stack.h - Stack implementation.

#ifndef __STACK_H__
#define __STACK_H__

typedef struct stack
{
    void **data;
    size_t sp, size;
} stack;

// Creates new stack. Return null on error.
stack *stackCreate(void);
void stackDestroy(stack *s);

// Return -1 on error.
int stackPush(stack *s, void *val);

// Return -1 on error.
int stackPop(stack *s, void **val);

// Return -1 on error.
int stackPeek(stack *s, void **val);

// Returns -1 on error. i is index from top. stackLookup(, 0, ) means peek.
int stackLookup(stack *s, unsigned i, void **val);

// Returns -1 on error, 0 on ok.
int stackReplaceTop(stack *s, void *val);

size_t stackSize(const stack *s);

#endif /* __STACK_H__ */
