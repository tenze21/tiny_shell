#include <stdio.h>
#include "string.h"

int main() {
  // Flush after every printf
  setbuf(stdout, NULL);

  // Uncomment this block to pass the first stage
  printf("$ ");

  // Wait for user input
  char input[100];
  fgets(input, sizeof(input), stdin);

  //fgets also saves the newline character added, when user presses enter, so we want to remove that newline character.
  //strcspn searches for the first occurance in a string of any of the specified characters and returns the length of the string up to that point.
  input[strcspn(input, "\n")]= '\0';
  printf("%s: command not found\n", input);
  return 0;
} 