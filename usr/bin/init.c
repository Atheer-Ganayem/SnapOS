#include <unistd.h>
#include <unistd.h>

void print_err() {
  static char err_msg[] = "error\n";
  write(STDOUT_FILENO, err_msg, sizeof(err_msg));
}

int echo_loop() {
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

int main() {
  int fd = open("test_dir/test.txt", 0, 0);
  if (fd < 0) {
    print_err();
    return 1;
  }


  char buf[512];
  ssize_t ret = read(fd, buf, sizeof(buf));
  if (ret < 0) {
    print_err();
    return 1;
  }


  if (write(STDOUT_FILENO, buf, ret) <= 0) {
    print_err();
    return 1;
  }

  return 0;
}