/*
 * Shared library for testing DT_INIT_ARRAY/DT_FINI_ARRAY execution.
 * Uses __attribute__((constructor)) and __attribute__((destructor)) which
 * the compiler turns into DT_INIT_ARRAY/DT_FINI_ARRAY entries.
 *
 * The library records the order of constructor/destructor calls into
 * a shared log buffer so the test harness can verify correct sequencing.
 */

static int init_count;
static int fini_count;

int get_init_count(void)
{
    return init_count;
}

int get_fini_count(void)
{
    return fini_count;
}

__attribute__((constructor)) static void ctor_one(void)
{
    init_count++;
}

__attribute__((constructor)) static void ctor_two(void)
{
    init_count++;
}

__attribute__((destructor)) static void dtor_one(void)
{
    fini_count++;
}

__attribute__((destructor)) static void dtor_two(void)
{
    fini_count++;
}
