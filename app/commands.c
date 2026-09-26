#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <readline/history.h>

#include "commands.h"
#include "utils.h"
#include "base.h"

#define MAXPATHLEN 1024

void echo(char *inp[])
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
  _exit(EXIT_SUCCESS);
}

void type(const char *cmd)
{
  if (is_shell_builtin(cmd))
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
  _exit(EXIT_SUCCESS);
}

void pwd(void)
{
  char current_working_directory[MAXPATHLEN];
  if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL)
  {
    printf("%s\n", current_working_directory);
  }
  else
  {
    perror("error: failed to get current working directory.\n");
    _exit(EXIT_FAILURE);
  }
  _exit(EXIT_SUCCESS);
}

void cd(char *dir)
{
  char new_dir[MAXPATHLEN];
  snprintf(new_dir, MAXPATHLEN, "%s", dir);
  if (strcmp(new_dir, "~") == 0)
  {
    char *path_to_home = getenv("HOME");
    if (chdir(path_to_home) != 0)
    {
      fprintf(stderr, "cd: %s: %s\n", path_to_home, strerror(errno));
    }
  }
  else
  {
    if (chdir(new_dir) != 0)
    {
      fprintf(stderr, "cd: %s: %s\n", new_dir, strerror(errno));
    }
  }
}

void history(char *arg)
{
    HIST_ENTRY **hist_list = history_list();
    int limit = 0;

    if (arg != NULL)
    {
        if (strcmp(arg, "-c") == 0)
        {
          clear_history();
        }
        else if (atoi(arg) > 0)
        {
            limit = atoi(arg);
        }
    }

    if (hist_list != NULL)
    {
        int total = 0;
        while (hist_list[total] != NULL) total++;   // count entries first

        int start = (limit > 0 && limit < total) ? total - limit : 0;

        for (int i = start; i < total; ++i)
            printf("%5d  %s\n", i + history_base, hist_list[i]->line);
    }
}

void exec_external(char *path_to_cmd, char *args[])
{
  execv(path_to_cmd, args);
  fprintf(stderr, "error: failed to execute %s\n", path_to_cmd);
  _exit(EXIT_FAILURE);
}