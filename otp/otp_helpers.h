/******************************************************************************
 * OTP - a One-Time Pad encryption program
 * Header file declares the global variables and functions used to:
 *   transmit, encode/decode, and return messages
 *   using network sockets
*******************************************************************************/

#ifndef OTP_OTP_H
#define OTP_OTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define bool int
#define true 1
#define false 0

#define NUM_CHAR_CHOICES 27
#define NUM_CONNECTIONS 5
#define BUFFER_SIZE 1048576     //2^20
#define CHUNK_SIZE 512
#define TERMINATOR "@@"

extern const char keyChars[NUM_CHAR_CHOICES];
char programID;

void error(const char* msg);
bool checkChars(char* input);
char* readFile(char* fileName);
char* transformMessage(char *keyInput, char *messageInput, char programID);

#endif //OTP_OTP_H
