#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>

#define ARRAYLEN(A) (sizeof(A)/sizeof(A[0]))
#define MAXINPUTLEN 100
#define MAXCMDLEN 20
#define MAXARGSLEN 50
#define MAXARGS 20
#define ECHOLEN 4
#define TYPELEN 4
#define MAXPATHLEN 1024
#define PATH_SEPERATOR ":"

static char *builtin_cmds[]={"echo", "type", "exit", "pwd", "cd"};

/*
 * Compare the input command with `cmd`
 */
static bool cmp_cmd(const char *input,const char *cmd){
    if(input==NULL || input[0]=='\0') return false;
    for(int i=0; input[i]!='\0' && input[i]!=' '; i++){
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
static char *find_in_path(const char *cmd){
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

static void change_dir(const char *path){
    if(chdir(path)!=0){
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    }
}

/*
 * remove heading and trailing white spaces.
 */
static void trim(char *str){
    unsigned int len=strlen(str);
    if(str[len-1]==' ') str[len-1]='\0';
    if(str[0]==' '){
        for(int i=1; i<len; i++)
            str[i-1]=str[i];
        str[len-1]='\0';
    }
}

/*
 * parse arguments, take all characters in quotes as normal characters and space split arguments
 * input: argument string entered in shell
 * argv[]: destination where parsed args are to be written
 * max_args: maximum number of arguments
 * returns the number of arguments
 * @dev elements in argv[] need to be freed
 */
static int parse_args(const char *input, char *argv[], int max_args){
    int argc=0;
    char buf[MAXARGSLEN];
    int bufp=0;
    bool in_single_quote=false;
    bool in_double_quote=false;
    bool in_token=false;
    
    memset(buf, 0, sizeof(buf));
    for(const char *p=input; *p!='\0' && argc<max_args; p++){
        if(in_single_quote){
            if(*p=='\''){
                in_single_quote=false;
            }else{
                if(bufp<MAXARGSLEN - 1)
                    buf[bufp++]=*p;
            }
        }else if(in_double_quote){
            if(*p=='"'){
                in_double_quote=false;
            }else{
                if(bufp<MAXARGSLEN - 1){
                    if(*p=='\\')
                        buf[bufp++]=*++p;
                    else
                        buf[bufp++]=*p;
                }
            }
        }else{
            if(*p=='\''){
                in_single_quote=true;
                in_token=true;
            }else if(*p=='"'){
                in_double_quote=true;
                in_token=true;
            }else if(*p==' '){
                if(in_token){
                    buf[bufp]='\0';
                    argv[argc++]=strdup(buf);
                    bufp=0;
                    in_token=false;
                    memset(buf, 0, sizeof(buf));
                }
            }else{
                in_token=true;
                if(bufp<MAXARGSLEN - 1){
                    if(*p=='\\'){
                        buf[bufp++]=*++p;
                    }else{
                        buf[bufp++]=*p;
                    }
                }
            }
        }
    }
    
    if(in_token || bufp>0){
        buf[bufp]='\0';
        argv[argc++]=strdup(buf);
    }
    return argc;
}

/*
 * parse external commands, important for cases where the program is supplied quoted executables.
 * input: whole command string passed to the program
 * cmd: pointer to where the parsed executable is to be written
 * max_len: maximum length of a executable
 */
static void parse_external_cmd(const char *input, char *cmd, const int max_len){
    bool in_single_quote=false;
    bool in_double_quote=false;
    bool in_token=false;
    int ci=0;
   for(;*input!='\0' && ci<max_len-1;input++){
       if(in_single_quote){
           if(*input=='\'')
               break;
           else
                cmd[ci++]=*input;
       }else if(in_double_quote){
           if(*input=='"'){
               break;
           }else{
                if(*input=='\\')
                    cmd[ci++]=*++input;
                else
                    cmd[ci++]=*input;
           }
       }else{
           if(*input=='\''){
               in_single_quote=true;
               in_token=true;
           }else if(*input=='"'){
               in_double_quote=true;
               in_token=true;
           }else if(*input==' '){
               if(in_token)
                   break;
           }else{
               in_token=true;
               if(*input=='\\')
                   cmd[ci++]=*++input;
               else
                   cmd[ci++]=*input;
                   
           }
       }
   }
}

static void free_args(char *argv[], const size_t argc){
    for(size_t i=0; i<argc; i++){
         if(argv[i]!=NULL){
             free(argv[i]);
         }       
    }
}

int main() {
  // Flush after every printf
  setbuf(stdout, NULL);

  char input[MAXINPUTLEN]={'\0'};
  char *argv[MAXARGS];
  while(true){
    printf("$ ");
  
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")]= '\0';
    trim(input);
    int argc=parse_args(&input[strcspn(input, " ") + 1], argv, MAXARGS);
    if(cmp_cmd(input, "exit"))
    {
        break;
    }
    else if(cmp_cmd(input, "echo"))
    {
        for(int i=0; i<argc; i++){
            if(i>0) printf(" ");
            printf("%s", argv[i]);
        }
        printf("\n");
    }
    else if(cmp_cmd(input, "type"))
    {
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
    }
    else if(cmp_cmd(input, "pwd"))
    {
        char current_working_directory[MAXPATHLEN];
        if(getcwd(current_working_directory, sizeof(current_working_directory))!=NULL){
            printf("%s\n", current_working_directory);
        }else{
            perror("error: failed to get current working directory.\n");
        }
    }
    else if(cmp_cmd(input, "cd"))
    {
        char new_dir[MAXPATHLEN];
        snprintf(new_dir, MAXPATHLEN, "%s", &input[3]);
        if(strcmp(new_dir, "~")==0){
            char *path_to_home=getenv("HOME");
            change_dir(path_to_home);
        }else{
            change_dir(new_dir);
        }
    }
    else
    {
        // Get the command
        char cmd[MAXCMDLEN]={'\0'};
        parse_external_cmd(input, cmd, MAXCMDLEN);
        
        char *exec_args[MAXARGS + 2];
        exec_args[0]=cmd;
        for(int i=0; i<argc; i++)
            exec_args[i+1]=argv[i];
        exec_args[argc+1]=NULL;
        
        // Check if command in PATH(is executable)
        char *path_to_cmd=find_in_path(cmd);
        
        if(path_to_cmd != NULL)//execute the system command
        {
            pid_t id=fork();
            if(id==-1){
                perror("error: fork failed");
                return EXIT_FAILURE;
            }else if(id==0){
                execvp(cmd, exec_args);
                printf("error: failed to execute %s\n", path_to_cmd);
                _exit(EXIT_FAILURE);
            }else{
                wait(NULL);
            }
        }
        else
        {
            printf("%s: command not found\n", cmd);
        }
        free(path_to_cmd);
    }
    free_args(argv, argc);
  }
  return EXIT_SUCCESS;
} 