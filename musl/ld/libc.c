#include "libc.h"

struct __libc __libc;

size_t __hwcap;
size_t __sysinfo;
char *__prognameb=0, *__progname_fullb=0;

weak_alias(__prognameb, program_invocation_short_name);
weak_alias(__progname_fullb, program_invocation_name);
