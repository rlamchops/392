#ifndef CLIENT
#define CLIENT

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdarg.h>

#define HELP_MENU "./client [-hv] NAME SERVER_IP SERVER_PORT\n \
-h                         Displays this help menu, and returns EXIT_SUCCESS.\n \
-v                         Verbose print all incoming and outgoing protocol verbs & content.\n\
NAME                       This is the username to display when chatting.\n\
SERVER_IP                  The ip address of the server to connect to.\n\
SERVER_PORT                The port to connect to.\n"
#define DEFAULT_COLOR "\x1B[0m"
#define ERRORS_COLOR "\x1B[1;31m"
#define VERBOSE_COLOR "\x1B[1;34m"

char * userName;
char * serverName;
char * serverPort;
struct addrinfo hints, * servInfo, * addrResult;
int gaiResult, clientSocket, sendResult;

void printMessage(int color, char *message, ...);
char * readServerMessage(int serverSocket);
int selectServer(int serverSocket, char *errorMessage, ...);
void writeMessageToServer(int serverSocket, char * protocolTag, char * serverMessage, ...);
bool loginProcedure(int serverSocket, char *userName);
void serverHandler(int serverSocket);
void *stdinHandler();

#endif
