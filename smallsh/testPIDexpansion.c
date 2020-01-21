/******************************************************************************
 * Test expanding $$ to current PID in user command
*******************************************************************************/

#include <printf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smallsh.h"

char* testPIDExpansion(char* inputString)
{
    char* result;

    printf("\ninput string: %s\n", inputString);
    result = expandPID(inputString);

    return result;
}

//Resource: https://stackoverflow.com/questions/9052490/find-the-count-of-substring-in-string
//Resource: https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
char *expandPID(char *inputString)
{
    char* pid = calloc(32, sizeof(char));
    char* insertPoint;
    char* temp;
    char* result;
    int count = 0;
    int lenFront = 0;       // Length from front when finding index for expansion

    //Save current PID
    sprintf(pid, "%d", getpid());

    //Find all instances of '$$'
    temp = inputString;
    while ((temp = strstr(temp, EXPAND_PID))) {
        temp += strlen(EXPAND_PID);
        count++;
    }

    //Allocate space for all expansions
    temp = result = calloc(strlen(inputString) + (strlen(pid) - strlen(EXPAND_PID)) * count + 1,
                           sizeof(char));

    //Return original string without modifications if nothing to expand
    if (count == 0) { strcpy(result, inputString); }

    //Else replace all instances of '$$' with PID
    while (count--) {
        insertPoint = strstr(inputString, EXPAND_PID);          // Find next '$$'
        lenFront = insertPoint - inputString;                   // Calc distance from front
        temp = strncpy(temp, inputString, lenFront) + lenFront; // Copy non-PID part of string
        temp = strcpy(temp, pid) + strlen(pid);                 // Replace '$$' with PID
        inputString += lenFront + strlen(EXPAND_PID);           // Move to next instance
    }

    free(pid);
    return result;
}

int main()
{
    printf("\nTest PID expansion\n");
    char* testResult;

    // Test with no expansion
    testResult = testPIDExpansion("user input");
    printf("Expanded result: %s\n", testResult);

    // Test with one expansion
    testResult = testPIDExpansion("testPID $$");
    printf("Expanded result: %s\n", testResult);

    // Test with two consecutive expansions
    testResult = testPIDExpansion("testPID $$ $$");
    printf("Expanded result: %s\n", testResult);

    // Test with two consecutive expansions no spaces
    testResult = testPIDExpansion("testPID$$$$");
    printf("Expanded result: %s\n", testResult);

    // Test with two non-consecutive expansions
    testResult = testPIDExpansion("testPID$$testPID$$");
    printf("Expanded result: %s\n", testResult);

    // Test with only '$$' (x4)
    testResult = testPIDExpansion("$$$$$$$$");
    printf("Expanded result: %s\n", testResult);

    free(testResult);
    return 0;
}

