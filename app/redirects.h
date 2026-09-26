#ifndef REDIRECTS_H
#define REDIRECTS_H
#include "base.h"
#include <fcntl.h>

int apply_redirects(command_t *cmd);
void restore_redirects(int saved_out, int saved_err);
bool is_redirect_op(char *token);
#endif