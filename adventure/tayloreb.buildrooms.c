/******************************************************************************
 * Build Rooms Program
 * Defines an array of 10 room names (named for ballet steps)
 * Randomly selects 7 rooms and randomly assigns from 3..6 connections to other rooms
 * Writes room data to files in a directory with the current process ID
*******************************************************************************/
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

#define bool int
#define true 1
#define false 0

#define NUM_ROOM_OPTIONS 10
#define NUM_ROOMS 7
#define MIN_CONNECTIONS 3
#define MAX_CONNECTIONS 6
#define BUFFER_SIZE 512
#define PREFIX "rebeccalyric"

char* roomTypes[] = {"START_ROOM", "MID_ROOM", "END_ROOM"};
struct room {
    int id;
    char* roomType;                           //defined above
    char* name;
    int numOutboundConnections;
    struct room* outboundConnections[MAX_CONNECTIONS];
};

struct room balletRooms[NUM_ROOMS];

//Function prototypes - create room structs and files
void CreateRandomString(int numOptions, int numValues, char *chosenValues);
void InitializeRooms(char **roomNames, char *randomRoomNums);
void AssignStartAndEnd();
void WriteRoomFiles(char *directoryName, char **roomNames, char *randomRoomNums);

//Function prototypes - connect room graph
bool IsGraphFull();
void AddRandomConnection();
struct room* GetRandomRoom();
bool CanAddConnectionFrom(struct room *balletRoom);
bool ConnectionAlreadyExists(struct room *ballet1, struct room *ballet2);
void ConnectRoom(struct room *ballet1, struct room *ballet2);
bool IsSameRoom(struct room *ballet1, struct room *ballet2);

int main()
{
    // Seed random
    srand(time(NULL));

    // Room choices are named for ballet steps - when connected form "choreography"
    char *roomNames[NUM_ROOM_OPTIONS] = {"plie", "tendu", "passe", "chasse", "brise",
                                         "jete", "glissade", "cabriole", "sissone", "fouette"};

    // Select 7 random rooms and save id nums in randomRoomNums string
    char randomRoomNums[BUFFER_SIZE];
    memset(randomRoomNums, '\0', sizeof(randomRoomNums));
    CreateRandomString(NUM_ROOM_OPTIONS, NUM_ROOMS, randomRoomNums);

    // Set info for each room
    InitializeRooms(roomNames, randomRoomNums);
    AssignStartAndEnd();

    // Create all connections in graph
    while (IsGraphFull() == false)
    {
        AddRandomConnection();
    }

    // Make new directory named with current PID and write room info to files
    char directoryName[BUFFER_SIZE];
    memset(directoryName, '\0', sizeof(directoryName));
    sprintf(directoryName, "%s.rooms.%d", PREFIX, (int)getpid());
    int result = mkdir(directoryName, 0755);
    assert(result == 0);

    WriteRoomFiles(directoryName, roomNames, randomRoomNums);

    return 0;
}

/******************************************************************************
 * Create string of selected values
 * Parameters: the maximum range of integers to choose from,
 * the number of values to choose, a string to hold the randomly selected values
*******************************************************************************/
void CreateRandomString(int numOptions, int numValues, char *chosenValues)
{
    int i = -5;
    int randomValue = 15;
    char charVal = '\0';
    char *c = NULL;

    for (i = 0; i < numValues; i++) {
        randomValue = rand() % numOptions;
        charVal = randomValue + '0';                //Convert int to char (only works for 0..9)

        //Check if value already used
        c = strchr(chosenValues, charVal);
        if (c != NULL) {                            //If already exists do not use
            i--;
        }
        else {                                      //Else add to selected values
            chosenValues[i] = charVal;
        }
    }
}

/******************************************************************************
 * Add starting info to room structs
 * Parameters: the array of available room names,
 * a string (character array) of randomly selected room numbers
*******************************************************************************/
void InitializeRooms(char **roomNames, char *randomRoomNums)
{
    int i = -5;
    int j = -5;
    int roomNum = -1;

    //For each room selected in random room nums string
    for (i = 0; i < NUM_ROOMS; i++) {

        roomNum = randomRoomNums[i] - '0';                  // Convert char to int (only works for 0..9)
        balletRooms[i].id = i;
        balletRooms[i].name = roomNames[roomNum];
        balletRooms[i].roomType = roomTypes[1];             // Set "MID_ROOM" as default
        balletRooms[i].numOutboundConnections = 0;

        // Set all pointers to outbound connections to NULL
        for (j = 0; j < MAX_CONNECTIONS; j++) {
            balletRooms[i].outboundConnections[j] = NULL;
        }
    }
}

/******************************************************************************
 * Randomly choose start and end room values and assign to corresponding room
*******************************************************************************/
void AssignStartAndEnd()
{
    // Get random indices
    char startAndEndValues[BUFFER_SIZE];
    memset(startAndEndValues, '\0', sizeof(startAndEndValues));
    CreateRandomString(NUM_ROOMS, 2, startAndEndValues);

    // Set start room
    int startRoom = startAndEndValues[0] - '0';              // Convert from char to int (only works for 0..9)
    balletRooms[startRoom].roomType = roomTypes[0];

    // Set end room
    int endRoom = startAndEndValues[1] - '0';
    balletRooms[endRoom].roomType = roomTypes[2];
}

/******************************************************************************
 * Create room files in buildrooms directory with current PID
 * Print room name, all connections (3..6), and room type to each file
*******************************************************************************/
void WriteRoomFiles(char *directoryName, char **roomNames, char *randomRoomNums)
{
    int i = -5;
    int j = -5;

    char roomPath[BUFFER_SIZE];
    memset(roomPath, '\0', sizeof(roomPath));

    for (i = 0; i < NUM_ROOMS; i++) {

        //Set room path and create file for writing
        sprintf(roomPath, "%s/%s_room", directoryName, balletRooms[i].name);

        FILE* currentFile;
        currentFile = fopen(roomPath, "w");
        if (!currentFile) {
            printf("Oh no, could not open file: %s\n", roomPath);
            exit(1);
        }

        //Write room name
        fprintf(currentFile, "%s%s\n", "ROOM NAME: ", balletRooms[i].name);

        //Write names of all room's connections
        for (j = 0; j < balletRooms[i].numOutboundConnections; j++) {

            fprintf(currentFile, "%s%d: %s\n", "CONNECTION ", j+1,
                    balletRooms[i].outboundConnections[j]->name);
        }

        //Write room type
        fprintf(currentFile, "%s%s\n", "ROOM TYPE: ", balletRooms[i].roomType);

        //Close file and clear roomPath string
        fclose(currentFile);
        memset(roomPath, '\0', sizeof(roomPath));
    }
}

/******************************************************************************
 * Check if all rooms have at least 3 connections
 * Returns true if all rooms have 3 to 6 outbound connections, false otherwise
*******************************************************************************/
bool IsGraphFull()
{
    int i = -5;
    for (i = 0; i < NUM_ROOMS; i++) {
        if (balletRooms[i].numOutboundConnections < MIN_CONNECTIONS) {
            return false;
        }
    }
    return true;
}

/******************************************************************************
 * Adds a random, valid outbound connection from a Room to another Room
*******************************************************************************/
void AddRandomConnection()
{
    struct room *newRoom1;
    struct room *newRoom2;

    // Select random room 1 which can have more connections
    while(true)
    {
        newRoom1 = GetRandomRoom();

        if (CanAddConnectionFrom(newRoom1) == true) {
            break;
        }
    }

    // Select random room 2 while rooms are not the same and a connection does not already exist
    do
    {
        newRoom2 = GetRandomRoom();
    }
    while(CanAddConnectionFrom(newRoom2) == false ||
          IsSameRoom(newRoom1, newRoom2) == true  ||
          ConnectionAlreadyExists(newRoom1, newRoom2) == true);

    // Connect each room to the other
    ConnectRoom(newRoom1, newRoom2);
    ConnectRoom(newRoom2, newRoom1);
}

/******************************************************************************
 * Returns a random Room, does NOT validate if connection can be added
*******************************************************************************/
struct room * GetRandomRoom()
{
    int randomValue = 15;
    randomValue = rand() % NUM_ROOMS;
    return &balletRooms[randomValue];
}

/******************************************************************************
 * Check if 2 rooms are already connected to each other
 * Returns true if a connection can be added from Room x
 * (< 6 outbound connections), false otherwise
*******************************************************************************/
bool CanAddConnectionFrom(struct room *balletRoom)
{
    if (balletRoom->numOutboundConnections >= 6) {
        return false;
    }
    return true;
}

/******************************************************************************
 * Check if 2 rooms are already connected to each other
 * Returns true if a connection from Room x to Room y already exists,
 * false otherwise
*******************************************************************************/
bool ConnectionAlreadyExists(struct room *ballet1, struct room *ballet2)
{
    int i;
    for (i = 0; i < ballet1->numOutboundConnections; i++) {

        if (ballet1->outboundConnections[i]->id == ballet2->id) {
            return true;
        }
    }
    return false;
}

/******************************************************************************
 * Connects Rooms x and y together,
 * does not check if this connection is valid
*******************************************************************************/
void ConnectRoom(struct room *ballet1, struct room *ballet2)
{
    //Find index and place next outbound connection
    int i = ballet1->numOutboundConnections;
    ballet1->outboundConnections[i] = ballet2;

    //Increment number of connections
    ballet1->numOutboundConnections++;
}

/******************************************************************************
 * Returns true if Rooms x and y are the same Room, false otherwise
*******************************************************************************/
bool IsSameRoom(struct room *ballet1, struct room *ballet2)
{
    if (ballet1->id == ballet2->id) {
        return true;
    }
    return false;
}
