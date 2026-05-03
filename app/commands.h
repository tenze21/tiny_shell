#ifndef COMMANDS_H
#define COMMANDS_H 

/**
 * @brief print whatever follows it to the standard output.
 * @param inp the string to print
 * @returns 0 on success, -1 on failure
*/
int echo(char *inp[]);

/**
 * @brief checks if a command is a shell builtin, if command isn't a shell builtin searches the computers PATH environment and prints the path to the executable. 
 * @param cmd the command to check
 * @returns 0 on success, -1 on failure
*/
int type(const char *cmd, const char *builtin_cmds[]);

/**
 * @brief prints the current working directory
 * @returns 0 on success, -1 on failure
*/
int pwd(void);

/**
 * @brief change working directory to `dir`
 * @param dir the target directory to switch to 
 * @returns 0 on success, -1 on failure
*/
int cd(char *dir);

/**
 * @brief execute external commands 
 * @param cmd command to execute
 * @returns 0 on success, -1 on failure
*/
int exec(char *cmd, char *args[]);

#endif
