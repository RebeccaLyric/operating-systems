/******************************************************************************
 * Source file for the server-side decryption program
 *   creates and validates connection to otp_dec client
 *   receives encrypted message and sends decrypted message back to client
 *   supports up to 5 concurrent socket connections
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "otp_helpers.h"

void beginListening(int portNumber);
void checkClientConnection(int socketFD);
void receiveTerminatedClientMessage(int connectionFD, char clientMessage[]);
void sendServerResponse(int connectionFD, char *message);
void sendWithTerminator(int socketFD, char *message);

int main(int argc, char *argv[])
{
    // Identify decryption program
    programID = 'D';

    // Check usage & args
	if (argc != 2 || atoi(argv[1]) < 0) {
	    fprintf(stderr,"USAGE: %s port\n", argv[0]);
	    exit(1);
	}

	// Save port number
    int portNumber = atoi(argv[1]);

	// Begin listening
    beginListening(portNumber);

	return 0; 
}

/******************************************************************************
 * Set up server info with passed-in port number
 * Create listening socket and listen for connections
 * Spawn a child process for up to 5 connections
 * For each connection, validate connected to otp_dec client
 * Receive encrypted text and key text from client and send back plaintext
*******************************************************************************/
void beginListening(int portNumber)
{
    int listenSocketFD, establishedConnectionFD;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress, clientAddress;

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', sizeof(buffer));

    char receivedKey[BUFFER_SIZE];
    memset(receivedKey, '\0', sizeof(receivedKey));

    char receivedEncryptedText[BUFFER_SIZE];
    memset(receivedEncryptedText, '\0', sizeof(receivedEncryptedText));

    // Set up the address struct for this process (the server)
    memset((char *)&serverAddress, '\0', sizeof(serverAddress));    // Clear out the address struct
    serverAddress.sin_family = AF_INET;                             // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber);                     // Store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY;                     // Any address is allowed for connection to this process

    // Create the socket
    if ((listenSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("otp_enc_d: ERROR opening socket");
    }

    // Enable the socket to begin listening - connect socket to port
    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("otp_enc_d: ERROR on binding");
    }

    // Flip the socket on - it can now receive up to 5 connections
    listen(listenSocketFD, NUM_CONNECTIONS);


    // Continue listening until socket closed
    while(1) {

        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress);                       // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (establishedConnectionFD < 0) error("otp_enc_d: ERROR on accept");

        pid_t childPID = fork();
        switch(childPID) {
            case -1:
                perror("fork"); exit(1);
                break;

            case 0:

                // Check connected to otp_enc client ONLY
                checkClientConnection(establishedConnectionFD);

                // Receive key and plaintext from client
                receiveTerminatedClientMessage(establishedConnectionFD, receivedKey);
                sendServerResponse(establishedConnectionFD, "received connection");
                receiveTerminatedClientMessage(establishedConnectionFD, receivedEncryptedText);
                sendServerResponse(establishedConnectionFD, "received plaintext");

                // Decrypt message and send back to client
                char* decryptedMessage = NULL;
                decryptedMessage = transformMessage(receivedKey, receivedEncryptedText, programID);
                sendWithTerminator(establishedConnectionFD, decryptedMessage);

                close(establishedConnectionFD);                     // Close the existing socket which is connected to the client
                close(listenSocketFD);                              // Close the listening socket
                free(decryptedMessage);                             // Free memory allocated in transformMessage()

                exit(0);

            default:
//                printf("Parent process: %d\n", getpid());
//                fflush(stdout);
                break;
        }
    }
}

/******************************************************************************
 * Receive the programID from the client over passed-in socket
 * Check that programID matches ('D' for decryption)
 * Send back either 'S' or 'F' for successful or failed check
*******************************************************************************/
void checkClientConnection(int socketFD)
{
    char clientID;

    int charRead = recv(socketFD, &clientID, sizeof(char), 0);

    // Check for error in client response
    if (charRead < 0 || clientID != programID) {
        sendServerResponse(socketFD, "F");                          // Failed connection
    }

    // Else send success response
    sendServerResponse(socketFD, "S");                              // Successful connection
}

/******************************************************************************
 * Clear the passed-in message buffer, read characters from the client
 * over the passed-in socket file descriptor until reach terminator characters
 * Replace terminator characters with null terminator
*******************************************************************************/
void receiveTerminatedClientMessage(int connectionFD, char clientMessage[])
{
    // Clear client message
    memset(clientMessage, '\0', strlen(clientMessage));

    int charsRead = 0;
    int totalChars = 0;
    char readChunk[CHUNK_SIZE];

    // Read chunks of message and concatenate until reach terminator
    do {
        memset(readChunk, '\0', sizeof(readChunk));

        charsRead = recv(connectionFD, readChunk, sizeof(readChunk) - 1, 0);      // Leave '\0'
        if (charsRead < 0) {
            error("otp_dec_d: ERROR reading from socket");
            return;
        }

        strcat(clientMessage, readChunk);
        totalChars += charsRead;

    } while (!strstr(clientMessage, TERMINATOR));

    // "Delete" terminal symbols by replacing with null terminator
    int terminalLocation = (int)(strstr(clientMessage, TERMINATOR) - clientMessage);
    clientMessage[terminalLocation] = '\0';

//    printf("SERVER: I received this from the client: \"%s\"\n", clientMessage);
}

/******************************************************************************
 * Takes a socket file descriptor and passed-in message
 * Sends message to client for either testing OR
 * When not testing makes client wait before sending next data
*******************************************************************************/
void sendServerResponse(int connectionFD, char *message)
{
    int charsWritten = -5;
    charsWritten = send(connectionFD, message, strlen(message), 0); // Send success back
    if (charsWritten < 0) error("ERROR writing to socket");
}

/******************************************************************************
 * Append terminator characters to passed-in message
 * (used to check entire message read) and send to client over passed-in socket
 * It'll be back
*******************************************************************************/
void sendWithTerminator(int socketFD, char *message)
{
    char* terminatedMessage = NULL;

    asprintf(&terminatedMessage, "%s%s", message, TERMINATOR);
    sendServerResponse(socketFD, terminatedMessage);

    free(terminatedMessage);
}

