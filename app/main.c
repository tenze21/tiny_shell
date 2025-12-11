#include <stdio.h>
#include "string.h"
#include "stdbool.h"

#define MAXLEN 100

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

    if(strcmp(input, "exit")==0) break;
    
    printf("%s: command not found\n", input);
  }
  return 0;
} 