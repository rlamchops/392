#ifndef CLIENT
#define CLIENT

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

struct chat{
	int fd1;
	int fd2;
	int pid;
	char *name;
	struct chat *next;
	struct chat *prev;
};
typedef struct chat chat;
chat *head = NULL;

#define HELP_MENU "./client [-hv] NAME SERVER_IP SERVER_PORT\n \
-h                         Displays this help menu, and returns EXIT_SUCCESS.\n \
-v                         Verbose print all incoming and outgoing protocol verbs & content.\n\
NAME                       This is the username to display when chatting.\n\
SERVER_IP                  The ip address of the server to connect to.\n\
SERVER_PORT                The port to connect to.\n"
#define HELP_MESSAGE "/help               Displays this help message\n\
/listu               List all online users\n\
/logout              Logout and exit from ME2U\n\
/chat <to> <msg>     Opens a chat window to send <msg> to <to>\n"
#define DEFAULT_COLOR "\x1B[0m"
#define ERRORS_COLOR "\x1B[1;31m"
#define VERBOSE_COLOR "\x1B[1;34m"
#define READ_SERVER char *response = readServerMessage(serverSocket); \
        if(response == NULL){\
            printMessage(2, "Server has sent garbage, now terminating connection and closing client.\n");\
			killChats();\
            close(serverSocket);\
            exit(EXIT_SUCCESS);\
        }
#define MAX_INPUT 50
#define XTERM(name, fd) char *arg[13]; \
	arg[12] = NULL; \
	for(int i = 0; i < 12; i++){ \
		arg[i] = malloc(MAX_INPUT); \
		memset(arg[i], '\0', MAX_INPUT); \
	}	\
	strcat(arg[0], "xterm"); \
	sprintf(arg[1], "-T"); \
	sprintf(arg[2], "%s", name); \
	sprintf(arg[3], "-bg");\
	sprintf(arg[4], "DarkSlateGray");\
	sprintf(arg[5], "-fa");\
	sprintf(arg[6], "\'Monospace\'");\
	sprintf(arg[7], "-fs");\
	sprintf(arg[8], "11");\
	sprintf(arg[9], "-e"); \
	sprintf(arg[10], "./chat"); \
	sprintf(arg[11], "%d", fd); \
	int PID = fork(); \
	if(PID == 0){ \
		printf("%d", execvp("xterm", arg)); \
		printf("%d", errno); \
		printf("\n");\
		exit(EXIT_FAILURE); \
	} \
	for(int i = 0; i < 12; i++){ \
		free(arg[i]);\
	} \

bool verbose = false;
char * userName;
char * serverName;
char * serverPort;
char * buffer = NULL;
struct addrinfo hints, * servInfo, * addrResult;
int gaiResult, clientSocket, sendResult;

const char *prefixList[] = {"U2EM", "MAI", "ETAKEN", "MOTD", "UTSIL", "OT", "FROM", "EDNE", "EYB", "UOFF"};

void printMessage(int color, char *message, ...);
char * readServerMessage(int serverSocket);
int selectServer(int serverSocket, char *errorMessage, ...);
void writeMessageToServer(int serverSocket, char * protocolTag, char * serverMessage, ...);
bool loginProcedure(int serverSocket, char *userName);
void selectHandler(int serverSocket);
int verifyChat(char * buffer);
char * getUsername(char * buffer);
char * getMessage(char * buffer);
void removeChat(chat * toRemove);
void addChat(char * name, int fd1, int fd2, int pid);
struct chat * getChat(char *name);
char *readMessage(int fd);
void killChats();

#endif
