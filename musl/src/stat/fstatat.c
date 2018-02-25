#include <sys/stat.h>
#include "syscall.h"
#include "libc.h"

int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag)
{
	return syscall(SYS_fstatat, fd, path, buf, flag);
}

LFS64(fstatat);
