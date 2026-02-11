#include <stdio.h>
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "unistd.h"

#define ARRAYLEN(A) (sizeof(A)/sizeof(A[0]))
#define MAXINPUTLEN 100
#define ECHOLEN 4
#define TYPELEN 4
#define MAXPATHLEN 100

#ifdef _WIN32
  #define PATH_SEPERATOR ";"
  #define SLASH "\\"
#else
  #define PATH_SEPERATOR ":"
  #define SLASH "/"
#endif


static char *builtin_cmds[]={"echo", "type", "exit"};

static bool check_cmd(char *input, char *cmd){
  size_t len= strlen(cmd);
  for(int i=0; i<len; i++){
    if(input[i] != cmd[i]) return false;
  }
  return true;
}

static char *find_in_path(char *cmd){
  char *path_env=getenv("PATH");
  if(!path_env) return NULL;
  
  #ifdef _WIN32
  char *path_ext=getenv("PATHEXT");/*file extension for windows executables*/
  if(!path_ext) path_ext=".COM;.EXE;.BAT;.CMD";
  char *ext_cpy=strdup(path_ext);
  #endif
  
  char *path_cpy=strdup(path_env);
  char *dir= strtok(path_cpy, PATH_SEPERATOR);
  #ifndef _WIN32
    while(dir!=NULL){
      char full_path[MAXPATHLEN];
      snprintf(full_path, MAXPATHLEN, "%s/%s", dir, cmd);
      if(access(full_path, F_OK | X_OK)==0){
        free(path_cpy);
        return strdup(full_path);
      }
      dir=strtok(NULL, PATH_SEPERATOR);
    }
  #else
      while(dir!=NULL){
        char full_path[MAXPATHLEN];
        snprintf(full_path, MAXPATHLEN, "%s\\%s", dir, cmd);
        // Try the command as-is first (might already have extension)
        if(access(full_path, F_OK)){
          free(path_cpy);
          free(ext_cpy);
          return strdup(full_path);
        }

        char *ext=strtok(ext_cpy, PATH_SEPERATOR);
        while(ext!=NULL){
          snprintf(full_path, MAXPATHLEN, "%s\\%s%s", dir, cmd, ext);
          if(access(full_path, F_OK)==0){
            free(path_cpy);
            free(ext_cpy);
            return strdup(full_path);
          }
          ext=strtok(NULL, PATH_SEPERATOR);
        }
        /*reset ext_cpy*/
        free(ext_cpy);
        ext_cpy=strdup(path_ext);

        dir=strtok(NULL, PATH_SEPERATOR);
      }
  #endif
  
  free(path_cpy);
  #ifdef _WIN32
    free(ext_cpy);
  #endif
  return NULL;
}

int main() {
  // Flush after every printf
  setbuf(stdout, NULL);

  
  char input[MAXINPUTLEN];
  while(true){
    printf("$ ");
  
    fgets(input, sizeof(input), stdin);
  
    input[strcspn(input, "\n")]= '\0';

    if(check_cmd(input, "exit")) break;
    else if(check_cmd(input, "echo")) 
      printf("%s\n", &input[ECHOLEN + 1]);//print only the string after echo and a space.
    else if(check_cmd(input, "type")){
      unsigned int i=0;
      while(i<ARRAYLEN(builtin_cmds)){
        if(strcmp(builtin_cmds[i], &input[TYPELEN + 1])==0){
          printf("%s is a shell builtin\n", builtin_cmds[i]);
          break;
        }
        i++;
        if(i==ARRAYLEN(builtin_cmds)){
          char *path_to_cmd= find_in_path(&input[TYPELEN + 1]);
          if(path_to_cmd==NULL)
            printf("%s: not found\n", &input[TYPELEN + 1]);
          else{
            printf("%s is %s\n", &input[TYPELEN + 1], path_to_cmd);
            free(path_to_cmd);
          }
        }

      }
    }else{
      printf("%s: command not found\n", input);
    }
  }
    
  return EXIT_SUCCESS;
} 