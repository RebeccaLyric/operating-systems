/******************************************************************************
 * Source file smallsh - a basic shell written in C
 * Define the functions for and execute the following functionality:
 *   expand $$ to current process id
 *   built-in commands: cd, status, exit
 *   fork and execute non-built-in commands
 *   allow processes to be run in the background with &
 *   allow file input and output redirection with < and >
 *   handle SIGINT (CTRL-C) and SIGTSTP (CTRL-Z) signals
*******************************************************************************/
#include "smallsh.h"

int main()
{
    //Initialize signal handlers
    initSignalHandlers();

    //Initialize array of background PIDs with defined starting list capacity
    backgroundPIDs.size = 0;
    backgroundPIDs.capacity = START_CAPACITY;
    backgroundPIDs.pids = malloc(sizeof(int) * backgroundPIDs.capacity);

    //Initialize file names in struct to store IO info
    memset(ioFiles.inputFile, '\0', sizeof(ioFiles.inputFile));
    memset(ioFiles.outputFile, '\0', sizeof(ioFiles.outputFile));

    //Get starting directory
    startingDirectory = getWorkingDirectory(startingDirectory);

    //Execute shell
    runShell();

    //Free memory still in use
    free(backgroundPIDs.pids);
    free(startingDirectory);

    return 0;
}

/******************************************************************************
 * Set up SIGINT (CTRL-C) and SIGTSTP (CTRL-Z) handlers
*******************************************************************************/
void initSignalHandlers() {

    //Resource: https://www.gnu.org/software/libc/manual/html_node/Flags-for-Sigaction.html
    //SIGINT handler (CTRL-C)
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;

    //SIGTSTP handler (CTRL-Z)
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;

    //Ignore handler
    ignore_action.sa_handler = SIG_IGN;

    //Activate signal handling
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/******************************************************************************
 * Print message upon SIGINT (CTRL-C) termination
*******************************************************************************/
void catchSIGINT(int signalNum)
{
//    char* message = "Caught SIGINT\n";
//    write(STDOUT_FILENO, message, 14);
//    fflush(stdout);
}

/******************************************************************************
 * Upon SIGTSTP (CTRL-Z) termination, enter or exit foreground-only mode
 * Print message
*******************************************************************************/
void catchSIGTSTP(int signalNum)
{
    if (!foregroundOnly) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        fflush(stdout);

        foregroundOnly = true;
    }

    else if (foregroundOnly) {
        char* message  = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        fflush(stdout);

        foregroundOnly = false;
    }
}

/******************************************************************************
 * Executes the primary loop of shell operations
*******************************************************************************/
void runShell()
{
    char* userInput = NULL;

    while (continueExecution) {

        char** commandArgs = malloc(MAX_ARGS * sizeof(char*));
        clearCommandArgs(commandArgs, MAX_ARGS);

        //Prompt user for command and parse into space-separated values
        userInput = promptCommandLine();
        saveCommandArgs(userInput, commandArgs);

        //Execute user command and check for completed background processes
        processCommandArgs(commandArgs);
        findCompletedProcesses();

        //Free allocated memory
        free(commandArgs);
        free(userInput);
    }
}

/******************************************************************************
 * Display the command line prompt
 * Return user input as string
*******************************************************************************/
char* promptCommandLine()
{
    int numChars = -5;
    size_t bufferSize = 0;
    char* userInput = NULL;

    while(1) {

        //Print prompt
        printf(CMD_PROMPT);
        fflush(stdout);

        //Get user input and check for read errors
        numChars = getline(&userInput, &bufferSize, stdin);

        if (numChars == -1) {
            clearerr(stdin);
        }
        else {
            break;                          // Exit the loop
        }
    }

    //Delete newline and expand $$ to PID
    userInput[numChars-1] = '\0';
    userInput = expandPID(userInput);

    return userInput;
}

/******************************************************************************
 * Takes an input string and its character count
 * Expands $$ to PID before returning string
 * Resource: https://stackoverflow.com/questions/9052490/find-the-count-of-substring-in-string
 * Resource: https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
*******************************************************************************/
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

/******************************************************************************
 * Set all strings in command args to NULL
*******************************************************************************/
void clearCommandArgs(char **commandArgs, int numArgs)
{
    for (int i = 0; i < numArgs; i++) {
        commandArgs[i] = NULL;
    }
}

/******************************************************************************
 * Take single-string user input as argument
 * Parse spaces as separate arguments
 * Save arguments in passed-in char** pointer as array of strings
*******************************************************************************/
void saveCommandArgs(char *userInput, char** commandArgs)
{
    //Resource: https://stackoverflow.com/questions/4513316/split-string-in-c-every-white-space
    char* argument = NULL;
    int argIndex = 0;
    backgroundProcess = false;

    //Separate arguments by spaces
    argument = strtok(userInput, " ");
    commandArgs[argIndex] = argument;

    while (argument != NULL) {
        argIndex++;
        argument = strtok(NULL, " ");
        commandArgs[argIndex] = argument;
    }

    //If command not empty
    if (commandArgs[0] != NULL) {

        //Check last argument for whether to send process to background
        if (strcmp(commandArgs[argIndex-1], "&") == 0) {

            commandArgs[argIndex-1] = NULL;                 //Remove & from argument list

            if (!foregroundOnly) {
                backgroundProcess = true;
            }
        }
    }
}

/******************************************************************************
 * Take array of string command args
 * Check if array[0] (i.e., the command) is not valid
 * If a built-in command (cd, status, exit), execute the command
 * Else pass command args to be forked and executed
*******************************************************************************/
void processCommandArgs(char** commandArgs)
{
    char* command = commandArgs[0];

    //Check for empty commands or comments
    if ( command == NULL || command[0] == '#' || command[0] == ' ' ) {
        return;
    }

    //Check for cd
    else if (strcmp(command, "cd") == 0) {
        char* path = commandArgs[1];
        changeDirectory(path);
    }

    //Check for status
    else if (strcmp(command, "status") == 0) {

        // If process terminated normally
        if (WIFEXITED(exitStatus)) {
            printf("exit value %d\n", WEXITSTATUS(exitStatus));
            fflush(stdout);
        }

        // Else process terminated by a signal
        else if (WIFSIGNALED(exitStatus)) {
            printf("terminated by signal %d\n", WTERMSIG(exitStatus));
            fflush(stdout);
        }
    }

    //Check for exit
    else if (strcmp(command, "exit") == 0) {
        exitShell();
    }

    //Otherwise execute specified command
    else {
        executeNewProcess(commandArgs);
    }
}

/******************************************************************************
 * Change to the path passed in as a string parameter
 * If path is NULL, go to HOME directory
*******************************************************************************/
void changeDirectory(char *path)
{
    int result = -5;

    //Change to specified path if argument given
    if(path) {
        result = chdir(path);
    }

    //Else if no argument given, change to HOME directory
    else {
        result = chdir(getenv("HOME"));
    }

    if (result != 0) { perror("cd"); exit(1); }

}

/******************************************************************************
 * Takes a list of command argument strings as its parameter
 * Forks a child process; Sets signal handling and redirects IO
 * as needed for foreground / background processes,
 * Execs process specified in 1st command arg
 * Waits for a foreground process
 * If a background process, prints PID and saves PID to list of background processes
*******************************************************************************/
void executeNewProcess(char **commandArgs)
{
    char* command = commandArgs[0];
    exitStatus = -5;
    pid_t spawnPID = -5;

    //Fork and exec specified process
    spawnPID = fork();
    switch (spawnPID) {
        case -1:
            perror("fork"); exit(1);
            break;

        case 0:

            sigaction(SIGTSTP, &ignore_action, NULL);           //Child processes ignore SIGTSTP (CTRL-Z)
            saveIORedirects(commandArgs);                       //Find any redirection

            //If background process, ignore SIGINT and redirect IO to /dev/null
            if (backgroundProcess && !foregroundOnly) {

                sigaction(SIGINT, &ignore_action, NULL);

                if (strlen(ioFiles.inputFile) == 0) {           //If no file otherwise specified
                    strcpy(ioFiles.inputFile, "/dev/null");
                }
                if (strlen(ioFiles.outputFile) == 0) {
                    strcpy(ioFiles.outputFile, "/dev/null");
                }
            }

            //Redirect IO and execute, check for errors
            redirectIO();
            exitStatus = execvp(command, commandArgs);
            if (exitStatus == -1) {
                printf("%s: ", command);
                fflush(stdout);
                perror("");
                exit(1);
            }

            break;

        default:
//            printf("Parent process: %d\n", getpid());
//            fflush(stdout);
            break;
    }

    //If a background process, add to background list and do not wait
    if (backgroundProcess && !foregroundOnly) {
        addToProcessList((int)spawnPID, &backgroundPIDs);

        printf("background pid is %d\n", (int)spawnPID);
        fflush(stdout);
    }

    //Else wait if foreground process, print if terminated by a signal
    else if (!backgroundProcess){
        waitpid(spawnPID, &exitStatus, 0);

        if (WIFSIGNALED(exitStatus)) {
            printf("terminated by signal %d\n", WTERMSIG(exitStatus));
            fflush(stdout);
        }
    }
}

/******************************************************************************
 * Save input and/or output file names
*******************************************************************************/
void saveIORedirects(char **commandArgs)
{
    memset(ioFiles.inputFile, '\0', MAX_CHARS);
    memset(ioFiles.outputFile, '\0', MAX_CHARS);

    //Start with second command arg
    int index = 1;
    char* command = commandArgs[index];

    //Find redirection operators and save following fileName
    while (command) {

        if (strcmp(command, ">") == 0 || strcmp(command, "<") == 0) {

            if (strcmp(command, ">") == 0) {
                strcpy(ioFiles.outputFile, commandArgs[index + 1]);
            }

            else if (strcmp(command, "<") == 0) {
                strcpy(ioFiles.inputFile, commandArgs[index + 1]);
            }

            //Move next arguments "left" and reset index
            if (index + 2 < MAX_ARGS) {                         //Check array bounds
                commandArgs[index] = commandArgs[index + 2];
                commandArgs[index + 1] = commandArgs[index + 3];

                commandArgs[index + 2] = NULL;
                commandArgs[index + 3] = NULL;

                index--;
            }
        }

        //Get next command
        index++;
        command = commandArgs[index];
    }

    fflush(stdout);
}

/******************************************************************************
 * Redirect input and output to file names saved in redirectFiles struct
*******************************************************************************/
void redirectIO() {

    //Resource: https://www.unix.com/programming/268879-c-unix-how-redirect-stdout-file-c-code.html
    //Resource: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html

    int inputFD = -5;
    int outputFD = -5;
    int fileResult = -5;

    //Redirect input
    if (strlen(ioFiles.inputFile) != 0) {
        inputFD = open(ioFiles.inputFile, O_RDONLY);
        if (inputFD == -1) {
            printf("cannot open %s for input\n", ioFiles.inputFile);
            fflush(stdout);
            exit(1);
        }

        fileResult = dup2(inputFD, STDIN_FILENO);
        if (fileResult == -1) { perror("input dup2"); exit(1); }
    }

    //Redirect output
    if (strlen(ioFiles.outputFile) != 0) {
        outputFD = open(ioFiles.outputFile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (outputFD == -1) {
            printf("cannot open %s for output\n", ioFiles.outputFile);
            fflush(stdout);
            exit(2);
        }

        fileResult = dup2(outputFD, STDOUT_FILENO);
        if (fileResult == -1) { perror("output dup2"); exit(2); }
    }
}

/******************************************************************************
 * Resize PID array if more capacity needed
 * Add passed-in PID to list
*******************************************************************************/
void addToProcessList(int pid, struct pidList *processList)
{
    //Resource: https://stackoverflow.com/questions/3536153/c-dynamically-growing-array

    if (processList->size >= processList->capacity) {
        processList->capacity *= 2;
        processList->pids = realloc(processList->pids, processList->capacity * sizeof(int));
    }

    processList->pids[processList->size++] = pid;
}

/******************************************************************************
 * Find the passed-in PID in the passed-in process list
 * Remove by shifting all later values and decrement process list size
*******************************************************************************/
void deleteFromProcessList(int pid, struct pidList *processList)
{
    //Traverse process list to find matching PID
    for (int i = 0; i < processList->size; i++) {

        if (processList->pids[i] == pid) {

            //Delete by shifting all other PIDs "left"
            for (int j = i; j < processList->size; j++) {

                processList->pids[j] = processList->pids[j+1];
                processList->size--;
                return;
            }
        }
    }
}

/******************************************************************************
 * Traverse background processes array for completed processes
 * For all completed, print message with PID and exit/termination method
 * Remove PID from background process list
*******************************************************************************/
void findCompletedProcesses()
{
    pid_t completed = -5;
    int exitMethod = -5;

    //Use do-while loop to ensure no remaining processes
    //For loop may miss completed process if already iterated over

    do {
        // Check for any completed process
        completed = waitpid(-1, &exitMethod, WNOHANG);

        // If PID returned
        if (completed > 0) {
            printf("background pid %d is done: ", completed);
            fflush(stdout);

            // If process terminated normally
            if (WIFEXITED(exitMethod)) {
                printf("exit value %d\n", WEXITSTATUS(exitMethod));
                fflush(stdout);
            }

            // Else process terminated by a signal
            else if (WIFSIGNALED(exitMethod)) {
                printf("terminated by signal %d\n", WTERMSIG(exitMethod));
                fflush(stdout);
            }

            deleteFromProcessList(completed, &backgroundPIDs);
        }

    } while (completed > 0);
}

/******************************************************************************
 * Clean up processes, return to working directory, exit the shell
*******************************************************************************/
void exitShell()
{
    //Terminate background processes
    for (int i = 0; i < backgroundPIDs.size; i++) {
        kill(backgroundPIDs.pids[i], SIGKILL);
    }

    //Return to working directory
    changeDirectory(startingDirectory);

    //Set variable to exit smallsh loop execution
    continueExecution = false;
}

/******************************************************************************
 * Save the current working directory in the string passed in as a parameter
*******************************************************************************/
char* getWorkingDirectory(char *directoryName)
{
    //Resource: https://pubs.opengroup.org/onlinepubs/009695399/functions/getcwd.html

    directoryName = NULL;
    char* buffer;

    if ( (buffer = (char*)malloc(MAX_CHARS)) != NULL )  {
        directoryName = getcwd(buffer, MAX_CHARS);
    }

    return directoryName;
}
