#include "lib-support.h"

const char *foo() { return __func__; }
const char *bar() { return __func__; }

const char *import_func0();
const char *import_func1();

MDL_PLT_BEGIN;
MDL_PLT_ENTRY(0, import_func0);
MDL_PLT_ENTRY(1, import_func1);

const char *test_import0()
{
    return import_func0();
}

const char *test_import1()
{
    return import_func1();
}

void *func_table[] = {
    foo, bar,
    test_import0, test_import1,
};

MDL_DEFINE_HEADER(func_table);
