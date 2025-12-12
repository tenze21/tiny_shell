#include <stdio.h>
#include "string.h"
#include "stdbool.h"

#define MAXLEN 100
#define ECHOLEN 4

static bool check_cmd(char *input, char *cmd){
  size_t len= strlen(cmd);
  for(int i=0; i<len; i++){
    if(input[i] != cmd[i]) return false;
  }
  return true;
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

    // echo implementation
    if(check_cmd(input, "echo")) printf("%s\n", &input[ECHOLEN + 1]);//print only the string after echo and a space.
    else printf("%s: command not found\n", input);
  }
  return 0;
} 