#ifndef CLIENT
#define CLIENT

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define HELP_MENU "./client [-hv] NAME SERVER_IP SERVER_PORT\n \
-h                         Displays this help menu, and returns EXIT_SUCCESS.\n \
-v                         Verbose print all incoming and outgoing protocol verbs & content.\n\
NAME                       This is the username to display when chatting.\n\
SERVER_IP                  The ip address of the server to connect to.\n\
SERVER_PORT                The port to connect to.\n"

char * userName;
char * serverName;
char * serverPort;
struct addrinfo hints, * servInfo, * addrResult;
int gaiResult, clientSocket, sendResult;

bool loginProcedure(int serverSocket);s
void serverHandler(int serverSocket);
void *stdinHandler();

#endif
