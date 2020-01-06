/******************************************************************************
 * Header file smallsh - a basic shell written in C
 * Define macros and declare global variables and function definitions
*******************************************************************************/

#ifndef SMALLSH_SMALLSH_H
#define SMALLSH_SMALLSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define bool int
#define true 1
#define false 0

#define MAX_CHARS 2048
#define MAX_ARGS 512

#define CMD_PROMPT ": "
#define START_CAPACITY 3

bool continueExecution = true;
bool backgroundProcess = false;
bool foregroundOnly = false;
int exitStatus = 0;                 //If status run before other commands
char* startingDirectory = NULL;

struct pidList {
    int* pids;
    int size;
    int capacity;
};
struct pidList backgroundPIDs;

struct redirectFiles {
    char inputFile[MAX_CHARS];
    char outputFile[MAX_CHARS];
};
struct redirectFiles ioFiles;

struct sigaction SIGINT_action = {{0}};
struct sigaction SIGTSTP_action = {{0}};
struct sigaction ignore_action = {{0}};

void initSignalHandlers();
void catchSIGINT(int signalNum);
void catchSIGTSTP(int signalNum);
char* getWorkingDirectory(char *directoryName);
void runShell();
char* promptCommandLine();
char* expandPID(char* inputString, int numChars);
void clearCommandArgs(char **commandArgs, int numArgs);
void saveCommandArgs(char *userInput, char** commandArgs);
void processCommandArgs(char** commandArgs);
void saveIORedirects(char **commandArgs);
void redirectIO();
void addToProcessList(int pid, struct pidList *processList);
void deleteFromProcessList(int pid, struct pidList *processList);
void findCompletedProcesses();
void changeDirectory(char *path);
void executeNewProcess(char **commandArgs);
void exitShell();

#endif //SMALLSH_SMALLSH_H
