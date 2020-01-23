/******************************************************************************
 * keygen: Generate a key of a length passed in as the argument
 * The characters of the key are randomly selected from A..Z plus ' '
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

char* generateKey(int length);

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));

    //Check for valid arguments
    if (argc != 2 || atoi(argv[1]) < 0) {
        fprintf(stderr, "Error: keygen must be called with a positive integer for key length\n");
        return 1;
    }

    //Generate the key and print to stdout
    int keyLength = atoi(argv[1]);
    char* newKey = generateKey(keyLength);
    printf("%s", newKey);

    free(newKey);

    return 0;
}

/*******************************************************************************
 * Return a random string of the passed-in length
 * Uses defined array of all capital letters plus a space ' '
*******************************************************************************/
char* generateKey(int length)
{
    char keyChars[27] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                         'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                         'W', 'X', 'Y', 'Z', ' '};

    //Allocate enough space for the length + newline
    char* returnKey = calloc( ((size_t)length + 1), sizeof(char) );

    //Randomly generate each character of the key and add a newline
    for (int i = 0; i < length; i++) {
        int charIdx = rand() % 27;
        returnKey[i] = keyChars[charIdx];
    }
    returnKey[length] = '\n';

    return returnKey;
}

