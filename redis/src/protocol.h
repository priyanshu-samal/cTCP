#ifndef PROTOCOL_H
#define PROTOCOL_H

#define COMMAND_MAX_ARGS 3
#define COMMAND_MAX_LENGTH 4096

typedef struct {
    int argc;
    char *argv[COMMAND_MAX_ARGS];
} Command;

int parse_command(char *line, Command *cmd);

#endif