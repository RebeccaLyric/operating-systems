/******************************************************************************
 * OTP - a One-Time Pad encryption program
 * Source file for the client-side decryption program
 *   validates encrypted text input
 *   creates and validates connection to otp_dec_d server
 *   sends encrypted message and prints returned plaintext to stdout
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "otp_helpers.h"

char* keyText = NULL;
char* encryptedText = NULL;

void validateInput(char *keyFile, char *keyText, char *encryptedText);
void createConnection(int portNumber);
void checkServerConnection(int socketFD, int portNumber);
void sendWithTerminator(int socketFD, char *message);
void sendMessageToServer(int socketFD, char *message);
void checkServerResponse(int socketFD, char message[]);
void receiveTerminatedServerMessage(int connectionFD, char *serverMessage);

int main(int argc, char *argv[])
{
    // Identify decryption program
    programID = 'D';

    // Check for 4 arguments, 4th should be non-negative port number
    if (argc != 4 || atoi(argv[3]) < 0) {
        fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);
        exit(0);
    }

    // Save args
    char* encryptedTextFile = argv[1];
    char* keyFile = argv[2];
    int portNumber = atoi(argv[3]);

    // Read files and check for valid input
    keyText = readFile(keyFile);
    encryptedText = readFile(encryptedTextFile);
    validateInput(keyFile, keyText, encryptedText);

    // Create the connection
    createConnection(portNumber);

    // Free memory
    free(keyText);
    free(encryptedText);

    return 0;
}

/******************************************************************************
 * Check that passed-in keyText is at least as long as passed-in encryptedText
 * Check that both contain valid characters (as defined in keyChars array)
 * Exit with error value 1 if not
*******************************************************************************/
void validateInput(char *keyFile, char *keyText, char *encryptedText)
{
    // Check that key length is >= plaintext length
    if (strlen(keyText) < strlen(encryptedText)) {
        fprintf(stderr, "Error: key '%s' is too short\n", keyFile);
        exit(1);
    }

    // Check that key and plaintext contain valid characters
    if (checkChars(keyText) == false || checkChars(encryptedText) == false)  {
        fprintf(stderr, "otp_dec error: input contains bad characters\n");
        exit(1);
    }
}

/******************************************************************************
 * Set up server connection info with passed-in port number
 * Create socket and connect to otp_dec_d server, validate connection
 * Send keyText and encryptedText to otp_dec_d
 * Receive decrypted text from server and print to stdout
*******************************************************************************/
void createConnection(int portNumber)
{
    int socketFD;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', sizeof(buffer));

    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress));             // Clear out the address struct
    serverAddress.sin_family = AF_INET;                                     // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber);                             // Store the port number
    serverHostInfo = gethostbyname("localhost");                            // Convert the machine name into a special form of address

    if (serverHostInfo == NULL) { fprintf(stderr, "otp_enc: ERROR, no such host\n"); exit(0); }

    // Copy in the address
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);                             // Create the socket
    if (socketFD < 0) { error("otp_enc: ERROR opening socket"); }

    // Connect socket to server address
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
        { error("otp_enc: ERROR connecting to otp_enc_d"); }

    // Check connected to otp_enc_d ONLY
    checkServerConnection(socketFD, portNumber);

//    printf("Confirmed connection to otp_dec_d server\n");

    // Send key text and plaintext to server
    sendWithTerminator(socketFD, keyText);
    checkServerResponse(socketFD, buffer);
    sendWithTerminator(socketFD, encryptedText);
    checkServerResponse(socketFD, buffer);

    // Receive encrypted message from server and send to stdout
    receiveTerminatedServerMessage(socketFD, buffer);
    printf("%s\n", buffer);

    // Close the socket
    close(socketFD);

}

/******************************************************************************
 * Send the programID to server over passed-in socket,
 * check for success response to verify connected to otp_dec_d
 * Exit with error value 2 if send/recv error or wrong server,
 * print error message with port number
*******************************************************************************/
void checkServerConnection(int socketFD, int portNumber)
{
    char serverResponse;

    int charSent = send(socketFD, &programID, sizeof(char), 0);
    int charRead = recv(socketFD, &serverResponse, sizeof(char), 0);

//    printf("Received response from server: %c\n", serverResponse);

    // Check for errors (want 'S' for successful connection)
    if (charSent < 0 || charRead < 0 || serverResponse != 'S') {
        fprintf(stderr, "ERROR: otp_dec cannot use otp_enc_d server\n");
        exit(2);
    }
}

/******************************************************************************
 * Append terminator characters to passed-in message
 * (used to check entire message read) and send to server over passed-in socket
 * It'll be back
*******************************************************************************/
void sendWithTerminator(int socketFD, char *message)
{
    char* terminatedMessage = NULL;

    asprintf(&terminatedMessage, "%s%s", message, TERMINATOR);
    sendMessageToServer(socketFD, terminatedMessage);

    free(terminatedMessage);            // Free memory allocated with asprintf
}

/******************************************************************************
 * Send passed-in message to server over passed-in socket
 * Check for errors
*******************************************************************************/
void sendMessageToServer(int socketFD, char *message)
{
    int charsWritten = send(socketFD, message, strlen(message), 0);    // Write to the server
    if (charsWritten < 0) error("otp_dec: ERROR writing to socket");
    if (charsWritten < strlen(message)) printf("otp_dec: WARNING: Not all data written to socket!\n");
}

/******************************************************************************
 * Takes a socket file descriptor and string message buffer
 * Clears the buffer and reads server response
 * In testing can check response by printing buffer
 * When not testing ensure program waits before sending next data
*******************************************************************************/
void checkServerResponse(int socketFD, char message[])
{
    memset(message, '\0', strlen(message));

    // Get return message from server
    int charsRead = recv(socketFD, message, BUFFER_SIZE-1, 0);      // Read data from the socket
    if (charsRead < 0) error("CLIENT: ERROR reading from socket");
//    printf("CLIENT: I received this from the server: \"%s\"\n", message);
}

/******************************************************************************
 * Takes a socket file descriptor and passed-in message buffer
 * Clear buffer and read chunks of message until reach terminator symbols
 * Replace terminator symbols with null terminator
*******************************************************************************/
void receiveTerminatedServerMessage(int connectionFD, char *serverMessage)
{
    // Clear server message
    memset(serverMessage, '\0', strlen(serverMessage));

    int charsRead = 0;
    int totalChars = 0;
    char readChunk[CHUNK_SIZE];

    // Read chunks of message and concatenate until reach terminator
    do {
        memset(readChunk, '\0', sizeof(readChunk));

        charsRead = recv(connectionFD, readChunk, sizeof(readChunk)-1, 0);      // Leave '\0'
        if (charsRead < 0) {
            error("otp_enc: ERROR reading from socket");
            return;
        }

        strcat(serverMessage, readChunk);
        totalChars += charsRead;

    } while (!strstr(serverMessage, TERMINATOR));

    // "Delete" terminal symbols by replacing with null terminator
    int terminalLocation = (int)(strstr(serverMessage, TERMINATOR) - serverMessage);
    serverMessage[terminalLocation] = '\0';

//    printf("CLIENT: I received this from the server: \"%s\"\n", serverMessage);
}
