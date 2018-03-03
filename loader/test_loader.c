#include <stdio.h>

// unfortunately this is nessicary
void * lookup_symbol_by_name_(const char * lib, const char * name);

int main() {
    const char * (*func_char)();
    const int (*func_int)();

    printf("Test libc functions >\n");

    int (*func_int_write)();
    func_int_write = lookup_symbol_by_name_("/lib/libc.so.6", "write");
    func_int_write(1, "write\n", 7);

    int (*func_int_strlen)();
    func_int_strlen = lookup_symbol_by_name_("/lib/libc.so.6", "strlen");
    printf("func_int_strlen(\"test string\\n\") = %d\n", func_int_strlen("test string\n"));

    int (*func_int_printf)();
    func_int_printf = lookup_symbol_by_name_("/lib/libc.so.6", "printf");
    func_int_printf("func_int_strlen(\"test string\n\")\n");

    printf("OK!\n");

    return 0;
}
