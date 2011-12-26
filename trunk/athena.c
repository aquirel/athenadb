// athena.c - Main.

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <signal.h>

#include <WinSock2.h>
#include <MSWSock.h>

#include "adlist.h"

#include "athena.h"
#include "dbengine.h"
#include "syncengine.h"
#include "command.h"

// Global server variables.
static server s;

void sigtermHandler(int sig)
{
    s.working = 0;
    shutdown(s.listenSocket, SD_BOTH);
}

void clientThread(void *param)
{
    FD_SET socketSet;
    struct timeval t;
    client *c = (client *) param;

    if (NULL == c || NULL == c->rf || NULL == c->wf)
        return;

    fprintf(c->wf, "Hello.\n");

    t.tv_sec = CLIENT_TIMEOUT;
    t.tv_usec = 0;

    while (s.working)
    {
        FD_ZERO(&socketSet);
        FD_SET(c->socket, &socketSet);
        if (0 == select(0, &socketSet, NULL, NULL, &t))
        {
            fprintf(c->wf, "Bye.\r\n");
            break;
        }

        fgets(c->queryBuf, QUERY_BUF_SIZE, c->rf);

        if (0 != strlen(c->queryBuf))
        {
            int res = 0;

            if ('\n' == c->queryBuf[strlen(c->queryBuf) - 1])
                c->queryBuf[strlen(c->queryBuf) - 1] = '\0';

            if (0 == strlen(c->queryBuf))
                continue;

            // -1 - quit, -2 - shutdown.
            res = commandExecutor(c, c->queryBuf);

            if (-1 == res)
            {
                break;
            }
            else if (-2 == res)
            {
                s.working = 0;
                closesocket(s.listenSocket);
                break;
            }
        }
        else
        {
            break;
        }
    }

    // Send client disconnected message.
    {
        struct sockaddr_in addr;
        int addrLen = sizeof(addr);
        if (SOCKET_ERROR != getpeername(c->socket, (struct sockaddr *) &addr, &addrLen))
        {
            printf("Client %s disconnected.\n", inet_ntoa(addr.sin_addr));
        }
        else
        {
            printf("Client disconnected.\n");
        }
    }

    shutdown(c->socket, SD_BOTH);
    closesocket(c->socket);
    //_close(c->fd);
    //fclose(c->rf);
    //fclose(c->wf);

    free(c);
    lockWrite(s.clients);
    listDelNode(s.clients, listSearchKey(s.clients, c));
    unlockWrite(s.clients);
}

int main(int argc, char *argv[])
{
    WSADATA wsaData;
    struct sockaddr_in athenaBindAddress;
    struct timeval selectTimeOut;
    selectTimeOut.tv_sec = 1;
    selectTimeOut.tv_usec = 0;

    s.listenSocket = INVALID_SOCKET;

    srand((unsigned) (time(NULL) ^ _getpid()));

    printf("Starting athena server.\n");
    if (0 != initSyncEngine())
    {
        perror("Failed to init sync engine.\n");
        return -1;
    }

    if (0 != initDbEngine())
    {
        cleanupSyncEngine();
        perror("Failed to init db engine.\n");
        return -1;
    }

    signal(SIGTERM, sigtermHandler);

    if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        perror("WSAStartup() failed.\n");
        cleanupDbEngine();
        cleanupSyncEngine();
        return -1;
    }

    //if (INVALID_SOCKET == (listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
    if (INVALID_SOCKET == (s.listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0)))
    {
        perror("Couldn't create listen socket.\n");
        cleanupDbEngine();
        cleanupSyncEngine();
        WSACleanup();
        return -1;
    }

    {
        DWORD dontLinger = 0;

        if (SOCKET_ERROR == setsockopt(s.listenSocket, SOL_SOCKET, SO_DONTLINGER, (const char *) &dontLinger, sizeof(DWORD)))
        {
            perror("Couldn't create listen socket.\n");
            cleanupDbEngine();
            cleanupSyncEngine();
            WSACleanup();
            return -1;
        }
    }

    FD_ZERO(&s.listenSet);
    FD_SET(s.listenSocket, &s.listenSet);

    athenaBindAddress.sin_family = AF_INET;
    athenaBindAddress.sin_addr.s_addr = argc >= 2 ? inet_addr(argv[1]) : htonl(INADDR_ANY);
    athenaBindAddress.sin_port = htons(argc >= 3 ? atoi(argv[2]) : (unsigned short) 3307);

    if (0 != bind(s.listenSocket, (struct sockaddr *) &athenaBindAddress, sizeof(athenaBindAddress)))
    {
        perror("bind() failed.\n");
        cleanupDbEngine();
        cleanupSyncEngine();
        closesocket(s.listenSocket);
        WSACleanup();
        return -1;
    }

    if (0 != listen(s.listenSocket, SOMAXCONN))
    {
        perror("listen() failed.\n");
        cleanupDbEngine();
        cleanupSyncEngine();
        closesocket(s.listenSocket);
        WSACleanup();
        return -1;
    }

    if (NULL == (s.clients = listCreate()))
    {
        perror("clients listCreate() failed.\n");
        cleanupDbEngine();
        cleanupSyncEngine();
        closesocket(s.listenSocket);
        WSACleanup();
        return -1;
    }

    if (0 != registerSyncObject(s.clients))
    {
        perror("Failed to init sync engine.\n");
        listRelease(s.clients);
        cleanupDbEngine();
        cleanupSyncEngine();
        closesocket(s.listenSocket);
        WSACleanup();
        return -1;
    }

    printf("Ready to accept clients.\n");
    s.working = 1;

    _beginthread(dbPrintStatus, 0, (void *) &s);

    while (s.working)
    {
        struct sockaddr_in newClientAddress;
        int newClientAddressSize = sizeof(newClientAddress);
        SOCKET newClientSocket = INVALID_SOCKET;
        int selected = 0;

        FD_SET(s.listenSocket, &s.listenSet);

        selected = select(0, &s.listenSet, NULL, &s.listenSet, &selectTimeOut);

        if (!s.working)
            break;

        if (!selected)
            continue;

        newClientSocket = accept(s.listenSocket, (struct sockaddr *) &newClientAddress, &newClientAddressSize);

        if (INVALID_SOCKET != newClientSocket)
        {
            client *newClient;
            printf("New client connected from %s\n", inet_ntoa(newClientAddress.sin_addr));

            if (NULL == (newClient = (client *) calloc(1, sizeof(client))))
            {
                perror("Failed to create client object.\n");
                closesocket(newClientSocket);
                continue;
            }

            newClient->socket = newClientSocket;

            if (-1 == (newClient->fd = _open_osfhandle(newClientSocket, _O_TEXT)))
            {
                perror("Failed to create client object.\n");
                closesocket(newClient->socket);
                free(newClient);
                continue;
            }

            if (NULL == (newClient->rf = _fdopen(newClient->fd, "rt")))
            {
                perror("Failed to create client object.\n");
                _close(newClient->fd);
                closesocket(newClient->socket);
                free(newClient);
                continue;
            }

            if (NULL == (newClient->wf = _fdopen(newClient->fd, "wt")))
            {
                perror("Failed to create client object.\n");
                fclose(newClient->rf);
                _close(newClient->fd);
                closesocket(newClient->socket);
                free(newClient);
                continue;
            }

            setvbuf(newClient->rf, NULL, _IONBF, 0);
            setvbuf(newClient->wf, NULL, _IONBF, 0);

            if (NULL == (newClient->queryBuf = sdsnewlen(NULL, QUERY_BUF_SIZE)))
            {
                perror("Failed to create client object.\n");
                _close(newClient->fd);
                fclose(newClient->rf);
                fclose(newClient->wf);
                closesocket(newClient->socket);
                free(newClient);
                continue;
            }

            lockWrite(s.clients);
            if (NULL == listAddNodeTail(s.clients, newClient))
            {
                unlockWrite(s.clients);
                perror("Failed to create client object.\n");
                _close(newClient->fd);
                fclose(newClient->rf);
                fclose(newClient->wf);
                closesocket(newClient->socket);
                free(newClient);
                continue;
            }
            unlockWrite(s.clients);

            // Run client thread.
            if (-1L == (newClient->tid = _beginthread(clientThread, 0, newClient)))
            {
                perror("Failed to client thread.\n");
                _close(newClient->fd);
                fclose(newClient->rf);
                fclose(newClient->wf);
                closesocket(newClient->socket);
                free(newClient);
                continue;
            }
        }
    }

    printf("Athena server ending. Waiting for all clients to terminate.\n");

    // Unlock all.

    // Wait for all clients to terminate.
    lockRead(s.clients);
    while (listLength(s.clients))
    {
        HANDLE tid = (HANDLE) ((client *) s.clients->head->value)->tid;
        unlockRead(s.clients);
        WaitForSingleObject(tid, INFINITE);
    }
    unlockRead(s.clients);

    listRelease(s.clients);

    WSACleanup();
    cleanupDbEngine();
    cleanupSyncEngine();
    printf("Athena server end.\n");
    return 0;
}
