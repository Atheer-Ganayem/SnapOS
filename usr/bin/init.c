#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

int main() {
  volatile uint64_t counter = 0;

  char buf[128];
  char prefix[] = "snapos-v1> ";
  char res[] = "input: ";

  while (1) {
    write(STDIN_FILENO, prefix, sizeof(prefix) - 1);
    long ret = read(STDIN_FILENO, buf, 128 - 10);
    //  sys_read(0, buf, 128 - 10);
    if (ret < 0) {
      break;
    }

    write(STDOUT_FILENO, res, sizeof(res) - 1);

    write(STDOUT_FILENO, buf, ret);
  }

  return 0;
}