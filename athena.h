// athena.h - common db routines.

#ifndef __ATHENA_H__
#define __ATHENA_H__

#include <WinSock2.h>

#include <stdio.h>

#include "adlist.h"
#include "sds.h"

// Some common definitions.
#define va_copy(d, s) ((d) = (s))
typedef size_t valType;

#define QUERY_BUF_SIZE 512
#define BG_STATUS_SLEEP 10000
#define CLIENT_TIMEOUT 30

typedef struct client
{
    SOCKET socket;
    int fd;
    FILE *rf, *wf;
    sds queryBuf;
    uintptr_t tid;
} client;

typedef struct server
{
    int working;
    list *clients;
    SOCKET listenSocket;
    FD_SET listenSet;
} server;

// Command prototypes.
typedef void (*athenaCommandProc)(FILE *f, int argc, sds *argv);
typedef struct athenaCommand
{
    char *name;
    size_t arity;
    athenaCommandProc proc;
    char argsDelim;
} athenaCommand;

int commandExecutor(client *c, const sds query);

void setCommand(FILE *f, int argc, sds *argv);
void delCommand(FILE *f, int argc, sds *argv);
void existsCommand(FILE *f, int argc, sds *argv);
void containsCommand(FILE *f, int argc, sds *argv);
void renameCommand(FILE *f, int argc, sds *argv);
void randsetCommand(FILE *f, int argc, sds *argv);
void randCommand(FILE *f, int argc, sds *argv);
void evalCommand(FILE *f, int argc, sds *argv);
void addCommand(FILE *f, int argc,  sds *argv);
void remCommand(FILE *f, int argc, sds *argv);
void cardCommand(FILE *f, int argc, sds *argv);
void movCommand(FILE *f, int argc, sds *argv);
void popCommand(FILE *f, int argc, sds *argv);
void lockCommand(FILE *f, int argc, sds *argv);
void unlockCommand(FILE *f, int argc, sds *argv);
void pingCommand(FILE *f, int argc, sds *argv);
void quitCommand(FILE *f, int argc, sds *argv); // Handled separately.
void shutdownCommand(FILE *f, int argc, sds *argv); // Handled separately.
void flushallCommand(FILE *f, int argc, sds *argv);
void gcCommand(FILE *f, int argc, sds *argv);
void truncCommand(FILE *f, int argc, sds *argv);
void indexCommand(FILE *f, int argc, sds *argv);
void setsCommand(FILE *f, int argc, sds *argv);
void eqCommand(FILE *f, int argc, sds *argv);
void subeCommand(FILE *f, int argc, sds *argv);
void subCommand(FILE *f, int argc, sds *argv);

void eqCommand(FILE *f, int argc, sds *argv);
void subeCommand(FILE *f, int argc, sds *argv);
void subCommand(FILE *f, int argc, sds *argv);

#endif __ATHENA_H__
