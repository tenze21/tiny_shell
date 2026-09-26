#include "utils.h"

#define MAXPATHLEN 1024
#define PATH_SEPERATOR ":"

bool is_shell_builtin(const char *cmd)
{
  int idx = 0;
  const char *builtin;
  while ((builtin = builtin_cmds[idx++]) != NULL)
  {
    if (strcmp(builtin, cmd) == 0)
      return true;
  }
  return false;
}

char *find_in_path(const char *cmd){
  if(cmd==NULL || cmd[0]=='\0')return NULL;
  char *path_env=getenv("PATH");
  if(!path_env) return NULL;

  char *path_cpy=strdup(path_env);
  char *dir= strtok(path_cpy, PATH_SEPERATOR);
  while(dir!=NULL){
      char full_path[MAXPATHLEN];
      snprintf(full_path, MAXPATHLEN, "%s/%s", dir, cmd);
      if(access(full_path, F_OK | X_OK)==0){
        free(path_cpy);
        return strdup(full_path);
      }
      dir=strtok(NULL, PATH_SEPERATOR);
  }
  free(path_cpy);
  return NULL;
}

void trim(char *str){
    unsigned int len=strlen(str);
    if(str[len-1]==' ') str[len-1]='\0';
    if(str[0]==' '){
        for(int i=1; i<len; i++)
            str[i-1]=str[i];
        str[len-1]='\0';
    }
}

/*
 * @dev tokenize input string into seperate command arguments.
 *
 * writes each command token to argv.
 */
int tokenize_cmd(const char *input, char *argv[])
{
    int argc = 0;
    char buf[MAXARGSLEN];
    int bufp = 0;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool in_token = false;

    memset(buf, 0, sizeof(buf));
    for (const char *p = input; *p != '\0' && argc < MAXARGS; p++)
    {
        if (in_single_quote)
        {
            if (*p == '\'')
            {
                in_single_quote = false;
            }
            else
            {
                if (bufp < MAXARGSLEN - 1)
                    buf[bufp++] = *p;
            }
        }
        else if (in_double_quote)
        {
            if (*p == '"')
            {
                in_double_quote = false;
            }
            else
            {
                if (bufp < MAXARGSLEN - 1)
                {
                    if (*p == '\\')
                        buf[bufp++] = *++p;
                    else
                        buf[bufp++] = *p;
                }
            }
        }
        else
        {
            if (*p == '\'')
            {
                in_single_quote = true;
                in_token = true;
            }
            else if (*p == '"')
            {
                in_double_quote = true;
                in_token = true;
            }
            else if (*p == ' ')
            {
                if (in_token)
                {
                    buf[bufp] = '\0';
                    argv[argc++] = strdup(buf);
                    bufp = 0;
                    in_token = false;
                    memset(buf, 0, sizeof(buf));
                }
            }
            else
            {
                in_token = true;
                if (bufp < MAXARGSLEN - 1)
                {
                    if (*p == '\\')
                    {
                        buf[bufp++] = *++p;
                    }
                    else
                    {
                        buf[bufp++] = *p;
                    }
                }
            }
        }
    }

    if (in_token || bufp > 0)
    {
        buf[bufp] = '\0';
        argv[argc++] = strdup(buf);
    }
    argv[argc] = NULL;
    return argc;
}
