#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "commands.h"
#include "utils.h"

#define MAXPATHLEN 1024

int echo(char *inp[])
{
  char *word;
  int idx = 0;
  while ((word = inp[idx++]) != NULL)
  {
    if (idx > 1)
      printf(" ");

    printf("%s", word);
  }
  printf("\n");
  return 0;
}

int type(const char *cmd, const char *builtin_cmds[])
{
  if (is_shell_builtin(cmd, builtin_cmds))
  {
    printf("%s is a shell builtin\n", cmd);
  }
  else
  {
    char *path_to_cmd = find_in_path(cmd);
    if (path_to_cmd == NULL)
      printf("%s: not found\n", cmd);
    else
    {
      printf("%s is %s\n", cmd, path_to_cmd);
      free(path_to_cmd);
    }
  }
  return 0;
}

int pwd(void)
{
  char current_working_directory[MAXPATHLEN];
  if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL)
  {
    printf("%s\n", current_working_directory);
  }
  else
  {
    perror("error: failed to get current working directory.\n");
    return -1;
  }
  return 0;
}

int cd(char *dir)
{
  char new_dir[MAXPATHLEN];
  snprintf(new_dir, MAXPATHLEN, "%s", dir);
  if (strcmp(new_dir, "~") == 0)
  {
    char *path_to_home = getenv("HOME");
    if (chdir(path_to_home) != 0)
    {
      fprintf(stderr, "cd: %s: %s\n", path_to_home, strerror(errno));
      return -1;
    }
  }
  else
  {
    if (chdir(new_dir) != 0)
    {
      fprintf(stderr, "cd: %s: %s\n", new_dir, strerror(errno));
      return -1;
    }
  }
  return 0;
}

int exec(char *cmd, char *args[])
{
  char *path_to_cmd = find_in_path(cmd);

  if (path_to_cmd != NULL) // execute the system command
  {
    pid_t id = fork();
    if (id == -1)
    {
      perror("error: fork failed");
      return EXIT_FAILURE;
    }
    else if (id == 0)
    {
      execv(path_to_cmd, args);
      fprintf(stderr, "error: failed to execute %s\n", path_to_cmd);
      fflush(stdout);
      _exit(EXIT_FAILURE);
    }
    else
    {
      wait(NULL);
    }
  }
  else
  {
    fprintf(stderr, "%s: command not found\n", cmd);
    return -1;
  }
  free(path_to_cmd);
  return 0;
}