/*
 * Standard shared library for testing native symbol resolution.
 * No MDL_PLT macros -- compiled as a normal .so with -shared -fPIC.
 */

const char *sym_hello(void)
{
    return "Hello from symlib!";
}

int sym_add(int a, int b)
{
    return a + b;
}

int sym_multiply(int a, int b)
{
    return a * b;
}

/* Import: resolved by host via api_resolve_symbols */
extern int host_callback(int x);

int sym_call_host(int x)
{
    return host_callback(x);
}
