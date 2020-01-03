/******************************************************************************
 * Adventure Program
 * Reads room information from room files generated in Build Rooms program
 * Executes text-based adventure game in which user navigates from room to room
 * At the end of the game prints the sequence of rooms visited which forms a
 * sequence of ballet steps as "choreography"
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zconf.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>

#define bool int
#define true 1
#define false 0

#define BUFFER_SIZE 512
#define ARRAY_SIZE 1024
#define NUM_ROOMS 7
#define MAX_CONNECTIONS 6
#define TIME_FILE_NAME "currentTime.txt"

char* roomTypes[] = {"START_ROOM", "MID_ROOM", "END_ROOM"};
struct room {
    int id;
    char roomType[BUFFER_SIZE];           //defined above
    char name[BUFFER_SIZE];
    int numOutboundConnections;
    struct room* outboundConnections[MAX_CONNECTIONS];
};

struct room balletRooms[NUM_ROOMS];
struct room nullRoom;
typedef enum {mostRecentFile, roomInfo, connectionInfo} searchTypes;

//Function prototypes - read room info from files
void IsNewest(struct dirent *currentFile, int *previousNewTime, char *newestFileName);
void InitBalletRooms();
void InitNullRoom();
void SearchDirectory(char *dirToOpen, char *targetSubstring, searchTypes currentSearch, char *directoryName);
FILE* OpenFile(char *fileName);
void ReadRoomAndType(FILE *inputFile, int roomIndex);
void ReadConnections(FILE *inputFile, int roomIndex);

//Function prototypes - play game
struct room* FindStartRoom();
void PrintLocationInfo(struct room *currentRoom);
void PrintEndGameInfo(int pathTaken[], int stepCount);
struct room *GetRoomFromUser(struct room *currentRoom);
char* FindRoomNameFromId(int roomId);
void PlayGame();

//Function prototypes - keep time
void* GetCurrentTime();
void* WriteCurrentTime();
void* PrintCurrentTime();
pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;

int main() {

    //// Find and read room info from files
    char newestDirectory[BUFFER_SIZE];
    memset(newestDirectory, '\0', sizeof(newestDirectory));
    char targetSubstring[BUFFER_SIZE];
    memset(targetSubstring, '\0', sizeof(targetSubstring));

    // Initialize all room structs with null and zero data
    InitBalletRooms();
    InitNullRoom();

    // Find newest buildrooms directory, read files for room names, save connections
    SearchDirectory(".", "tayloreb.rooms.", mostRecentFile, newestDirectory);
    SearchDirectory(newestDirectory, "_room", roomInfo, NULL);
    SearchDirectory(".", "_room", connectionInfo, NULL);            // Already changed into newest buildrooms directory
    chdir("..");                                                    // Move back up to working directory

    //// Play game then destroy mutex
    PlayGame();
    pthread_mutex_destroy(&timeMutex);

    return 0;
}

/******************************************************************************
 * Initialize all ballet room structs with null and zero data
*******************************************************************************/
void InitBalletRooms()
{
    int i = -5;
    int j = -5;

    for (i = 0; i < NUM_ROOMS; i++) {

        balletRooms[i].id = i;

        //Clear name, clear room type, set num connections to 0
        memset(balletRooms[i].name, '\0', sizeof(balletRooms[i].name));
        memset(balletRooms[i].roomType, '\0', sizeof(balletRooms[i].roomType));
        balletRooms[i].numOutboundConnections = 0;

        //Set all room connections to null
        for (j = 0; j < MAX_CONNECTIONS; j++) {
            balletRooms[i].outboundConnections[j] = NULL;
        }
    }
}

/******************************************************************************
 * Initialize null room struct with null and zero data
 * Null room is returned if no matching room struct
 * Room type can also be set to "time" to signal user entered time command
*******************************************************************************/
void InitNullRoom()
{
    int i = -5;
    nullRoom.id = -1;

    //Set name to "null_room", clear room type, set num connections to 0
    memset(nullRoom.name, '\0', sizeof(nullRoom.name));
    sprintf(nullRoom.name, "null_room");
    memset(nullRoom.roomType, '\0', sizeof(nullRoom.roomType));
    nullRoom.numOutboundConnections = 0;

    //Set all room connections to null
    for (i = 0; i < MAX_CONNECTIONS; i++) {
        nullRoom.outboundConnections[i] = NULL;
    }
}

/******************************************************************************
 * Search through files in directory
 * Arguments: the directory to open, the substring to find target files,
 * which operation to execute on the found matching files,
 * string to set with the result of the directory search
*******************************************************************************/
void SearchDirectory(char *dirToOpen, char *targetSubstring, searchTypes currentSearch, char *directoryName)
{
    int newestDirTime = -1;                             // Modified timestamp of newest subdir examined
    int roomIndex = 0;                                  // Iterate through all room files

    DIR* currentDir;                                    // Holds the current subdir of the starting dir
    struct dirent* nextFile;

    currentDir = opendir(dirToOpen);

    if(currentDir) {                                                        // Check could open current directory
        while ((nextFile = readdir(currentDir)) != NULL) {                  // Check each entry in dir

            if (strstr(nextFile->d_name, targetSubstring) != NULL) {        // If entry has prefix
                //printf("\tFound file with substring: %s\n", nextFile->d_name);

                switch (currentSearch) {

                    case mostRecentFile:                                    // Find most recent file
                        IsNewest(nextFile, &newestDirTime, directoryName);
                        break;

                    case roomInfo:                                          // Get data from room files
                        chdir(dirToOpen);
                        ReadRoomAndType(OpenFile(nextFile->d_name), roomIndex);
                        roomIndex++;
                        break;

                    case connectionInfo:                                    // Get room connections from room files
                        chdir(dirToOpen);
                        ReadConnections(OpenFile(nextFile->d_name), roomIndex);
                        roomIndex++;
                        break;

                    default:
                        printf("Oh no, could not open the directory with that operation\n");
                        exit(1);
                }
            }
        }
    }

    else {
        printf("Oh no, could not open directory: %s\n", dirToOpen);
        exit(1);
    }

    closedir(currentDir);
}

/******************************************************************************
 * Find newest file
 * Arguments: the currently open file, the previous newest time,
 * a string to set with the newest file name
*******************************************************************************/
void IsNewest(struct dirent *currentFile, int *previousNewTime, char *newestFileName)
{
    // Get attributes of the entry
    struct stat dirAttributes;
    stat(currentFile->d_name, &dirAttributes);

    // If this time is bigger
    if ((int)dirAttributes.st_mtime > *previousNewTime)
    {
        // Set as most recent
        *previousNewTime = (int)dirAttributes.st_mtime;
        memset(newestFileName, '\0', sizeof(newestFileName));
        strcpy(newestFileName, currentFile->d_name);
        // printf("Newer subdir: %s, new time: %d\n", nextFile->d_name, *previousNewTime);
    }
}

/******************************************************************************
 * Open a file with the name passed in and return the file pointer
 * Does not close file
*******************************************************************************/
FILE* OpenFile(char *fileName)
{
    FILE* fp = NULL;
    fp = fopen(fileName, "r");
    if (fp == NULL) {
        printf("\nOh no, %s could not be opened\n", fileName);
        exit(1);
    }
    //printf("\nSuccessfully opened file: %s\n", fileName);

    return fp;
}

/******************************************************************************
 * Read room name and type from passed-in file
 * Save in room struct index passed as second parameter
 * Then close file
*******************************************************************************/
void ReadRoomAndType(FILE *inputFile, int roomIndex)
{
    char nextLine[BUFFER_SIZE];
    memset(nextLine, '\0', sizeof(nextLine));
    char roomData[BUFFER_SIZE];
    memset(roomData, '\0', sizeof(roomData));

    // Ensure reading from beginning
    fseek(inputFile, 0, SEEK_SET);

    // For each line of the file
    while (!feof(inputFile)) {
        if (fgets(nextLine, sizeof(nextLine), inputFile)) {

            // Save the string after the first descriptor and colon into room data
            sscanf(nextLine, "%*s %*s %s", roomData);

            // Save room name to room struct
            if (strstr(nextLine, "ROOM NAME") != NULL) {
                sprintf(balletRooms[roomIndex].name, roomData);
                memset(roomData, '\0', sizeof(roomData));
            }

            // Save room type to room struct
            else if (strstr(nextLine, "ROOM TYPE") != NULL) {
                sprintf(balletRooms[roomIndex].roomType, roomData);
                memset(roomData, '\0', sizeof(roomData));
            }
        }
    }

    fclose(inputFile);
}

/******************************************************************************
 * Get connection names from passed-in file
 * Find matching room struct and add as connection to room at passed-in index
 * Then close file
*******************************************************************************/
void ReadConnections(FILE *inputFile, int roomIndex)
{
    int i = -5;
    int connectionIndex = 0;

    char nextLine[BUFFER_SIZE];
    memset(nextLine, '\0', sizeof(nextLine));
    char connectionName[BUFFER_SIZE];
    memset(connectionName, '\0', sizeof(connectionName));

    // Ensure reading from beginning
    fseek(inputFile, 0, SEEK_SET);

    // For each line of the file
    while (!feof(inputFile)) {
        if (fgets(nextLine, sizeof(nextLine), inputFile)) {

            // Check if contains connection data
            if (strstr(nextLine, "CONNECTION") != NULL) {

                // Save name of connecting room (string after "CONNECTION: ")
                sscanf(nextLine, "%*s %*s %s", connectionName);

                // Find corresponding room struct matching connection name
                for (i = 0; i < NUM_ROOMS; i++) {

                    if (strcmp(balletRooms[i].name, connectionName) == 0) {

                        // Assign as outbound connection and increment num connections
                        balletRooms[roomIndex].outboundConnections[connectionIndex] = &balletRooms[i];
                        balletRooms[roomIndex].numOutboundConnections++;

                        // Reset connection name and move to next connection index
                        memset(connectionName, '\0', sizeof(connectionName));
                        connectionIndex++;
                    }
                }
            }
        }
    }

    fclose(inputFile);
}

/******************************************************************************
 * Find start room and return
 * Returns null room is no room with type "START_ROOM" found
*******************************************************************************/
struct room* FindStartRoom()
{
    int i = -5;
    for (i = 0; i < NUM_ROOMS; i++) {
        if (strcmp(balletRooms[i].roomType, "START_ROOM") == 0)
        {
            return &balletRooms[i];
        }
    }

    return &nullRoom;                        //If start room not found
}

/******************************************************************************
 * Find room struct by passed-in room name
 * Search passed-in current room connections
 * Returns null room if no matching room found
*******************************************************************************/
struct room *FindRoomStructFromName(char *roomName, struct room *currentRoom)
{
    int i = -5;
    int j = -5;

    // Check all current room connections for matching room name
    for (i = 0; i < currentRoom->numOutboundConnections; i++) {

        // If found matching room name, find corresponding room struct
        if (strcmp(currentRoom->outboundConnections[i]->name, roomName) == 0)
        {
            for (j = 0; j < NUM_ROOMS; j++) {
                if (strcmp(balletRooms[j].name, roomName) == 0) {
                    return &balletRooms[j];
                }
            }
        }
    }

    return &nullRoom;                        // If matching room not found
}

/******************************************************************************
 * Find room name by passed-in room id
 * Returns "unknown room" if not matching room found
*******************************************************************************/
char* FindRoomNameFromId(int roomId)
{
    int i = -5;
    for (i = 0; i < NUM_ROOMS; i++) {
        if (balletRooms[i].id == roomId) {
            return balletRooms[i].name;
        }
    }

    return "unknown room";                  // If matching room not found
}

/******************************************************************************
 * Print current location and list possible connections from
 * passed-in room struct
*******************************************************************************/
void PrintLocationInfo(struct room *currentRoom)
{
    int i = -5;

    printf("CURRENT LOCATION: %s\n", currentRoom->name);

    printf("POSSIBLE CONNECTIONS: ");
    for (i = 0 ; i < currentRoom->numOutboundConnections; i++) {

        // Do not print comma or space for last room
        if (i == currentRoom->numOutboundConnections-1) {
            printf("%s", currentRoom->outboundConnections[i]->name);
        }
        // Else print comma if not last room
        else {
            printf("%s, ", currentRoom->outboundConnections[i]->name);
        }
    }
    printf(".\n");                              //End line with period
}

/******************************************************************************
 * Print end game message, number of steps taken, and list each step
*******************************************************************************/
void PrintEndGameInfo(int pathTaken[], int stepCount)
{
    int i = -5;

    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n"
           "YOU TOOK %d STEP(S). YOUR PATH TO VICTORY WAS: \n", stepCount);

    for (i = 0; i < stepCount; i++) {
        printf("%s\n", FindRoomNameFromId(pathTaken[i]));
    }
}

/******************************************************************************
 * Print "Where to" prompt and read console input from user
 * If user enters "time", set null room type to "time" and return null room
 * Return room struct matching user input or null if none matching
*******************************************************************************/
struct room* GetRoomFromUser(struct room* currentRoom)
{
    char* userInput = NULL;
    size_t bufferSize = 0;
    ssize_t numChars = -5;

    struct room* matchingRoom = NULL;

    // Print prompt and get user input
    printf("WHERE TO? >");
    numChars = getline(&userInput, &bufferSize, stdin);
    userInput[numChars-1] = '\0';                           // Discard newline

    // Check for time command, set time in room type, free user input memory, and return room
    if (strcmp(userInput, "time") == 0) {
        strcpy(nullRoom.roomType, "time");
        free(userInput);
        return &nullRoom;
    }

    // Else find matching room, free user input memory, and return matching room
    matchingRoom = FindRoomStructFromName(userInput, currentRoom);
    free(userInput);
    return matchingRoom;
}

/******************************************************************************
 * Get the current time, format, and print to file
*******************************************************************************/
void *GetCurrentTime()
{
    // Get current time (Reference: http://www.cplusplus.com/reference/ctime/strftime/)
    time_t currentTime;
    struct tm* timeData;
    char timeOutput[BUFFER_SIZE];
    memset(timeOutput, '\0', sizeof(timeOutput));

    time(&currentTime);
    timeData = localtime(&currentTime);

    // Print formatted time data to string
    strftime(timeOutput, BUFFER_SIZE, "%I:%M%P, %A, %B %d, %Y", timeData);

    // Write time data string to file and close file
    FILE* timeFile;
    timeFile = fopen(TIME_FILE_NAME, "w");
    if (timeFile == NULL) {
        printf("Oh no, could not open time file for writing\n");
        exit(1);
    }
    fprintf(timeFile, "%s\n", timeOutput);
    fclose(timeFile);
}

/******************************************************************************
 * Lock the time mutex, create second thread which writes current time to file
*******************************************************************************/
void *WriteCurrentTime()
{
    // Lock time mutex
    pthread_mutex_lock(&timeMutex);

    // Create time thread with GetCurrentTime() and assert successfully created
    pthread_t timeThread;
    int timeResult;
    timeResult = pthread_create(&timeThread, NULL, (void*) &GetCurrentTime, NULL);
    assert(timeResult == 0);

    // Unlock time mutex and pause
    pthread_mutex_unlock(&timeMutex);
    sleep(1);
}

/******************************************************************************
 * Print the current time from time file to the console
 * Locks the printTime mutex for time thread to execute
*******************************************************************************/
void *PrintCurrentTime()
{
    char timeLine[BUFFER_SIZE];
    memset(timeLine, '\0', sizeof(timeLine));

    // Open time file
    FILE* timeFile;
    timeFile = fopen(TIME_FILE_NAME, "r");
    if (timeFile == NULL) {
        printf("Oh no, could not open time file for reading\n");
        exit(1);
    }

    // Print formatted time from time file to console and close file
    if (fgets(timeLine, sizeof(timeLine), timeFile)) {
        printf("\n%s\n", timeLine);
    }
    fclose(timeFile);
}

/******************************************************************************
 * Begin game
 * Prints location info to the user and changes room based on user input
 * Displays the current time and date if user enters "time" command
 * Game ends when user moves to "END_ROOM"
*******************************************************************************/
void PlayGame()
{
    struct room* currentRoom = NULL;
    struct room* returnedRoom = NULL;

    int stepCount = 0;
    int pathTaken[ARRAY_SIZE];

    bool endGame = false;

    //Find start room and begin game
    currentRoom = FindStartRoom();

    do {

        PrintLocationInfo(currentRoom);

        //Move to next room and check if not valid choice
        GET_ROOM: returnedRoom = GetRoomFromUser(currentRoom);
        if ( (strcmp(returnedRoom->name, "null_room") == 0) &&
             (strcmp(returnedRoom->roomType, "time") != 0)) {
            printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }

        // Else check if user entered time command
        else if ( (strcmp(returnedRoom->name, "null_room") == 0) &&
                  (strcmp(returnedRoom->roomType, "time") == 0) ) {
            InitNullRoom();                                 // Clear "time" from room type
            WriteCurrentTime();                             // Second thread writes time to file
            PrintCurrentTime();                             // Read time from file to console
            goto GET_ROOM;                                  // Skip re-printing room info
        }

        //Else is valid room
        else {
            printf("\n");
            currentRoom = returnedRoom;
            pathTaken[stepCount] = currentRoom->id;
            stepCount++;

            //Check for exit
            if (strcmp(currentRoom->roomType, "END_ROOM") == 0) {
                PrintEndGameInfo(pathTaken, stepCount);
                endGame = true;
            }
        }

    } while (endGame == false);
}
