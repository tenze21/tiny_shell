#ifndef COMMANDS_H
#define COMMANDS_H 

/**
 * @brief print whatever follows it to the standard output.
 * @param inp the string to print
 * @returns 0 on success, -1 on failure
*/
void echo(char *inp[]);

/**
 * @brief checks if a command is a shell builtin, if command isn't a shell builtin searches the computers PATH environment and prints the path to the executable. 
 * @param cmd the command to check
 * @returns 0 on success, -1 on failure
*/
void type(const char *cmd);

/**
 * @brief prints the current working directory
 * @returns 0 on success, -1 on failure
*/
void pwd(void);

/**
 * @brief change working directory to `dir`
 * @param dir the target directory to switch to 
 * @returns 0 on success, -1 on failure
*/
void cd(char *dir);

/**
 * @brief list shell history
*/
void history(char *arg);

/**
 * @brief execute external commands 
 * @param cmd command to execute
 * @returns 0 on success, -1 on failure
*/
void exec_external(char *cmd, char *args[]);

#endif
