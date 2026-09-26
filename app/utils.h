#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>
#include "base.h"

/**
 * @brief check if `cmd` is shell builtin(i.e., present in `builtins[]`)
 */
bool is_shell_builtin(const char *cmd);

/**
 * @brief Search for `cmd` in the PATH environment variable.
 * @param cmd the cmd to look for in PATH
 * @returns path to `cmd` executable
 */
char *find_in_path(const char *cmd);

/**
 * @brief remove leading and trailing white spaces
 * @param str string to trim
 */
void trim(char *str);

/**
 * @brief splits input into tokens by space, supports escaping with `\`. word in quotations are regarded as one token
 * @param input string to tokenize
 * @param argv string array to write tokens to 
 */
int tokenize_cmd(const char *input, char *argv[]);

#endif