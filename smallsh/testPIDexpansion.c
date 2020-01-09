/******************************************************************************
 * Test expanding $$ to current PID in user command
*******************************************************************************/

#include <printf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smallsh.h"

char *expandPID(char *inputString)
{
    char* pid = calloc(32, sizeof(char));
    char* temp;
    int count = 0;

    //Save current PID
    sprintf(pid, "%d", getpid());

    //Find all instances of '$$'
    temp = inputString;
    while ((temp = strstr(temp, EXPAND_PID))) {
        temp += strlen(EXPAND_PID);
        count++;
    }
    printf("Count: %d\n", count);

    //Return original string without modifications if nothing to expand
    if (count == 0) { free(pid); return inputString; }

    //Else replace all instances of '$$' with PID
    while (count--) {
        printf("\tcount: %d\n", count);
    }

    free(pid);
    return inputString;
}

int main()
{
    printf("\nTest PID expansion\n");

    char* userInput = calloc(32, sizeof(char));

    // Test with no expansion
    sprintf(userInput, "userInput");
    printf("user input: %s\n", userInput);

    userInput = expandPID(userInput);
    printf("Expanded user input: %s\n\n", userInput);

    // Test with one expansion
    sprintf(userInput, "testPID $$");
    printf("user input: %s\n", userInput);

    userInput = expandPID(userInput);
    printf("Expanded user input: %s\n\n", userInput);

    // Test with two consecutive expansions
    sprintf(userInput, "testPID $$ $$");
    printf("user input: %s\n", userInput);

    userInput = expandPID(userInput);
    printf("Expanded user input: %s\n\n", userInput);

    // Test with two consecutive expansions no spaces
    sprintf(userInput, "testPID$$$$");
    printf("user input: %s\n", userInput);

    userInput = expandPID(userInput);
    printf("Expanded user input: %s\n\n", userInput);

    // Test with two non-consecutive expansions
    sprintf(userInput, "testPID$$testPID$$");
    printf("user input: %s\n", userInput);

    userInput = expandPID(userInput);
    printf("Expanded user input: %s\n\n", userInput);

    free(userInput);

    return 0;
}

