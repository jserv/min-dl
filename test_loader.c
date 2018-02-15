#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "lib-support.h"
#include <errno.h>

const char *test_import0() { return __func__; }
const char *test_import1() { return __func__; }

static int resolver_call_count = 0;
static int resolver_last_id = -1;

// int test() { return __func__; }

static void *plt_resolver(void *handle, int import_id)
{
    printf("executed plt_resolver\n");
    dloader_p o = handle;
    printf("resolver executed for func #%i\n", import_id);
    resolver_call_count++;
    resolver_last_id = import_id;

    void *funcs[] = {
        (void *) test_import0, (void *) test_import1,
    };
    printf("func = %p (funcs[%d])\n", funcs[import_id], import_id);
    void *func = funcs[import_id];
    printf("calling set_plt_entry with args %p, %d, %p\n", o, import_id, func);
    DLoader.set_plt_entry(o, import_id, func);
    printf("returning %p\n", func);
    return func;
}

int main()
{
    typedef const char *(*func_t)(void);

    dloader_p o = DLoader.load("test_lib.so");
    void **func_table = DLoader.get_info(o);

    const char *(*func)(void);
    const char *result;

    printf("Test exported functions >\n");

    func = (func_t) func_table[0];
    result = func();
    assert(!strcmp(result, "foo"));

    func = (func_t) func_table[1];
    result = func();
    assert(!strcmp(result, "bar"));

    printf("OK!\n");

    printf("Test imported functions >\n");
    DLoader.set_plt_resolver(o, plt_resolver,
                             /* user_plt_resolver_handle */ o);

    printf("aquiring address of test_import0\n");
    func = (func_t) func_table[2];
    printf("aquired address of test_import0 %p\n", func);
    printf("executing func %p\n", func);
    result = func();
    printf("executed func %p\n", func);
    assert(!strcmp(result, "test_import0"));
    assert(resolver_call_count == 1);
    assert(resolver_last_id == 0);
    resolver_call_count = 0;
    printf("executing func %p\n", func);
    result = func();
    printf("executed func %p\n", func);
    assert(!strcmp(result, "test_import0"));
    assert(resolver_call_count == 0);

    printf("aquiring address of test_import1\n");
    func = (func_t) func_table[3];
    printf("aquired address of test_import1 %p\n", func);
    printf("aquiring address of test_import1\n");
    char * (*funcb)() = (func_t) func_table[3];
    printf("aquired address of test_import1 %p\n", funcb);
    printf("executing func %p\n", func);
    result = func();
    printf("executed func %p\n", func);
    assert(!strcmp(result, "test_import1"));
    printf("funcb = %p\n", (func_t) func_table[3]);
    assert(resolver_call_count == 1);
    assert(resolver_last_id == 1);
    resolver_call_count = 0;
    printf("executing func %p\n", func);
    result = func();
    printf("executed func %p\n", func);
    assert(!strcmp(result, "test_import1"));
    printf("funcb = %s\n", funcb());
    assert(resolver_call_count == 0);

    printf("OK!\n");
    printf("attempting to load test\n");
    void * h = dlopen("./test_lib.so", RTLD_LAZY);
    int (*test)() = dlsym(h, "test");
    assert(test);
    printf("loaded test\n");
    printf("test() returned %d (%p)\n", test(), dlsym(h, "test"));
    dlclose(h);
//     func = (func_t) (void *) test;
//     result = func();
//     printf("test() returned %d (%s)\n", func(), func() );
//     assert(!result == 5);

    return 0;
}
