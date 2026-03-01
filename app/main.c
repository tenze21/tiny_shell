#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdint.h"
#include "sys/wait.h"

#define ARRAYLEN(A) (sizeof(A)/sizeof(A[0]))
#define MAXINPUTLEN 100
#define MAXCMDLEN 15
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

/*
 * Compare the input command with `cmd`
 */
static bool cmp_cmd(char *input, char *cmd){
  size_t len= strlen(cmd);
  for(int i=0; i<len; i++){
    if(input[i] != cmd[i]) return false;
  }
  return true;
}

/*
 * A simple function to count the number of arguments
 */
static unsigned int count_args(const char *str){
    unsigned int count=1;
    while(*str){
        if(*str==' ') count++;
        str++; 
    }
    return count;
}

/*
 * Look for the provided command in the PATH enviroment variable.
 * returns a pointer to a string containing the full path to the 
 * executable or NULL if command isn't in PATH. 
 */
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

    if(cmp_cmd(input, "exit")) 
        break;
    else if(cmp_cmd(input, "echo")) 
        printf("%s\n", &input[ECHOLEN + 1]);//print only the string after echo and a space.
    else if(cmp_cmd(input, "type")){
      unsigned int i=0;
      while(i<ARRAYLEN(builtin_cmds)){
        // Check if command is a builtin shell command
        if(strcmp(builtin_cmds[i], &input[TYPELEN + 1])==0){
          printf("%s is a shell builtin\n", builtin_cmds[i]);
          break;
        }
        i++;
        if(i==ARRAYLEN(builtin_cmds)){//check if command is a system executable
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
        // Get the command
        char cmd[MAXCMDLEN];
        size_t input_cmd_len=strcspn(input, " ");
        strncpy(cmd, input, input_cmd_len);
        
        // Check if command in PATH(is executable)
        char *path_to_cmd=find_in_path(cmd);
        
        if(path_to_cmd != NULL){//execute the system command
            // Get number of arguments from the input string and store the arguments in a string array(array of pointers to characters)
            unsigned int args_count=count_args(input);
            char *args[args_count + 1];
            char *token=strtok(input, " ");
            for(int i=0; i<args_count && token!=NULL; i++){
                args[i]=token;
                token=strtok(NULL, " ");
            }
            args[args_count]=NULL;
            
            // Create a child process to execute the system command
            int id=fork();
            if(id==-1){
                perror("error: fork failed");
                return EXIT_FAILURE;
            }else if(id==0){
                execvp(args[0], args);
                printf("Failed to execute %s\n", path_to_cmd);//only reached on failure
                _exit(EXIT_FAILURE);
            }else{
                wait(NULL);
            }
        }else{
            printf("%s: command not found\n", cmd);
        }
    }
  }
  return EXIT_SUCCESS;
} 