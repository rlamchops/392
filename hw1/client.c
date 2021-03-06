#include "client.h"

int main(int argc, char **argv){
    int c;
    while ((c = getopt (argc, argv, "hv")) != -1)
    switch (c)
      {
      case 'h':
        fprintf(stdout, HELP_MENU);
        exit(EXIT_SUCCESS);
        break;
      case 'v':
        verbose = true;
        break;
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
        printMessage(2, "Error resolving address %s.\n%s\n", serverName, gai_strerror(gaiResult));
        exit(EXIT_FAILURE);
      }

      //getaddrinfo returns a linked list of candidates. need to try connecting to the entire list
      for (addrResult = servInfo; addrResult != NULL; addrResult = addrResult->ai_next) {
        if ((clientSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol)) == -1) {
          printMessage(2, "Failed to make client socket. Exiting...\n");
          exit(EXIT_FAILURE);
        }
        if (connect(clientSocket, addrResult->ai_addr, addrResult->ai_addrlen) == -1) {
          printMessage(0, "Could not connect to %s.\n", addrResult->ai_addr->sa_data);
          continue;
        }
        //if code gets here, then connected to a server
        break;
      }
      if (addrResult == NULL) {
        printMessage(2, "Failed to connect to target.\n");
        exit(EXIT_FAILURE);
      }
      //once connection is made, can now free the linked list
      freeaddrinfo(servInfo);

      //Initiate login procedure with the server before spawning a thread for handling stdin input
      //If login failed then close the client
      if(loginProcedure(clientSocket, userName) == false){
          exit(EXIT_FAILURE);
      }

      //Send this main thread to handle input from the server
      selectHandler(clientSocket);
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
        if(verbose){
            fprintf(stdout, VERBOSE_COLOR);
            vfprintf(stdout, message, argptr);
            fprintf(stdout, DEFAULT_COLOR);
        }
        return;
    } else if(color == 2){
        fprintf(stderr, ERRORS_COLOR);
        vfprintf(stderr, message, argptr);
        fprintf(stdout, DEFAULT_COLOR);
        fprintf(stderr, DEFAULT_COLOR);
        return;
    } else{
        fprintf(stdout, DEFAULT_COLOR);
    }

    vfprintf(stdout, message, argptr);
}

//Dynamically allocate memory as message from the server is being read in
//one byte at a time
//Returns a malloced string which should be freed when it is done being used
char * readServerMessage(int serverSocket){
    char *message = malloc(sizeof(char));
    char *messagePointer = message;
    int size = sizeof(char);
    message[size-1] = '\0';

    bool properPrefix = false;

    for(int i = 0; (i = read(serverSocket, messagePointer, 1)) == 1;){
        size++;
        message = realloc(message, size);
        message[size-1] = '\0';
        messagePointer = message + size - 1;

        //Check for proper prefix in server message
        if(!properPrefix && (size-1 >  1 && size-1 < 7)){
            for(int i = 0; i < sizeof(prefixList)/sizeof(prefixList[0]); i++){
                if(strcmp(message, ((char *)prefixList[i])) == 0){
                    properPrefix = true;
                    break;
                }
            }
        }
        //At this point message is larger than largest known prefix and is still not a proper prefix
        //meaning the server has sent garbage
        else if(!properPrefix && size-1 >= 7){
            return NULL;
        }

        //Check for carriage returns, if there then remove them and return the message
        int length;
        if((length = strlen(message)) > 5){
            if(message[length-1] == '\n' && message[length-2] == '\r' && message[length-3] == '\n' && message[length-4] == '\r'){
                memset(message + length - 4, '\0', 4);
                printMessage(1, "VERBOSE: %s\n", message);
                return message;
            }
        }
    }

    //If it reaches this point then it means that the client finished reading
    //everything in the server socket and never saw a complete terminating sequence
    return NULL;
}

//select on serversocket with a timeout
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

void writeMessageToServer(int serverSocket, char * protocolTag, char * serverMessage){
    if(serverMessage == NULL){
        printMessage(1, protocolTag);
        char *m = malloc(strlen(protocolTag + 4));
        strcpy(m, protocolTag);
        strcpy(m+strlen(protocolTag), "\r\n\r\n");
        write(serverSocket, m, strlen(protocolTag) + 4);
        free(m);
        return;
    }
    char *message = malloc(strlen(protocolTag) + strlen(serverMessage) + 5);
    char *fmt = malloc(sizeof(char));
    memset(message, '\0', strlen(protocolTag) + strlen(serverMessage) + 5);
    strcpy(message, protocolTag);
    strcat(message, " ");
    strcat(message, serverMessage);
    strcat(message, "\r\n\r\n");
    write(serverSocket, message, strlen(message));

    printMessage(1, "VERBOSE: %s\n", message);

    free(fmt);
    free(message);
}

bool loginProcedure(int serverSocket, char *userName){
    write(serverSocket, "ME2U\r\n\r\n", strlen("ME2U\r\n\r\n"));
    printMessage(1, "VERBOSE: ME2U\n");

    //timeout occurred
    if(selectServer(serverSocket, "Timeout with server occured during login, closing connection and client now.\n") == 0){
        return false;
    }

    READ_SERVER

    if(strcmp(response, "U2EM") != 0){
        printMessage(2, "Error - garbage from the server, SERVER: %s\n", response);
        free(response);
        return false;
    }
    free(response);

    writeMessageToServer(serverSocket, "IAM", userName);

    if(selectServer(serverSocket, "Timeout with server occured during login, closing connection and client now.\n") == 0){
        return false;
    }

    response = readServerMessage(serverSocket);
        if(response == NULL){
            printMessage(2, "Server has sent garbage, now terminating connection and closing client.\n");
            close(serverSocket);
            exit(EXIT_SUCCESS);
        }

    //server could return ETAKEN to indicate that the username was already taken
    if(strcmp(response, "MAI") != 0){
        printMessage(2, "Username may already be taken if verb is ETAKEN, closing connection and client now. VERB: %s\n", response);
        return false;
    }
    free(response);

    //select for MOTD
    if(selectServer(serverSocket, "Timeout with server occured during login, closing connection and client now.\n") == 0){
        return false;
    }
    response = readServerMessage(serverSocket);
        if(response == NULL){
            printMessage(2, "Server has sent garbage, now terminating connection and closing client.\n");
            close(serverSocket);
            exit(EXIT_SUCCESS);
        }
    char motd[5];
    memset(motd, '\0', 5);
    strncpy(motd, response, 4);
    if(strcmp(motd, "MOTD") != 0){
        printMessage(2, "Garbage received from the server, %s, now closing the connection and client.\n");
        return false;
    }
    printMessage(0, "%s\n", response+5);
    free(response);

    return true;
}

//allocates just enough memory for the max message so far
void readBuffer(int fd) {
  if (buffer == NULL) {
    buffer = malloc(1);
    buffer[0] = '\0';
  }
  int size = 1;
  char last_char[1] = {0};
  for(;read(fd, last_char, 1) == 1;) {
    if (*last_char == '\n') {
      break;
    }
    strncpy(buffer + size - 1, last_char, 1);
    buffer = realloc(buffer, size + 1);
    size++;
  }
  buffer[size-1] = '\0';
}

//verifyChat makes sure that there is a target in the buffer
int verifyChat(char * buffer) {
  //start at buffer[6] because buffer[0-5] contains "/chat "
  for(int a = 6; buffer[a] != '\0'; a++) {
    //if there is an extra space or there was no content
    if (a == 6 && ((buffer[a] == ' ') || (buffer[6] == '\0'))) {
      return 0;
    }
    //name was found
    else if (a > 6 && (buffer[a] == ' ')) {
      return 1;
    }
  }
  return 0;
}

void selectHandler(int serverSocket){
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(serverSocket, &rset);
    FD_SET(0, &rset);
    while(true){
        FD_ZERO(&rset);
        FD_SET(serverSocket, &rset);
        FD_SET(0, &rset);
        for(chat *iterator = head; iterator != NULL; iterator = iterator->next){
          FD_SET(iterator->fd1, &rset);
        }
        select(FD_SETSIZE, &rset, NULL, NULL, NULL);

        //server handling case
        if(FD_ISSET(serverSocket, &rset)){
            READ_SERVER

            char *verb = malloc(sizeof(char) * 6);
            memset(verb, '\0', 6);
            strncpy(verb, response, 4);

            //FROM handler
            if(strcmp(verb, "FROM") == 0){
                if(sizeof(response) < 6){
                    printMessage(2, "No name in the FROM message from the server, closing connection and exiting now.");
                    close(serverSocket);
                    exit(EXIT_FAILURE);
                }
                char *p = response+5;
                char *name = malloc(sizeof(char));
                int length = 1;
                name[length-1] = '\0';
                for(int i = 0; i < strlen(p) && p[i] != ' '; i++){
                    strncpy(name+length-1, p+i, 1);
                    length++;
                    name = realloc(name, length);
                    name[length-1] = '\0';
                }
                //Send the MORF to name now
                writeMessageToServer(serverSocket, "MORF", name);
                //Now to get the rest of the message
                p = response + 5 + strlen(name) + 1;
                char *message = malloc(sizeof(char));
                length = 1;
                message[length-1] = '\0';
                for(int i = 0; i < strlen(p); i++){
                    strncpy(message + length - 1, p + i, 1);
                    length++;
                    message = realloc(message, length);
                    message[length-1] = '\0';
                }
                // TODO open xterm if it's not open already
                struct chat *person = getChat(name);
                //NULL means no chat window open for said person
                if(person == NULL){
                    int socketPair[2];
                    socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair);
                    XTERM(name, socketPair[1]);
                    addChat(name, socketPair[0], socketPair[1], PID);
                    write(socketPair[0], message, strlen(message));
                    write(socketPair[0], "\n", 1);
                }
                //Non-NULL means already a chat window
                else{
                    write(person->fd1, message, strlen(message));
                    write(person->fd1, "\n", 1);
                }

                free(name);
                free(message);
            }

            //UOFF user logged off handler
            memset(verb, '\0', 6);
            strncpy(verb, response, 4);
            if(strcmp(verb, "UOFF") == 0){
                char *p = response+5;
                char *name = malloc(sizeof(char));
                int length = 1;
                name[length-1] = '\0';
                for(int i = 0; i < strlen(p); i++){
                    strncpy(name+length-1, p+i, 1);
                    length++;
                    name = realloc(name, length);
                    name[length-1] = '\0';
                }
                // TODO find the existing xterm window and kill it
                struct chat *person = getChat(name);
                if(person == NULL){
                    //nothing to do then
                }
                //else gotta kill this child/window
                else{
                    kill(person->pid, 9);
                    removeChat(person);
                }
                free(name);
            }

            free(response);
            free(verb);
        }

        //stdin handling case
        else if(FD_ISSET(0, &rset)){
            //readBuffer allocates memory to buffer so msg can be read.
            readBuffer(0);
            if (strcmp("/help", buffer) == 0) {
              printMessage(3, HELP_MESSAGE);
            }

            //list users case
            else if (strcmp("/listu", buffer) == 0) {
                write(clientSocket, "LISTU\r\n\r\n", 9);
                if(selectServer(clientSocket, "Select timed out waiting for list user response, closing connection and client.") == 0){
                    close(clientSocket);
                    killChats();
                    exit(EXIT_FAILURE);
                }

                READ_SERVER
                char verb[6];
                memset(verb, '\0', 6);
                strncpy(verb, response, 5);
                if(strcmp(verb, "UTSIL") == 0){
                    printMessage(0, "User List: %s\n", response+6);
                    free(response);
                    continue;
                }
                printMessage(2, "Improper prefix was sent as response to list users, closing connection and client now.");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }

            //client is logging out case
            else if (strcmp("/logout", buffer) == 0) {
              write(clientSocket, "BYE\r\n\r\n", 7);
              if(selectServer(clientSocket, "Select on server while telling the server the client is logging out timed out, closing server connection and closing client now.\n") == 0){
                  close(clientSocket);
                  killChats();
                  exit(EXIT_FAILURE);
              }
              READ_SERVER
              if(strcmp(response, "EYB") != 0){
                  free(response);
                  close(clientSocket);
                  printMessage(2, "Garbage from server was sent back while logging out, closing server connection and closing client now.\n");
                  killChats();
                  exit(EXIT_FAILURE);
              }
              free(response);
              close(clientSocket);
              killChats();
              exit(EXIT_SUCCESS);
            }
            //chatting to others
            else if (strncmp("/chat ", buffer, 6) == 0) {
              //first need to verify that this /chat request is valid
              if (verifyChat(buffer)) {
                //buffer[6] is where the username begins
                writeMessageToServer(clientSocket, "TO", &buffer[6]);
                //should wait for OT here
                if (selectServer(clientSocket, "Select on server while waiting for OT timed out. Try chatting again.\n") == 0) {
                  continue;
                }
                char * targetName = getUsername(buffer);
                char * temp = getMessage(buffer);
                READ_SERVER
                char verb[3] = {'\0', '\0', '\0'};
                strncpy(verb, response, 2);
                if ((strcmp(verb, "OT") != 0) || (strcmp(&response[3], targetName) != 0)) {
                  printMessage(2, "Garbage from server after TO message or was an EDNE indicating user does not exist. Try chatting again.\n");
                  continue;
                }
                //create XTERM and IPC if OT is received
                bool activeWindow = false;
                //but first check if a window with this person is already open
                for (chat * iterator = head; iterator != NULL; iterator=iterator->next) {
                  if (strcmp(iterator->name, targetName) == 0) {
                      if (waitpid(iterator->pid, NULL, WNOHANG) != 0) {
                        close(iterator->fd1);
                        close(iterator->fd2);
                        removeChat(iterator);
                        // printf("removed a chat");
                        break;
                      }
                      else {
                        activeWindow = true;
                        write(iterator->fd1, temp, strlen(temp));
                        write(iterator->fd1, "\n", 1);
                        break;
                      }
                  }
                }

                if (!activeWindow) {
                  int socketPair[2];
                  socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair);
                  XTERM(targetName, socketPair[1]);
                  addChat(targetName, socketPair[0], socketPair[1], PID);
                  //send the chat the /chat contents
                  write(socketPair[0], temp, strlen(temp));
                  write(socketPair[0], "\n", 1);
                }

                free(targetName);
              }
              else {
                printMessage(3, "Unknown command %s.\n", buffer);
              }
            }
            else {
              printMessage(3, "Unknown command %s.\n", buffer);
            }
        }

        else{
            for(chat *iterator = head; iterator != NULL; iterator = iterator->next){
                if(FD_ISSET(iterator->fd1, &rset)){
                    char *message = readMessage(iterator->fd1);
                    char *wholeMessage = malloc(strlen(message) + strlen(iterator->name) + 1);
                    memset(wholeMessage, '\0', strlen(iterator->name) + strlen(message) + 1);
                    strcpy(wholeMessage, iterator->name);
                    strcat(wholeMessage, " ");
                    strcat(wholeMessage, message);
                    writeMessageToServer(serverSocket, "TO", wholeMessage);
                    free(message);
                    free(wholeMessage);
                }
            }
        }
    }
}

char *readMessage(int fd){
    char *m = malloc(sizeof(char));
    int size = 1;
    m[size-1] = '\0';
    for(int i = 0; (i = read(fd, m+size-1, 1)) == 1;){
        if(m[size-1] == '\n'){
            m[size-1] = '\0';
            return m;
        }
        size++;
        m = realloc(m, size);
        m[size-1] = '\0';
    }
    return m;
}

//grab the username of the target
//returns string that must be freed
char * getUsername(char * buffer) {
  char * temp = malloc(1);
  temp[0] = '\0';
  int size = 1;
  for (int a = 6; buffer[a] != ' '; a++) {
    strncpy(temp + size - 1, &buffer[a], 1);
    temp = realloc(temp, size + 1);
    size++;
  }
  temp[size-1] = '\0';
  return temp;
}


//grab the start of the message
char * getMessage(char * buffer) {
  for (int a = 6; buffer[a] != '\0'; a++) {
    if (buffer[a] == ' ') {
      return &buffer[a+1];
    }
  }
  return NULL;
}

void addChat(char * name, int fd1, int fd2, int pid) {
  if (head == NULL) {
    head = malloc(sizeof(chat));
    head->name = malloc(strlen(name) + 1);
    strcpy(head->name, name);
    head->name[strlen(name)] = '\0';
    head->fd1 = fd1;
    head->fd2 = fd2;
    head->pid = pid;
    head->next = NULL;
    head->prev = NULL;
  }
  else {
    chat * temp = malloc(sizeof(chat));
    temp->name = malloc(strlen(name) + 1);
    strcpy(temp->name, name);
    temp->name[strlen(name)] = '\0';
    temp->fd1 = fd1;
    temp->fd2 = fd2;
    temp->pid = pid;
    temp->next = head;
    temp->prev = NULL;
    head->prev = temp;
    head = temp;
  }
}
void removeChat(chat * toRemove) {
  if ((toRemove->prev == NULL) && (toRemove->next == NULL)) {
    close(toRemove->fd1);
    close(toRemove->fd2);
    free(toRemove->name);
    free(toRemove);
    head = NULL;
  }
  else if ((toRemove->prev == NULL) && (toRemove->next != NULL)) {
    close(toRemove->fd1);
    close(toRemove->fd2);
    head = toRemove->next;
    toRemove->next->prev = NULL;
    free(toRemove->name);
    free(toRemove);
  }
  else if ((toRemove->prev != NULL) && (toRemove->next == NULL)) {
    close(toRemove->fd1);
    close(toRemove->fd2);
    toRemove->prev->next = NULL;
    free(toRemove->name);
    free(toRemove);
  }
  else {
    close(toRemove->fd1);
    close(toRemove->fd2);
    toRemove->prev->next = toRemove->next;
    toRemove->next->prev = toRemove->prev;
    free(toRemove->name);
    free(toRemove);
  }
}

struct chat * getChat(char *name){
    if(head == NULL){
        return NULL;
    } else{
        struct chat *pointer = head;
        while(pointer != NULL){
            if(strcmp(pointer->name, name) == 0){
                return pointer;
            }
            pointer = pointer->next;
        }
    }
    return NULL;
}

void killChats() {
  for(chat * temp = head; temp != NULL;) {
    chat * next = temp->next;
    free(temp->name);
    kill(temp->pid, 9);
    close(temp->fd1);
    close(temp->fd2);
    free(temp);
    temp = next;
  }
  head = NULL;
}
