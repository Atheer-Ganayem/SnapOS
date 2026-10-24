#ifndef TTY_H
#define TTY_H

struct file;
struct tty_struct;
extern struct file* tty0_file;
extern struct tty_struct* tty0;

void tty_push_char(struct tty_struct* tty, char input);

int tty_init();


#endif