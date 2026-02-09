#include <stdio.h>
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"

#define ARRAYLEN(A) (sizeof(A)/sizeof(A[0]))
#define MAXLEN 100
#define ECHOLEN 4
#define TYPELEN 4


static char *builtin_cmds[]={"echo", "type", "exit"};

static bool check_cmd(char *input, char *cmd){
  size_t len= strlen(cmd);
  for(int i=0; i<len; i++){
    if(input[i] != cmd[i]) return false;
  }
  return true;
}

static bool is_builtin(char *cmd){
  for(unsigned int i=0; i<ARRAYLEN(builtin_cmds); i++){
        if(strcmp(builtin_cmds[i], &cmd[TYPELEN + 1])==0) return true;
  }
  return false;
}

int main() {
  // Flush after every printf
  setbuf(stdout, NULL);

  
  char input[MAXLEN];
  while(true){
    printf("$ ");
  
    fgets(input, sizeof(input), stdin);
  
    //fgets also saves the newline character added, when user presses enter, so we want to remove that newline character.
    //strcspn searches for the first occurance in a string of any of the specified characters and returns the length of the string up to that point.
    input[strcspn(input, "\n")]= '\0';

    //termination
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
        if(i==ARRAYLEN(builtin_cmds)) printf("%s: not found\n", &input[TYPELEN + 1]);
      }
    }else{
      printf("%s: command not found\n", input);
    }
  }
  return EXIT_SUCCESS;
} 