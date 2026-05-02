#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define ARRAYLEN(A) (sizeof(A)/sizeof(A[0]))
#define MAXINPUTLEN 100
#define MAXCMDLEN 20
#define MAXARGSLEN 50
#define MAXARGS 20
#define ECHOLEN 4
#define TYPELEN 4
#define MAXPATHLEN 1024
#define PATH_SEPERATOR ":"

static const char *builtin_cmds[]={"echo", "type", "exit", "pwd", "cd", NULL};
static const char *redirect_ops[]={">", ">>", "1>>", "1>", "2>", "2>>", NULL};

typedef struct{
    char *argv[MAXARGS];
    int argc;
    char *redirect_out;
    char *redirect_err;
    bool out_append;
    bool err_append;
}command_t;

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

static bool is_shell_builtin(char *cmd){
    int idx=0;
    const char *builtin;
    while((builtin= builtin_cmds[idx++]) != NULL){
      // Check if command is a builtin shell command
      if(strcmp(builtin, cmd)==0){
          return true;
      }
    }
    return false;
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
 * @dev tokenize input string into seperate command arguments.
 *
 * writes each command token to argv.
 */
static int tokenize_cmd(const char *input, char *argv[]){
    int argc=0;
    char buf[MAXARGSLEN];
    int bufp=0;
    bool in_single_quote=false;
    bool in_double_quote=false;
    bool in_token=false;

    memset(buf, 0, sizeof(buf));
    for(const char *p=input; *p!='\0' && argc<MAXARGS; p++){
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
    argv[argc]=NULL;
    return argc;
}

static bool is_redirect_op(char *token){
    int idx=0;
    const char *op;
    while((op= redirect_ops[idx++]) != NULL){
        if(strcmp(token, op)==0) return true;
    }
    return false;
}

/*
 * @dev parse tokenized input string into the `command_t` structure.
 */
static int parse_cmd(const char *input, command_t *cmd){
    char *argv[MAXARGS];
    int argc= tokenize_cmd(input, argv);

    cmd->argc=0;
    cmd->out_append=false;
    cmd->err_append=false;
    cmd->redirect_out=NULL;
    cmd->redirect_err=NULL;

    for(int i=0; i<argc; i++){
        bool is_redirect_out= strcmp(argv[i], ">") ==0 || strcmp(argv[i], "1>") ==0;
        bool is_redirect_err= strcmp(argv[i], "2>") ==0;
        bool is_out_append= strcmp(argv[i], ">>") ==0 || strcmp(argv[i], "1>>") ==0;
        bool is_err_append=strcmp(argv[i], "2>>")==0;

        if(is_redirect_out || is_redirect_err || is_err_append || is_out_append){
            if(i+1>=argc){/*No output file*/
                fprintf(stderr, "tShell: Syntax error near unexpected token 'newline'\n");
                return -1;
            }

            if(is_redirect_op(argv[i+1])){/*consecutive redirects*/
                fprintf(stderr, "tShell: Syntax error near unexpected token '%s'\n", argv[i+1]);
                return -1;
            }

            cmd->out_append=is_out_append;
            cmd->err_append=is_err_append;

            if(is_redirect_out || is_out_append)
                cmd->redirect_out=argv[i+1];
            if(is_redirect_err || is_err_append)
                cmd->redirect_err=argv[i+1];
            i++;
        }else{
            cmd->argv[cmd->argc++]=argv[i];
        }
    }
    cmd->argv[cmd->argc]=NULL;
    return 0;
}

static int apply_redirects(command_t *cmd, int *saved_stdout, int *saved_stderr){
    if(cmd->redirect_out!=NULL){
        int flags=O_CREAT | O_WRONLY | (cmd->out_append? O_APPEND : O_TRUNC);
        int fd_out=open(cmd->redirect_out, flags, 0644);
        if(fd_out<0){
            perror("error: couldn't open file");
            return -1;
        }
        *saved_stdout=dup(STDOUT_FILENO);
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    if(cmd->redirect_err!=NULL){
        int flags=O_CREAT | O_WRONLY | (cmd->err_append? O_APPEND : O_TRUNC);
        int fd_err=open(cmd->redirect_err, flags, 0644);
        if(fd_err<0){
            perror("error: couldn't open file");
            return -1;
        }
        *saved_stderr=dup(STDERR_FILENO);
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
    }
    return 0;
}

static void restore_redirects(int saved_out, int saved_err){
    if(saved_out>=0){
        dup2(saved_out, STDOUT_FILENO);
        close(saved_out);
    }
    if(saved_err>=0){
        dup2(saved_err, STDERR_FILENO);
        close(saved_err);
    }
}

static void free_args(char *argv[], const size_t argc){
    for(size_t i=0; i<argc; i++){
         if(argv[i]!=NULL){
             free(argv[i]);
         }
    }
}

/*
 * READLINE UTILITIES
*/

/**
 * @dev Readline calls this repeatedly until it returns NULL, when user presses TAB.
 * @param text - The partial word the user has typed so far.
 * @param state - 0 on first call, increments each subsequent call. used by readline to determine if it's a fresh call.
 */
static char *command_generator(const char *text, int state){
    static int idx;
    static int len;
    const char *name;
    
    // state=0 implies it's a fresh completion call, reset the index
    if(state==0){
        idx=0;
        len=strlen(text);
    }
    
    // Walk through `builtin_cmds` and return the next match
    while((name=builtin_cmds[idx++]) != NULL){
        // Check if name starts wih whatever the user typed
        if(strncmp(name, text, len) == 0)
            return strdup(name);
    }
    
    return NULL;
}

/*
 * @dev readline calls this function first, this allows you to do context aware completion by passing different `command_generator` functions depending on the first word.
 */
static char **command_completion(const char *text, int start, int end){
    // start = 0, means we are completing the first word, the command itself.
    // start > 0, means we are completing an argument, this can be handles differently depending on the command that was passed.
    if(start==0)
        return rl_completion_matches(text, command_generator);
    
    return NULL;
}

int main(void) {
    rl_readline_name= "tinyShell";
    
    // Wire up completion function
    rl_attempted_completion_function= command_completion;
    char *line;
    while((line = readline("$ ")) != NULL){
        
        if(*line!='\0') add_history(line);
        
        command_t cmd={0};
    
        if(parse_cmd(line, &cmd)<0) continue;
    
        int saved_stdout=-1;
        int saved_stderr=-1;
        if(apply_redirects(&cmd, &saved_stdout, &saved_stderr)<0) continue;
    
        if(strcmp(cmd.argv[0], "exit")==0)
        {
            break;
        }
        else if(strcmp(cmd.argv[0], "echo")==0)
        {
            for(int i=1; i<cmd.argc; i++){
                if(i>1) printf(" ");
                printf("%s", cmd.argv[i]);
            }
            printf("\n");
        }
        else if(strcmp(cmd.argv[0], "type")==0)
        {
            if(is_shell_builtin(cmd.argv[1])){
                printf("%s is a shell builtin\n", cmd.argv[1]);
            }else{
                char *path_to_cmd= find_in_path(cmd.argv[1]);
                if(path_to_cmd==NULL)
                    printf("%s: not found\n", cmd.argv[1]);
                else{
                    printf("%s is %s\n", cmd.argv[1], path_to_cmd);
                    free(path_to_cmd);
                }
            }
        }
        else if(strcmp(cmd.argv[0], "pwd")==0)
        {
            char current_working_directory[MAXPATHLEN];
            if(getcwd(current_working_directory, sizeof(current_working_directory))!=NULL){
                printf("%s\n", current_working_directory);
            }else{
                perror("error: failed to get current working directory.\n");
            }
        }
        else if(strcmp(cmd.argv[0], "cd")==0)
        {
            char new_dir[MAXPATHLEN];
            snprintf(new_dir, MAXPATHLEN, "%s",cmd.argv[1]);
            if(strcmp(new_dir, "~")==0){
                char *path_to_home=getenv("HOME");
                change_dir(path_to_home);
            }else{
                change_dir(new_dir);
            }
        }
        else
        {
            char *path_to_cmd=find_in_path(cmd.argv[0]);
    
            if(path_to_cmd != NULL)//execute the system command
            {
                pid_t id=fork();
                if(id==-1){
                    perror("error: fork failed");
                    return EXIT_FAILURE;
                }else if(id==0){
                    execv(path_to_cmd, cmd.argv);
                    fprintf(stderr, "error: failed to execute %s\n", path_to_cmd);
                    fflush(stdout);
                    _exit(EXIT_FAILURE);
                }else{
                    wait(NULL);
                }
            }
            else
            {
                fprintf(stderr, "%s: command not found\n", cmd.argv[0]);
            }
            free(path_to_cmd);
        }
        free_args(cmd.argv, cmd.argc);
    
        restore_redirects(saved_stdout, saved_stderr);
    }
    return EXIT_SUCCESS;
}
