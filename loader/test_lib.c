#include <stdio.h>

#include <assert.h>

int k = 1239;
char bar_a[4] = "bar";

static
char * bar_p = "bar_";
static
char *bar() { return bar_p ; }

// int t() { /* printf("k = %d\n", k);*/ return k; }

__attribute__((visibility("hidden")))
const int TESTTTTTTTTTTTTTT = 1234;

int __attribute__((visibility("hidden"))) test_GLOBALB = 5;

const char *foo() { return "foo"; }



int *bar_int() { return k ; }

int test_printf() { printf("test\n"); return 0; }

int test() { 
    int a;
    int test_nested() { 
        int k=5;
        return k;
    }
    int l = test_nested();
    a = l;
    return l+a;
}


//     25: 0000000000000000     0 SECTION LOCAL  DEFAULT   25                               // NULL, start of static list
//     26: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crtstuff.c
//     27: 0000000000200e08     0 OBJECT  LOCAL  DEFAULT   19 __JCR_LIST__
//     28: 0000000000000670     0 FUNC    LOCAL  DEFAULT   12 deregister_tm_clones
//     29: 00000000000006b0     0 FUNC    LOCAL  DEFAULT   12 register_tm_clones
//     30: 0000000000000700     0 FUNC    LOCAL  DEFAULT   12 __do_global_dtors_aux
//     31: 0000000000201048     1 OBJECT  LOCAL  DEFAULT   24 completed.6960
//     32: 0000000000200e00     0 OBJECT  LOCAL  DEFAULT   18 __do_global_dtors_aux_fin
//     33: 0000000000000740     0 FUNC    LOCAL  DEFAULT   12 frame_dummy
//     34: 0000000000200df8     0 OBJECT  LOCAL  DEFAULT   17 __frame_dummy_init_array_
//     35: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS test_lib.c
//     36: 0000000000201038     8 OBJECT  LOCAL  DEFAULT   23 bar_p
//     37: 0000000000000770    13 FUNC    LOCAL  DEFAULT   12 bar
//     38: 00000000000007b2    20 FUNC    LOCAL  DEFAULT   12 test_nested.2240
//     39: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS test_lib2.c
//     40: 0000000000201040     8 OBJECT  LOCAL  DEFAULT   23 bar_p
//     41: 00000000000007fa    13 FUNC    LOCAL  DEFAULT   12 bar
//     42: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crtstuff.c
//     43: 00000000000009c0     0 OBJECT  LOCAL  DEFAULT   16 __FRAME_END__
//     44: 0000000000200e08     0 OBJECT  LOCAL  DEFAULT   19 __JCR_END__
//     45: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS                                   // NULL, end of static list
//     46: 000000000000081c     4 OBJECT  LOCAL  DEFAULT   14 TESTTTTTTTTTTTTTT
