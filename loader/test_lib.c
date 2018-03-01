#include <stdio.h>

#include <assert.h>

int k = 1239;

// int t() { /* printf("k = %d\n", k);*/ return k; }

__attribute__((visibility("hidden")))
const int TESTTTTTTTTTTTTTT = 1234;

int __attribute__((visibility("hidden"))) test_GLOBALB = 5;

const char *foo() { return "foo"; }

char * bar_ = "bar";

__attribute__((visibility("hidden")))
const char *bar() { return "bar" ; }

int *bar_int() { return k ; }
