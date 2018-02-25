#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "syscall.h"

#define MAXTRIES 100

char *__randname(char *);

char *tmpnam(char *buf)
{
	static char internal[L_tmpnam];
	char s[] = "/tmp/tmpnam_XXXXXX";
	int try;
	int r;
	for (try=0; try<MAXTRIES; try++) {
		__randname(s+12);
#ifdef SYS_lstat
		r = __syscall(SYS_lstat, s, &(struct stat){0});
#else
		r = __syscall(SYS_fstatat, AT_FDCWD, s,
			&(struct stat){0}, AT_SYMLINK_NOFOLLOW);
#endif
		if (r == -ENOENT) return strcpy(buf ? buf : internal, s);
	}
	return 0;
}
