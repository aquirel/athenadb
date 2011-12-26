// command.c - Command definitions and execution.

#include "athena.h"
#include "command.h"

static struct athenaCommand commandList[] =
    {
        { "set", 2, setCommand, ' ' },
        { "del", 1, delCommand, ' ' },
        { "exists", 1, existsCommand, ' ' },
        { "contains", 2, containsCommand, ' ' },
        { "rename", 2, renameCommand, ' ' },
        { "randset", 0, randsetCommand, ' ' },
        { "rand", 1, randCommand, ' ' },
        { "eval", 1, evalCommand, ' ' },
        { "add", 2, addCommand, ' ' },
        { "rem", 2, remCommand, ' ' },
        { "card", 1, cardCommand, ' ' },
        { "mov", 3, movCommand, ' ' },
        { "pop", 1, popCommand, ' ' },
        { "lock", 1, lockCommand, ' ' },
        { "unlock", 1, unlockCommand, ' ' },
        { "ping", 0, pingCommand, ' ' },
        { "quit", 0, NULL, ' ' },
        { "shutdown", 0, NULL, ' ' },
        { "flushall", 0, flushallCommand, ' ' },
        { "gc", 0, gcCommand, ' ' },
        { "trunc", 0, truncCommand, ' ' },
        { "index", 0, indexCommand, ' ' },
        { "sets", 0, setsCommand, ' ' },
        { "eq", 2, eqCommand, '.' },
        { "sube", 2, subeCommand, '.' },
        { "sub", 2, subCommand, '.' }
    };

int commandExecutor(client *c, const sds query)
{
    size_t i;
    sds command = NULL;
    char *p = NULL;
    athenaCommand *cmdDescription = NULL;

    if (NULL == c || NULL == c->wf || NULL == query || 0 == strlen(query))
        return 0;

    if (NULL != (p = strchr(query, ' ')))
    {
        command = sdsnewlen(query, p - query);
    }
    else
    {
        command = sdsnew(query);
    }

    for (i = 0; i < sizeof(commandList) / sizeof(athenaCommand); i++)
        if (0 == strcmp(command, commandList[i].name))
        {
            cmdDescription = &commandList[i];
            break;
        }

    if (NULL != cmdDescription)
    {
        if (NULL != commandList[i].proc)
        {
            size_t argc = commandList[i].arity;
            sds *argv = NULL;

            if (0 == argc)
            {
                commandList[i].proc(c->wf, argc, argv);
                return 0;
            }
            else
            {
                char *n = NULL;
                size_t argsCounter = 0;
                char delim = commandList[i].argsDelim;
                argv = (sds *) calloc(argc, sizeof(sds));

                if (NULL != p)
                {
                    while (isspace(*p))
                        p++;

                    do
                    {
                        if (NULL == (n = strchr(p, delim)) || argsCounter == argc - 1)
                        {
                            argv[argsCounter] = sdsnew(p);
                        }
                        else
                        {
                            argv[argsCounter] = sdsnewlen(p, n - p);
                        }

                        p = n + 1;
                        argsCounter++;
                    }
                    while (argsCounter < argc && NULL != n);
                }

                while (argsCounter < argc)
                {
                    argv[argsCounter++] = NULL;
                }

                if (argsCounter == argc)
                {
                    commandList[i].proc(c->wf, argc, argv);
                }

                for (i = 0; i < argsCounter; i++)
                {
                    if (NULL != argv[i])
                        sdsfree(argv[i]);
                }
                free(argv);

                return 0;
            }
        }
        else
        {
            if (0 == strcmp("quit", command))
            {
                fprintf(c->wf, "Bye.\r\n");
                return -1;
            }
            else if (0 == strcmp("shutdown", command))
            {
                fprintf(c->wf, "Bye.\r\n");
                return -2;
            }
        }
    }

    fprintf(c->wf, "Unknown command.\r\n");
    sdsfree(command);
    return 0;
}
