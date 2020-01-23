/******************************************************************************
 * OTP - a One-Time Pad encryption program
 * Source file defines the global functions used to:
 *   transmit, encode/decode, and return messages
 *   using network sockets
*******************************************************************************/
#include <string.h>
#include <sys/socket.h>
#include "otp_helpers.h"

const char keyChars[NUM_CHAR_CHOICES] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

/*******************************************************************************
 * Error function used for reporting issues with perror and description
*******************************************************************************/
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

/*******************************************************************************
 * Returns whether each character of passed-in input is valid
 * i.e., exists in keyChars array
*******************************************************************************/
bool checkChars(char* input)
{
    // Resource: https://stackoverflow.com/questions/10651999/how-can-i-check-if-a-single-char-exists-in-a-c-string
    int length = (int)strlen(input);
    int i = -5;

    for (i = 0; i < length-1; i++) {
        if (!strchr(keyChars, input[i])) {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
 * Open passed-in file, get length, read contents to buffer, and return buffer
*******************************************************************************/
char* readFile(char *fileName)
{
    //Resource: https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
    char* buffer = 0;
    long length = 0;
    FILE* fp = fopen(fileName, "r");
    if (!fp) { fprintf(stderr, "%s: %s\n", fileName, strerror(errno)); exit(1); }

    if (fp) {

        // Find length by moving to end of file and noting position
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);

        // Move back to beginning and read contents to buffer
        fseek(fp, 0, SEEK_SET);
        buffer = calloc(length+1, sizeof(char));
        if (buffer) {
            fread(buffer, 1, length, fp);
            buffer[length-1] = '\0';                            // Remove newline
        }

        fclose(fp);
    }

    return buffer;
}

/*******************************************************************************
 * Depending on the passed-in programID, either encrypts or decrypts
 * the passed-in message using the passed-in keyInput
 * Returns the transformed message
*******************************************************************************/
char *transformMessage(char *keyInput, char *messageInput, char programID)
{
    int i = -5;
    int keyVal = -5, msgVal = -5, newVal = -5;
    int length = (int)strlen(messageInput);
    int mod = NUM_CHAR_CHOICES;

    char messageBuffer[BUFFER_SIZE];
    memset(messageBuffer, '\0', sizeof(messageBuffer));

    for (i = 0; i < length; i++) {

        // Convert message character
        if (messageInput[i] == ' ') { msgVal = mod-1; }    // If space, change to last valid element in keyChars
        else { msgVal = (int)messageInput[i]-'A'; }

        // Convert key character
        if (keyInput[i] == ' ') { keyVal = mod-1; }        // If space, change to last valid element in keyChars
        else { keyVal = (int)keyInput[i]-'A'; }

        switch (programID) {
            case 'E':                                                       // If encrypting
                newVal = ((msgVal + keyVal) % mod) + 'A';
                if (newVal == 91) { newVal = ' '; }                         // If > 'Z', change to space
                break;

            case 'D':                                                       // If decrypting (Resource: https://stackoverflow.com/questions/11720656/modulo-operation-with-negative-numbers)
                newVal = msgVal - keyVal;
                newVal = ((newVal % mod + mod) % mod) + 'A';
                if (newVal == 91) { newVal = ' '; }                         // If > 'Z', change to space
                break;

            default:
                fprintf(stderr, "Error: Not a valid programID\n"); exit(3);
                break;
        }

        messageBuffer[i] = (char)newVal;
    }

    // Return a char* (Resource: https://stackoverflow.com/questions/46013382/c-strndup-implicit-declaration)
    char* returnMessage = calloc((size_t)length+1, sizeof(char));
    if (returnMessage) {
        memcpy(returnMessage, messageBuffer, length+1);
    }
    return returnMessage;
}
