#ifndef UTILS_H
#define UTILS_H

/**
 * @brief check if `cmd` is shell builtin(i.e., present in `builtins[]`)
 */
bool is_shell_builtin(const char *cmd, const char *builtins[]);

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

#endif