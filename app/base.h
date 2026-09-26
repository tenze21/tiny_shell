#ifndef BASE_H
#define BASE_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARRAYLEN(A) (sizeof(A) / sizeof(A[0]))
#define MAXINPUTLEN 100
#define MAXCMDLEN 20
#define ECHOLEN 4
#define TYPELEN 4
#define MAXPATHLEN 1024
#define MAXPIPES 16
#define MAXARGSLEN 50
#define MAXARGS 20

static const char *builtin_cmds[] = {"echo", "type", "exit", "pwd", "cd", NULL};
static const char *redirect_ops[] = {">", ">>", "1>>", "1>", "2>", "2>>", NULL};

typedef struct command
{
    char *argv[MAXARGS];
    int argc;
    char *redirect_out;
    char *redirect_err;
    bool out_append;
    bool err_append;
} command_t;

typedef struct pipeline {
    command_t cmds[MAXPIPES];
    int ncmds;
} pipeline_t;

#endif