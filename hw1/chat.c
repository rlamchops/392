#include "chat.h"

int main(int argc, char * argv[]) {
  //  ./chat <FD>
  int fd = atoi(argv[1]);
  fd_set set;
  FD_ZERO(&set);
  FD_SET(fd, &set);
  FD_SET(0, &set);
  while (1) {
    wait = select(FD_SETSIZE, &set, NULL, NULL, NULL);
    if (wait == -1) {}
    else {
      if (FD_ISSET(0, &set)) {
        readBuffer(0);
        if (buffer[0] == '\0' || strlen(buffer) == 0 ) {
          continue;
        }
        if (strcmp("/close", buffer) == 0) {
          exit(EXIT_SUCCESS);
        }
        else {
          write(fd, buffer, strlen(buffer));
        }
      }
      else if (FD_ISSET(fd, &set)) {
        fprintf(stdout, "<");
        fprintf(stdout, buffer);
      }
    }
  }
}

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
