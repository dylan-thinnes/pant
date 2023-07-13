#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main () {
  int fd = open("/dev/tty", O_RDONLY);
  char buf[100];
  char* head = buf;
  int command_started = 0;
  int color = 6; // start at white (47)
  while (1) {
    int n = read(fd, head, 1);

    // break when receiving a Ctrl+C
    if (*head == '\x03') break;

    // State machine for tracking an ANSI command
    if (*head == '\x1b') {
      if (command_started == 2) command_started = 2;
      else command_started = 1;
    } else if (*head == '[') {
      if (command_started == 1) command_started = 2;
      else command_started = 0;
    } else if (*head == 'M' || *head == 'm') {
      if (command_started == 2) command_started = 3;
      else command_started = 0;
    } else {
      if (command_started == 2) command_started = 2;
      else {
        command_started = 0;
        if (*head >= '1' && *head <= '7') {
          color = *head - '1';
        }
      }
    }

    fprintf(stderr, "%d %d\r\n", *head, command_started);

    // Once a command is complete, parse it, and reset the buffer
    if (command_started == 3) {
      head[n] = '\0';
      fprintf(stderr, "Got command! '%s'\r\n", buf + 1);
      int code, x, y = 0;
      int nread = sscanf(buf + 1, "[<%d;%d;%d", &code, &x, &y);
      if (nread != 3) sscanf(buf + 1, "[%d;%d;%d", &code, &x, &y);
      fprintf(stderr, "Extracted %d %d %d\r\n", code, x, y);
      printf("\x1b[%d;%dH\x1b[%dm \x1b[0;0H\x1b[0m", y, x, color + 41);
      fflush(stdout);
      head = buf;
    } else {
      // When globbing a command, continue loading into the buffer
      if (command_started > 0 && command_started < 3) head += n;
    }
  }
}
