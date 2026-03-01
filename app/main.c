#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>

#define ARRAYLEN(A) (sizeof(A)/sizeof(A[0]))
#define MAXINPUTLEN 100
#define MAXCMDLEN 20
#define ECHOLEN 4
#define TYPELEN 4
#define MAXPATHLEN 1024
#define PATH_SEPERATOR ":"

static char *builtin_cmds[]={"echo", "type", "exit", "pwd", "cd"};

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

static void change_dir(char *path){
    if(chdir(path)!=0){
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    }
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
    }else if(cmp_cmd(input, "pwd")){
        char current_working_directory[MAXPATHLEN];
        if(getcwd(current_working_directory, sizeof(current_working_directory))!=NULL){
            printf("%s\n", current_working_directory);
        }else{
            perror("error: failed to get current working directory.\n");
        }
    }else if(cmp_cmd(input, "cd")){
        char new_dir[MAXPATHLEN];
        snprintf(new_dir, MAXPATHLEN, "%s", &input[3]);
        if(strcmp(new_dir, "~")==0){
            char *path_to_home=getenv("HOME");
            change_dir(path_to_home);
        }else{
            change_dir(new_dir);
        }
    }else{
        // Get the command
        char cmd[MAXCMDLEN];
        size_t input_cmd_len=strcspn(input, " ");
        strncpy(cmd, input, input_cmd_len);
        cmd[input_cmd_len]='\0';
        
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
                execvp(cmd, args);
                printf("Failed to execute %s\n", path_to_cmd);//only reached on failure
                _exit(EXIT_FAILURE);
            }else{
                wait(NULL);
            }
        }else{
            printf("%s: command not found\n", cmd);
        }
        free(path_to_cmd);
    }
  }
  return EXIT_SUCCESS;
} 