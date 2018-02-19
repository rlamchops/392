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
    struct timeval *t = malloc(sizeof(struct timeval));
    memset((void *)t, '\0', sizeof(struct timeval));
    t->tv_sec = 10;
    t->tv_usec = 10000000;
    int ret = select(1, &rset, NULL, NULL, t);
    free(t);

    if(ret == 0){
        printMessage(2, "Timeout with server occurred during login, closing connection and client now.\n");
    }

    return ret;
}

bool loginProcedure(int serverSocket, char *userName){
    write(serverSocket, "ME2U\r\n\r\n", strlen("ME2U\r\n\r\n"));

    int ret = selectServer(serverSocket, "Timeout with server occured during login with server, closing connection and client now.");

    //timeout occurred
    if(ret == 0){
        return false;
    }

    char *response = readServerMessage(serverSocket);

    if(strcmp(response, "U2EM") == 0){
        printMessage(1, "It's working so far\n");
    }
    free(response);
    return true;
}

void *stdinHandler(){
    int wait = 0;
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(0, &rset);
    while(true){
        write(1, ">", 1);
        wait = select(FD_SETSIZE, &rset, NULL, NULL, NULL);
        if (wait == -1) {}
        else {
          int sendResult;
          if (FD_ISSET(0, &rset)) {
            //readBuffer allocates memory to buffer so msg can be read.
            readBuffer(0);
            if (strcmp("/help", buffer) == 0) {
              printMessage(3, HELP_MESSAGE);
            }
            else if (strcmp("/listu", buffer) == 0) {
              sendResult = write(clientSocket, "LISTU\r\n\r\n", 9);
            }
            else if (strcmp("/logout", buffer) == 0) {
              sendResult = write(clientSocket, "BYE\r\n\r\n", 7);
            }
            else if (strncmp("/chat", buffer, 5) == 0) {
              //the overall increase in memory needed is 1 byte
              char * temp = malloc(strlen(buffer) + 1);
              memset(temp, 0, strlen(buffer) + 1);
              strcpy(temp, buffer);
              //shift 3 char's over
              memmove(temp, &temp[3], strlen((&temp[3]));
              //construct message
              temp[0] = 'T';
              temp[1] = 'O';
              temp[8] = '\r';
              temp[9] = '\n';
              temp[10] = '\r';
              temp[11] = '\n';
              sendResult = write(clientSocket, temp, strlen(temp));
              free(temp);
            }
            else {
              printMessage(2, "Unknown command %s.\n", buffer);
            }
          }
        }
    }
    exit(EXIT_SUCCESS);
}

void serverHandler(int serverSocket){
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(serverSocket, &rset);
    select(1, &rset, NULL, NULL, NULL);
}
