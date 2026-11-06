#ifndef _ERRNO_H
#define _ERRNO_H

extern int errno;

#define EPERM            1      // Operation not permitted
#define ENOENT           2      // No entity
#define EIO              5
#define EBADF            9
#define EGAIN           11
#define ENOMEM          12
#define EACCES          13      // Permission denied
#define EFAULT          14      // Bad address (Page Fault equivalent)
#define EEXIST          17      // File exists
#define ENODEV          19
#define ENOTDIR         20      // Not a directory
#define EINVAL          22      // Invalid argument
#define ENOTTY          25
#define ENOSYS          38
#define EUCLEAN         117     // fs corruption

#endif