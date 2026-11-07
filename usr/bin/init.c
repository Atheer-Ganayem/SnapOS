#include <unistd.h>

int main() {
  char buf[128];
  char prefix[] = "snapos> ";
  char res[] = "input: ";

  while (1) {
    if (write(STDIN_FILENO, prefix, sizeof(prefix) - 1) <= 0)
      break;
    
    long ret = read(STDIN_FILENO, buf, 128 - 10);
    //  sys_read(0, buf, 128 - 10);
    if (ret < 0) {
      break;
    }

    if (write(STDOUT_FILENO, res, sizeof(res) - 1) <= 0)
      break;

    if (write(STDOUT_FILENO, buf, ret) <= 0)
      break;
  }

  return 0;
}