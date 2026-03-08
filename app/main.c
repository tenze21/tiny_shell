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
#include <fcntl.h>

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
static int parse_cmd(const char *input, char *argv[], int max_args){
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
  unsigned int is_redirect= false;
  char input[MAXINPUTLEN]={'\0'};
  char *argv[MAXARGS];
  while(true){
    is_redirect=false;
    printf("$ ");
  
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")]= '\0';
    trim(input);
    int argc=parse_cmd(input, argv, MAXARGS);
    
    for(int i=1; i<argc; i++){
        if(strcmp(argv[i], ">")==0 || strcmp(argv[i], "1>")==0){
            is_redirect=true;
            break;
        } 
    }
    
    int saved_stdout=-1;
    if(is_redirect){
        /* open the destination file */
        int fd=open(argv[argc - 1], O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if(fd<0){
            perror("error: couldn't open file");
            return EXIT_FAILURE;
        }
        /*save original STDOUT */
        saved_stdout=dup(STDOUT_FILENO);
        /* make file descriptor 1(STDOUT) point to destination file */
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    if(strcmp(argv[0], "exit")==0)
    {
        break;
    }
    else if(strcmp(argv[0], "echo")==0)
    {
        if(is_redirect){/* if STDOUT is redirected */
            printf("%s", argv[1]);
        }else{
            for(int i=1; i<argc; i++){
                if(i>1) printf(" ");
                printf("%s", argv[i]);
            }
            printf("\n");
        }
    }
    else if(strcmp(argv[0], "type")==0)
    {
      unsigned int i=0;
      while(i<ARRAYLEN(builtin_cmds)){
        // Check if command is a builtin shell command
        if(strcmp(builtin_cmds[i], argv[1])==0){
          printf("%s is a shell builtin\n", builtin_cmds[i]);
          break;
        }
        i++;
        if(i==ARRAYLEN(builtin_cmds)){//check if command is a system executable
            char *path_to_cmd= find_in_path(argv[1]);
            if(path_to_cmd==NULL)
            printf("%s: not found\n", argv[1]);
            else{
                printf("%s is %s\n", argv[1], path_to_cmd);
                free(path_to_cmd);
            }
        }
      }
    }
    else if(strcmp(argv[0], "pwd")==0)
    {
        char current_working_directory[MAXPATHLEN];
        if(getcwd(current_working_directory, sizeof(current_working_directory))!=NULL){
            printf("%s\n", current_working_directory);
        }else{
            perror("error: failed to get current working directory.\n");
        }
    }
    else if(strcmp(argv[0], "cd")==0)
    {
        char new_dir[MAXPATHLEN];
        snprintf(new_dir, MAXPATHLEN, "%s", argv[1]);
        if(strcmp(new_dir, "~")==0){
            char *path_to_home=getenv("HOME");
            change_dir(path_to_home);
        }else{
            change_dir(new_dir);
        }
    }
    else
    {
        char *exec_args[MAXARGS + 2];
        exec_args[0]=argv[0];
        int i;
        for(i=1; i<argc && strcmp(argv[i], ">")!=0 && strcmp(argv[i], "1>")!=0; i++){
            exec_args[i]=argv[i];
        }
        exec_args[i]=NULL;
        
        // Check if command in PATH(is executable)
        char *path_to_cmd=find_in_path(argv[0]);
        
        if(path_to_cmd != NULL)//execute the system command
        {
            pid_t id=fork();
            if(id==-1){
                perror("error: fork failed");
                return EXIT_FAILURE;
            }else if(id==0){
                execvp(argv[0], exec_args);
                printf("error: failed to execute %s\n", path_to_cmd);
                fflush(stdout);
                _exit(EXIT_FAILURE);
            }else{
                wait(NULL);
            }
        }
        else
        {
            printf("%s: command not found\n", argv[0]);
        }
        free(path_to_cmd);
    }
    free_args(argv, argc);
    
    if(is_redirect){/* restore STDOUT to terminal */
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
  }
  return EXIT_SUCCESS;
} 