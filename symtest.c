/*
 * Test harness for native symbol resolution (issue #1).
 * Validates api_lookup_symbol and api_resolve_symbols on a standard .so
 * built without MDL_PLT macros.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lib-support.h"

static int my_host_callback(int x)
{
    return x * 10;
}

static void *sym_resolver(void *handle, const char *name)
{
    (void) handle;
    if (strcmp(name, "host_callback") == 0)
        return (void *) my_host_callback;
    return NULL;
}

int main(void)
{
    typedef const char *(*hello_fn)(void);
    typedef int (*binop_fn)(int, int);
    typedef int (*unary_fn)(int);

    dloader_p o = DLoader.load("symlib.so");

    printf("Test symbol lookup >\n");

    /* Look up exported symbols by name */
    hello_fn hello = (hello_fn) DLoader.lookup_symbol(o, "sym_hello");
    assert(hello != NULL);
    const char *msg = hello();
    printf("  sym_hello() = \"%s\"\n", msg);
    assert(strcmp(msg, "Hello from symlib!") == 0);

    binop_fn add = (binop_fn) DLoader.lookup_symbol(o, "sym_add");
    assert(add != NULL);
    assert(add(3, 4) == 7);
    printf("  sym_add(3, 4) = %d\n", add(3, 4));

    binop_fn mul = (binop_fn) DLoader.lookup_symbol(o, "sym_multiply");
    assert(mul != NULL);
    assert(mul(5, 6) == 30);
    printf("  sym_multiply(5, 6) = %d\n", mul(5, 6));

    /* Non-existent symbol returns NULL */
    assert(DLoader.lookup_symbol(o, "nonexistent") == NULL);
    printf("  lookup(\"nonexistent\") = NULL\n");

    printf("OK!\n");

    printf("Test symbol resolution (imports) >\n");

    /* Resolve external symbols in symlib.so */
    int ret = DLoader.resolve_symbols(o, sym_resolver, NULL);
    assert(ret == 0);

    /* Now call a function that uses the resolved import */
    unary_fn call_host = (unary_fn) DLoader.lookup_symbol(o, "sym_call_host");
    assert(call_host != NULL);
    int result = call_host(7);
    printf("  sym_call_host(7) = %d\n", result);
    assert(result == 70);

    printf("OK!\n");

    return 0;
}
