#include <dlfcn.h>
#include "libc.h"

__attribute__((__visibility__("hidden")))
void __dl_seterr(const char *, ...);

static void *stub_dlsym(void *restrict p, const char *restrict s, void *restrict ra)
{
	__dl_seterr("Symbol not found: %s", s);
	return 0;
}

weak_alias(stub_dlsym, __dlsym);
