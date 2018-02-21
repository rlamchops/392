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
  setbuf(stdout, NULL);

  while (1) {
    FD_ZERO(&set);
    FD_SET(fd, &set);
    FD_SET(0, &set);
    write(1, "<", 1);

    int ret = select(FD_SETSIZE, &set, NULL, NULL, NULL);
    //error case
    if (ret == -1) {
      write(1, "select failed\n", 14);
    }

    else {
      write(1, "after select\n", 13);

      //handle stdin input
      if (FD_ISSET(0, &set)) {
        write(1, "before read\n",12);
        readBuffer(0);
        write(1, "after read\n", 11);
        write(1, buffer, strlen(buffer));
        if (buffer[0] == '\0' || strlen(buffer) == 0 ) {
          continue;
        }
        if (strcmp("/close\n", buffer) == 0) {
          close(fd);
          exit(EXIT_SUCCESS);
        }
        else {
          write(fd, buffer, strlen(buffer));
          // free(buffer);
          // write(1, "\n", 1);
        }
      }

      //input from client
      else if (FD_ISSET(fd, &set)) {
        readBuffer(fd);
        write(1, buffer, strlen(buffer));
        // free(buffer);
      }
      else {
        write(1, "error\n", 6);
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
    write(1, last_char, 1);
    strncpy(buffer + size - 1, last_char, 1);
    buffer = realloc(buffer, size + 1);
    size++;
    if (last_char[0] == '\n') {
      break;
    }
  }
  write(1, "outside for\n", 12);
  buffer[size-1] = '\0';
}
