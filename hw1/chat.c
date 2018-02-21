#include "chat.h"

int main(int argc, char * argv[]) {
  //  ./chat <FD>
  //it's up to the client to format FROM <from> <msg>\r\n\r\n into ><msg>
  //and <<msg> into TO <to> <msg>\r\n\r\n
  int fd = atoi(argv[1]);
  fd_set set;
  FD_ZERO(&set);
  FD_SET(fd, &set);
  FD_SET(0, &set);
  while (1) {
    write(1, "<", 1);
    int ret = select(FD_SETSIZE, &set, NULL, NULL, NULL);
    if (ret == -1) {
      write(1, "hi", 2);
    }
    else {
      if (FD_ISSET(0, &set)) {
        readBuffer(0);
        if (buffer[0] == '\0' || strlen(buffer) == 0 ) {
          continue;
        }
        if (strcmp("/close", buffer) == 0) {
          close(fd);
          exit(EXIT_SUCCESS);
        }
        else {
          write(fd, buffer, strlen(buffer));
          // write(1, "\n", 1);
        }
      }
      else if (FD_ISSET(fd, &set)) {
        readBuffer(fd);
        write(1, buffer, strlen(buffer));
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
