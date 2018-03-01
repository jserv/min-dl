#include <assert.h>
#include <stdio.h>
// #include <string.h>
// #include <elf.h>
// #include <link.h>
// #include <unistd.h>
// #include <locale.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <sys/mman.h>
// #include <stdlib.h>
// void * getaux(void * type);
// char ** argv;

// unfortunately this is nessicary
void * lookup_symbol_by_name_(const char * lib, const char * name);

int main() {
    printf("Test exported functions >\n");
    const char *result_char;
    const int *result_int;
    const char *(*func)(void);
    func = lookup_symbol_by_name_("./files/test_lib.so", "foo");
    result_char = func();
    assert(!strcmp(result_char, "foo"));
    printf("func = %s\n", func());
    
    int (*func_int)() = lookup_symbol_by_name_("./files/test_lib.so", "bar_int");
    result_int = func_int();
    printf("func_int = %d\n", func_int());

    char * (*func_char)() = lookup_symbol_by_name_("./files/test_lib.so", "bar");
    printf("func_char = %s\n", func_char());

    printf("OK!\n");
    return 0;
}
