#include <stdio.h>

#include <assert.h>

int k = 1239;
char bar_a[4] = "bar";
char * bar_p = "bar_";

// int t() { /* printf("k = %d\n", k);*/ return k; }

__attribute__((visibility("hidden")))
const int TESTTTTTTTTTTTTTT = 1234;

int __attribute__((visibility("hidden"))) test_GLOBALB = 5;

const char *foo() { return "foo"; }


__attribute__((visibility("hidden")))
char *bar() { return bar_p ; }

int *bar_int() { return k ; }

char * test() { printf("test\n"); }
