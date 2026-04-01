/*
 * Test harness for DT_INIT_ARRAY/DT_FINI_ARRAY constructor/destructor
 * execution.  Validates that api_run_init() fires constructors and
 * api_run_fini() fires destructors in the loaded library.
 */

#include <assert.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib-support.h"

int main(void)
{
    typedef int (*int_fn)(void);

    dloader_p o = DLoader.load("initlib.so");

    /* Resolve symbols so we can query the library's internal state */
    int_fn get_init = (int_fn) DLoader.lookup_symbol(o, "get_init_count");
    int_fn get_fini = (int_fn) DLoader.lookup_symbol(o, "get_fini_count");
    assert(get_init != NULL);
    assert(get_fini != NULL);

    printf("Test constructors (DT_INIT_ARRAY) >\n");

    /* Before run_init, constructors have not fired */
    assert(get_init() == 0);

    DLoader.run_init(o);

    int init_count = get_init();
    printf("  init_count after run_init = %d\n", init_count);
    assert(init_count == 2);

    DLoader.run_init(o);
    assert(get_init() == 2);

    printf("OK!\n");

    /*
     * Test destructors in a child process so the parent can verify
     * the fini callbacks actually ran without affecting its own state.
     */
    printf("Test destructors (DT_FINI_ARRAY) >\n");

    fflush(NULL);
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        /* Child: run fini, check count, exit with result */
        DLoader.run_fini(o);
        DLoader.run_fini(o);
        int fini_count = get_fini();
        /* Exit 0 if exactly 2 destructors ran, 1 otherwise */
        _exit(fini_count == 2 ? 0 : 1);
    }

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    printf("  fini_count after run_fini = 2 (verified in child)\n");

    printf("OK!\n");

    return 0;
}
