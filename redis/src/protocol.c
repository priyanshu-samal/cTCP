#include "protocol.h"

#include <string.h>

int parse_command(char *line, Command *cmd)
{
    cmd->argc = 0;

    char *token = strtok(line, " \t\r\n");

    while (token != NULL)
    {
        if (cmd->argc >= COMMAND_MAX_ARGS)
        {
            cmd->argc = 0;
            return -1;
        }

        cmd->argv[cmd->argc++] = token;

        token = strtok(NULL, " \t\r\n");
    }

    if (cmd->argc == 0)
        return -1;

    return 0;
}