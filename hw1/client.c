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
      loginProcedure(clientSocket, userName);

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
        fprintf(stdout, VERBOSE_COLOR);        
    } else if(color == 2){
        fprintf(stdout, ERRORS_COLOR);
    } else{
        fprintf(stdout, DEFAULT_COLOR);
    }

    vfprintf(stdout, message, argptr);
}

//Dynamically allocate memory as message from the server is being read in
//one byte at a time
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

bool loginProcedure(int serverSocket, char *userName){
    write(serverSocket, "ME2U\r\n\r\n", strlen("ME2U\r\n\r\n"));
    char *response = readServerMessage(serverSocket);
    printMessage(2, "%s\n", response);
    if(strcmp(response, "U2EM") == 0){
        printMessage(1, "It's working so far\n");
    }
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
