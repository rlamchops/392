#ifndef CLIENT
#define CLIENT

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HELP_MENU "./client [-hv] NAME SERVER_IP SERVER_PORT\n \
-h                         Displays this help menu, and returns EXIT_SUCCESS.\n \
-v                         Verbose print all incoming and outgoing protocol verbs & content.\n\
NAME                       This is the username to display when chatting.\n\
SERVER_IP                  The ip address of the server to connect to.\n\
SERVER_PORT                The port to connect to.\n"

#endif