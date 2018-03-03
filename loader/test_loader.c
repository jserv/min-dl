#include <assert.h>
#include <stdio.h>
// #include <dlfcn.h>
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
    readelf_("./files/test_lib.so");
    const char *result_char;
    const int *result_int;
    const char *(*func)(void);
    char * (*func_char)();
    int (*func_int)();

    printf("Test exported functions >\n");

    func = lookup_symbol_by_name_("./files/test_lib.so", "foo");
    printf("func = %s\n", func());
    
    func_int = lookup_symbol_by_name_("./files/test_lib.so", "bar_int");
    printf("func_int = %d\n", func_int());

    func_char = lookup_symbol_by_name_("./files/test_lib.so", "bar");
    printf("func_char = %s\n", func_char());

    printf("OK!\n");



    printf("Test nested functions >\n");

    func_int = lookup_symbol_by_name_("./files/test_lib.so", "test_nested.2240");
    printf("test_nested = %d\n", func_int());

    func_int = lookup_symbol_by_name_("./files/test_lib.so", "test");
    printf("test = %d\n", func_int());


    printf("OK!\n");

//     char * l;
//     printf("attempting to load libc.so.6\n\n\n");
//     void * h = dlopen("/lib/libc.so.6", RTLD_LAZY);
//     l = dlerror();
//     printf("\n\n\ndlerror returned with %s\n", l);
//     dlerror();
//     printf("\n\n\nloaded libc.so.6\n");
//     printf("aquiring symbol printf\n\n\n\n");
//     int (*t)() = dlsym(h, "printf");
//     l = dlerror();
//     printf("\n\n\ndlerror returned with %s\n", l);
//     dlerror();
//     printf("\n\n\naquired symbol printf\n");
//     assert(t);
//     printf("testing that printf works\n");
//     t("do i work? %d, %s\n", 0, "i work");
//     printf("attempting to close libc.so\n");
//     dlclose(h);
//     l = dlerror();
//     printf("\n\n\ndlerror returned with %s\n", l);
//     printf("closed libc.so.6\n");


//     printf("Test libc functions >\n");
//     
//     printf("attempting to obtain init_\n");
//     int (*func_int_init_)();
//     func_int_init_ = lookup_symbol_by_name_("/lib/libc.so.6", "init_");
//     printf("attempting to call init_\n");
//     func_int_init_();
// 
//     int (*func_int_write)();
//     func_int_write = lookup_symbol_by_name_("/lib/libc.so.6", "write");
//     func_int_write(1, "write\n", 7);
// 
//     int (*func_int_strlen)();
//     func_int_strlen = lookup_symbol_by_name_("/lib/libc.so.6", "strlen");
//     printf("func_int_strlen(\"test string\\n\") = %d\n", func_int_strlen("test string\n"));
// 
//     int (*func_int_printf)();
//     func_int_printf = lookup_symbol_by_name_("/lib/libc.so.6", "printf");
//     func_int_printf("func_int_strlen(\"test string\n\")\n");
// 
//     printf("OK!\n");

    
//     printf("Test functions that call external libc functions >\n");
// 
//     func_int = lookup_symbol_by_name_("./files/test_lib.so", "test_printf");
//     result_int = func_int();
//     printf("func_int = %d\n", func_int());
// 
//     printf("OK!\n");



    return 0;
}
