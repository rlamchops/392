#define _GNU_SOURCE

// #include "sfwrite.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <pthread.h>
#include <sys/epoll.h>

char * buffer = NULL;
void readBuffer(int fd);
