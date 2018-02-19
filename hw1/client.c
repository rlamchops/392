#include "client.h"

int main(int argc, char **argv){
    int c;
    while ((c = getopt (argc, argv, "hv")) != -1)
    switch (c)
      {
      case 'h':
        fprintf(stdout, HELP_MENU);
        exit(EXIT_SUCCESS);
      case 'v':
        #define VERBOSE
      break;
      }
    //Not enough arguments supplied to run the program right
    if(argc < 4){
        printMessage(2, "Not enough arguments supplied to run the client. NEED: 3, SUPPLIED: %d\n", argc-1);
        fprintf(stdout, HELP_MENU);
        exit(EXIT_FAILURE);
    }
    else {
      userName = argv[optind];
      serverName = argv[optind + 1];
      serverPort = argv[optind + 2];      

      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      //protocol independent way to resolve address
      if ((gaiResult = getaddrinfo(serverName, serverPort, &hints, &servInfo)) != 0) {
        fprintf(stderr, "Error resolving address %s.\n%s\n", serverName, gai_strerror(gaiResult));
        exit(1);
      }

      //getaddrinfo returns a linked list of candidates. need to try connecting to the entire list
      for (addrResult = servInfo; addrResult != NULL; addrResult = addrResult->ai_next) {
        if ((clientSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol)) == -1) {
          fprintf(stderr, "Failed to make client socket. Exiting...\n");
          exit(1);
        }
        if (connect(clientSocket, addrResult->ai_addr, addrResult->ai_addrlen) == -1) {
          fprintf(stdout, "Could not connect to %s.", addrResult->ai_addr->sa_data);
          continue;
        }
        //if code gets here, then connected to a server
        break;
      }
      if (addrResult == NULL) {
        fprintf(stderr, "Failed to connect to target.\n");
        exit(1);
      }
      //once connection is made, can now free the linked list
      freeaddrinfo(servInfo);

      //or could use send(clientSocket, message, length of message, 0)
    //   sendResult = write(clientSocket, "Hello world!", 12);
    //   if (sendResult < 0) {
    //     fprintf(stderr, "Failed to send message to server\n");
    //   }

      //Initiate login procedure with the server before spawning a thread for handling stdin input
      //If login failed then close the client
      if(loginProcedure(clientSocket, userName) == false){
          exit(EXIT_FAILURE);
      }

      //Spawn a thread for handling input from stdin
      pthread_t stdinThread;
      pthread_create(&stdinThread, NULL, &stdinHandler, NULL);

      //Send this main thread to handle input from the server
      serverHandler(clientSocket);
    }
}

//Function to set the color for the text and print the message
//1 for verbose
//2 for errors
//3 for default
void printMessage(int color, char *message, ...){
    va_list argptr;
    va_start(argptr,message);

    if(color == 1){
        #ifdef VERBOSE
        fprintf(stdout, VERBOSE_COLOR); 
        vfprintf(stdout, message, argptr);
        return;
        #endif       
    } else if(color == 2){
        fprintf(stdout, ERRORS_COLOR);
        vfprintf(stderr, message, argptr);
        return;
    } else{
        fprintf(stdout, DEFAULT_COLOR);
    }

    vfprintf(stdout, message, argptr);
}

//Dynamically allocate memory as message from the server is being read in
//one byte at a time
//Returns a malloced string which should be freed when it done being used
char * readServerMessage(int serverSocket){
    char *message = malloc(sizeof(char));
    char *messagePointer = message;
    int size = sizeof(char);
    for(int i = 0; (i = read(serverSocket, messagePointer, 1)) == 1;){
        message = realloc(message, ++size);
        messagePointer = message + size - 1;
        
        //Check for carriage returns, if their then remove them and return the message
        int length;
        if((length = strlen(message)) > 5){
            if(message[length-1] == '\n' && message[length-2] == '\r' && message[length-3] == '\n' && message[length-4] == '\r'){
                memset(message + length - 4, '\0', 4);
                return message;
            }
        }
    }
    return message;
}

//select on serversocket
//if timeout occurred print out an error message and return
int selectServer(int serverSocket, char *errorMessage, ...){
    va_list argptr;
    va_start(argptr, errorMessage);

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(serverSocket, &rset);
    struct timeval t = {10, 0};
    int ret = select(serverSocket+1, &rset, NULL, NULL, &t);

    if(ret == 0){
        printMessage(2, errorMessage, argptr);
    }

    return ret;
}

void writeMessageToServer(int serverSocket, char * protocolTag, char * serverMessage, ...){
    va_list argptr;
    va_start(argptr, serverMessage);
    char *message = malloc(sizeof(*protocolTag));
    char *fmt = malloc(sizeof(char));
    memset(message, '\0', sizeof(*protocolTag));
    strcpy(message, protocolTag);
    strcat(message, " ");
    int bytesNeeded = vsnprintf(fmt, sizeof(protocolTag), serverMessage, argptr);
    if(bytesNeeded > 0){
        fmt = realloc(fmt, sizeof(char) + bytesNeeded);
        message = realloc(message, sizeof(message) + sizeof(fmt) + 5);
        strcat(message, fmt);
        strcat(message, "\r\n\r\n\0");
    } else{
        message = realloc(message, sizeof(message) + sizeof(fmt) + 5);
        strcat(message, fmt);
        strcat(message, "\r\n\r\n\0");
    }
    write(serverSocket, message, sizeof(message));

    free(fmt);
    free(message);
}

bool loginProcedure(int serverSocket, char *userName){
    write(serverSocket, "ME2U\r\n\r\n", strlen("ME2U\r\n\r\n"));

    //timeout occurred
    if(selectServer(serverSocket, "Timeout with server occured during login with server, closing connection and client now.") == 0){
        return false;
    }

    char *response = readServerMessage(serverSocket);
    
    if(strcmp(response, "U2EM") != 0){
        printMessage(2, "Error - garbage from the server, SERVER: %s", response);
        free(response);
        return false;
    }
    free(response);

    writeMessageToServer(serverSocket, "IAM", userName);

    if(selectServer(serverSocket, "Timeout with server occured during login with server, closing connection and client now.") == 0){
        return false;
    }

    response = readServerMessage(serverSocket);

    //server could return ETAKEN to indicate that the username was already taken
    if(strcmp(response, "MAI") != 0){
        printMessage(2, "Username may already be taken if verb is ETAKEN, closing connection and client now. VERB: %s", response);
        return false;
    }
    free(response);

    //select for MOTD
    if(selectServer(serverSocket, "Timeout with server occured during login with server, closing connection and client now.") == 0){
        return false;
    }
    response = readServerMessage(serverSocket);
    char motd[5];
    memset(motd, '\0', 5);
    strncpy(motd, response, 4);
    if(strcmp(motd, "MOTD") != 0){
        printMessage(2, "Garbage received from the server, %s, now closing the connection and client.");
        return false;
    }
    printMessage(0, "%s\n", response+5);
    free(response);

    return true;
}

void *stdinHandler(){
    while(true){
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(1, &rset);
        select(1, &rset, NULL, NULL, NULL);
    }
    exit(EXIT_SUCCESS);
}

void serverHandler(int serverSocket){
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(serverSocket, &rset);
    select(1, &rset, NULL, NULL, NULL);
}
