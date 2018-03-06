#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lib-support.h"

const char *test_import0() { return __func__; }
const char *test_import1() { return __func__; }

static int resolver_call_count = 0;
static int resolver_last_id = -1;

static void *plt_resolver(void *handle, int import_id)
{
    dloader_p o = handle;
    printf("resolver called for func #%i\n", import_id);
    resolver_call_count++;
    resolver_last_id = import_id;

    void *funcs[] = {
        (void *) test_import0, (void *) test_import1,
    };
    void *func = funcs[import_id];
    DLoader.set_plt_entry(o, import_id, func);
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

    func = (func_t) func_table[2];
    result = func();
    assert(!strcmp(result, "test_import0"));
    assert(resolver_call_count == 1);
    assert(resolver_last_id == 0);
    resolver_call_count = 0;
    result = func();
    assert(!strcmp(result, "test_import0"));
    assert(resolver_call_count == 0);

    func = (func_t) func_table[3];
    result = func();
    assert(!strcmp(result, "test_import1"));
    assert(resolver_call_count == 1);
    assert(resolver_last_id == 1);
    resolver_call_count = 0;
    result = func();
    assert(!strcmp(result, "test_import1"));
    assert(resolver_call_count == 0);

    printf("OK!\n");

    return 0;
}
