#include "redirects.h"

int apply_redirects(command_t *cmd)
{
    if (cmd->redirect_out != NULL)
    {
        int flags = O_CREAT | O_WRONLY | (cmd->out_append ? O_APPEND : O_TRUNC);
        int fd_out = open(cmd->redirect_out, flags, 0644);
        if (fd_out < 0)
        {
            perror("error: couldn't open file");
            return -1;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    if (cmd->redirect_err != NULL)
    {
        int flags = O_CREAT | O_WRONLY | (cmd->err_append ? O_APPEND : O_TRUNC);
        int fd_err = open(cmd->redirect_err, flags, 0644);
        if (fd_err < 0)
        {
            perror("error: couldn't open file");
            return -1;
        }
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
    }
    return 0;
}

void restore_redirects(int saved_out, int saved_err)
{
    if (saved_out >= 0)
    {
        dup2(saved_out, STDOUT_FILENO);
        close(saved_out);
    }
    if (saved_err >= 0)
    {
        dup2(saved_err, STDERR_FILENO);
        close(saved_err);
    }
}

bool is_redirect_op(char *token)
{
    int builtin_idx = 0;
    const char *op;
    while ((op = redirect_ops[builtin_idx++]) != NULL)
    {
        if (strcmp(token, op) == 0)
            return true;
    }
    return false;
}