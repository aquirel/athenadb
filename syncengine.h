// syncengine.h - synchronization engine for shared objects.

#ifndef __SYNCENGINE_H__
#define __SYNCENGINE_H__

int initSyncEngine(void);
void cleanupSyncEngine(void);

int registerSyncObject(const void *object);
void unregisterSyncObject(const void *object);
int syncObjectIsRegistered(const void *object);

void lockRead(const void *object);
void lockWrite(const void *object);
void unlockRead(const void *object);
void unlockWrite(const void *object);

#endif /* __SYNCENGINE_H__ */
