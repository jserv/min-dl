#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "pthread_impl.h"
#include <stdio.h>
__attribute__((visibility("hidden")))
long __syscall_retb(unsigned long), __syscallb(syscall_arg_t, ...),
	__syscall_cpb(syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t,
	             syscall_arg_t, syscall_arg_t, syscall_arg_t);
#define __SYSCALL_CONCAT_Xb(a,b) a##b
#define __SYSCALL_CONCATb(a,b) __SYSCALL_CONCAT_Xb(a,b)
#define __SYSCALL_DISPb(b,...) __SYSCALL_CONCATb(b,__SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)
#define __syscallb(...) __SYSCALL_DISPb(__syscall,__VA_ARGS__)
#define syscallb(...) __syscall_retb(__syscall(__VA_ARGS__))
#define __syscall_cpb(...) __SYSCALL_DISPb(__syscall_cp,__VA_ARGS__)
// #STABLE
// #headers
// rm -rfv malloc/headers/
// mkdir -p malloc/headers/arch/x86_64/bits
// cp ./arch/x86_64/bits/signal.h ./arch/x86_64/bits/posix.h ./arch/x86_64/bits/mman.h ./arch/x86_64/bits/limits.h ./arch/x86_64/bits/stdint.h malloc/headers/arch/x86_64/bits
// cp ./arch/x86_64/pthread_arch.h ./arch/x86_64/syscall_arch.h ./arch/x86_64/atomic_arch.h malloc/headers/arch/x86_64
// mkdir -p malloc/headers/arch/generic/bits
// cp ./arch/generic/bits/errno.h malloc/headers/arch/generic/bits
// mkdir -p malloc/headers/src/internal
// cp ./src/internal/futex.h ./src/internal/syscall.h ./src/internal/atomic.h ./src/internal/libc.h ./src/internal/pthread_impl.h malloc/headers/src/internal
// mkdir -p malloc/headers/obj/include/bits
// cp ./obj/include/bits/alltypes.h ./obj/include/bits/syscall.h malloc/headers/obj/include/bits
// mkdir -p malloc/headers/include/sys
// cp ./include/signal.h ./include/time.h ./include/sched.h ./include/pthread.h ./include/unistd.h ./include/strings.h ./include/string.h ./include/stdarg.h ./include/malloc.h ./include/limits.h ./include/stdio.h ./include/alloca.h ./include/stdlib.h ./include/features.h ./include/errno.h ./include/stdint.h malloc/headers/include
// cp ./include/sys/syscall.h ./include/sys/mman.h malloc/headers/include/sys
// cd malloc
// #malloc.so
// gcc -shared -o malloc_STABLE.so -std=c99 -nostdinc -ffreestanding -fexcess-precision=standard -frounding-math -Wa,--noexecstack -D_XOPEN_SOURCE=700 -I./headers/arch/x86_64 -I./headers/arch/generic -I./headers/src/internal -I./headers/obj/include -I./headers/include  -Os -pipe -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Werror=implicit-function-declaration -Werror=implicit-int -Werror=pointer-sign -Werror=pointer-arith  -Wl,--sort-section,alignment -Wl,--sort-common -Wl,--gc-sections -Wl,--hash-style=both -Wl,--no-undefined -Wl,--exclude-libs=ALL -Wl,-Bsymbolic-functions  -nostdlib malloc_STABLE.c -fPIC
// #malloc
// gcc -o malloc_STABLE -I./headers/arch/x86_64 -I./headers/arch/generic -I./headers/obj/src/internal -I./headers/src/internal -I./headers/obj/include -I./headers/include malloc_STABLE.c
// cd ../


// #UNSTABLE
// #malloc.so
// cd malloc
// gcc -shared -o malloc_UNSTABLE.so -std=c99 -nostdinc -ffreestanding -fexcess-precision=standard -frounding-math -Wa,--noexecstack -D_XOPEN_SOURCE=700 -Os -pipe -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Werror=implicit-function-declaration -Werror=implicit-int -Werror=pointer-sign -Werror=pointer-arith  -Wl,--sort-section,alignment -Wl,--sort-common -Wl,--gc-sections -Wl,--hash-style=both -Wl,--no-undefined -Wl,--exclude-libs=ALL -Wl,-Bsymbolic-functions  -nostdlib malloc_UNSTABLE.c -fPIC
// 
// #malloc
// 
// gcc -o malloc_UNSTABLE malloc_UNSTABLE.c
// cd ../

// seems to work in place of memcpy.s but cannot be sure it is 100% sufficent as memcpy.s may be talored specifically to this implimentation


static void *
memcpy_b(void *dest, const void *src, unsigned long n)
{
    // printf("memcpy_b()\n");
	unsigned long i;
	unsigned char *d = (unsigned char *)dest;
	unsigned char *s = (unsigned char *)src;

	for (i = 0; i < n; ++i)
		d[i] = s[i];

	return dest;
}

static void *__memalignb(size_t, size_t);

static void *aligned_allocb(size_t align, size_t len)
{
    // printf("aligned_alloc()\n");
	return __memalignb(align, len);
}

static uintptr_t __brk_b(uintptr_t newbrk)
{
    // printf("__brk_()\n");
	return __syscallb(SYS_brk, newbrk);
}


/* This function returns true if the interval [old,new]
 * intersects the 'len'-sized interval below &libc.auxv
 * (interpreted as the main-thread stack) or below &b
 * (the current stack). It is used to defend against
 * buggy brk implementations that can cross the stack. */

static int traverses_stack_pb(uintptr_t old, uintptr_t new)
{
    // printf("traverses_stack_p()\n");
	const uintptr_t len = 8<<20;
	uintptr_t a, b;

	b = (uintptr_t)libc.auxv;
	a = b > len ? b-len : 0;
	if (new>a && old<b) return 1;

	b = (uintptr_t)&b;
	a = b > len ? b-len : 0;
	if (new>a && old<b) return 1;

	return 0;
}

static void *__mmap_b(void *, size_t, int, int, int, off_t);

/* Expand the heap in-place if brk can be used, or otherwise via mmap,
 * using an exponential lower bound on growth by mmap to make
 * fragmentation asymptotically irrelevant. The size argument is both
 * an input and an output, since the caller needs to know the size
 * allocated, which will be larger than requested due to page alignment
 * and mmap minimum size rules. The caller is responsible for locking
 * to prevent concurrent calls. */

static void *__expand_heapb(size_t *pn)
{
    // printf("__expand_heapb()\n");
	static uintptr_t brk;
	static unsigned mmap_step;
	size_t n = *pn;

	if (n > SIZE_MAX/2 - PAGE_SIZE) {
		errno = ENOMEM;
		return 0;
	}
	n += -n & PAGE_SIZE-1;

	if (!brk) {
		brk = __syscallb(SYS_brk, 0);
		brk += -brk & PAGE_SIZE-1;
	}

	if (n < SIZE_MAX-brk && !traverses_stack_pb(brk, brk+n)
	    && __syscallb(SYS_brk, brk+n)==brk+n) {
		*pn = n;
		brk += n;
		return (void *)(brk-n);
	}

	size_t min = (size_t)PAGE_SIZE << mmap_step/2;
	if (n < min) n = min;
	void *area = __mmap_b(0, n, PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (area == MAP_FAILED) return 0;
	*pn = n;
	mmap_step++;
	return area;
}

#if defined(__GNUC__) && defined(__PIC__)
#define inline inline __attribute__((always_inline))
#endif

static void *__mmap_b(void *, size_t, int, int, int, off_t);
static int __munmapb(void *, size_t);
static void *__mremapb(void *, size_t, size_t, int, ...);
static int __madviseb(void *, size_t, int);

struct chunk {
	size_t psize, csize;
	struct chunk *next, *prev;
};

struct bin {
	volatile int lock[2];
	struct chunk *head;
	struct chunk *tail;
};

struct {
	volatile uint64_t binmap;
	struct bin bins[64];
	volatile int free_lock[2];
} mal;


#define SIZE_ALIGN (4*sizeof(size_t))
#define SIZE_MASK (-SIZE_ALIGN)
#define OVERHEAD (2*sizeof(size_t))
#define MMAP_THRESHOLD (0x1c00*SIZE_ALIGN)
#define DONTCARE 16
#define RECLAIM 163840

#define CHUNK_SIZE(c) ((c)->csize & -2)
#define CHUNK_PSIZE(c) ((c)->psize & -2)
#define PREV_CHUNK(c) ((struct chunk *)((char *)(c) - CHUNK_PSIZE(c)))
#define NEXT_CHUNK(c) ((struct chunk *)((char *)(c) + CHUNK_SIZE(c)))
#define MEM_TO_CHUNK(p) (struct chunk *)((char *)(p) - OVERHEAD)
#define CHUNK_TO_MEM(c) (void *)((char *)(c) + OVERHEAD)
#define BIN_TO_CHUNK(i) (MEM_TO_CHUNK(&mal.bins[i].head))

#define C_INUSE  ((size_t)1)

#define IS_MMAPPED(c) !((c)->csize & (C_INUSE))


/* Synchronization tools */

static inline void lockb(volatile int *lk)
{
    // printf("lockb()\n");
	if (libc.threads_minus_1)
		while(a_swap(lk, 1)) __wait(lk, lk+1, 1, 1);
}

static inline void unlockb(volatile int *lk)
{
    // printf("unlockb()\n");
	if (lk[0]) {
		a_store(lk, 0);
		if (lk[1]) __wake(lk, 1, 1);
	}
}

static inline void lock_binb(int i)
{
    // printf("lock_binb()\n");
	lockb(mal.bins[i].lock);
	if (!mal.bins[i].head)
		mal.bins[i].head = mal.bins[i].tail = BIN_TO_CHUNK(i);
}

static inline void unlock_binb(int i)
{
    // printf("unlock_binb()\n");
	unlockb(mal.bins[i].lock);
}

static int first_setb(uint64_t x)
{
    // printf("first_set()\n");
#if 1
	return a_ctz_64(x);
#else
	static const char debruijn64[64] = {
		0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
		62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
	};
	static const char debruijn32[32] = {
		0, 1, 23, 2, 29, 24, 19, 3, 30, 27, 25, 11, 20, 8, 4, 13,
		31, 22, 28, 18, 26, 10, 7, 12, 21, 17, 9, 6, 16, 5, 15, 14
	};
	if (sizeof(long) < 8) {
		uint32_t y = x;
		if (!y) {
			y = x>>32;
			return 32 + debruijn32[(y&-y)*0x076be629 >> 27];
		}
		return debruijn32[(y&-y)*0x076be629 >> 27];
	}
	return debruijn64[(x&-x)*0x022fdd63cc95386dull >> 58];
#endif
}

static const unsigned char bin_tab[60] = {
	            32,33,34,35,36,36,37,37,38,38,39,39,
	40,40,40,40,41,41,41,41,42,42,42,42,43,43,43,43,
	44,44,44,44,44,44,44,44,45,45,45,45,45,45,45,45,
	46,46,46,46,46,46,46,46,47,47,47,47,47,47,47,47,
};

static int bin_indexb(size_t x)
{
    // printf("bin_indexb()\n");
	x = x / SIZE_ALIGN - 1;
	if (x <= 32) return x;
	if (x < 512) return bin_tab[x/8-4];
	if (x > 0x1c00) return 63;
	return bin_tab[x/128-4] + 16;
}

static int bin_index_upb(size_t x)
{
    // printf("bin_index_up()\n");
	x = x / SIZE_ALIGN - 1;
	if (x <= 32) return x;
	x--;
	if (x < 512) return bin_tab[x/8-4] + 1;
	return bin_tab[x/128-4] + 17;
}

#if 0
void __dump_heapb(int x)
{
    // printf("__dump_heap()\n");
	struct chunk *c;
	int i;
	for (c = (void *)mal.heap; CHUNK_SIZE(c); c = NEXT_CHUNK(c))
		f// printf(stderr, "base %p size %zu (%d) flags %d/%d\n",
			c, CHUNK_SIZE(c), bin_indexb(CHUNK_SIZE(c)),
			c->csize & 15,
			NEXT_CHUNK(c)->psize & 15);
	for (i=0; i<64; i++) {
		if (mal.bins[i].head != BIN_TO_CHUNK(i) && mal.bins[i].head) {
			f// printf(stderr, "bin %d: %p\n", i, mal.bins[i].head);
			if (!(mal.binmap & 1ULL<<i))
				f// printf(stderr, "missing from binmap!\n");
		} else if (mal.binmap & 1ULL<<i)
			f// printf(stderr, "binmap wrongly contains %d!\n", i);
	}
}
#endif

static void *__expand_heapb(size_t *);

static struct chunk *expand_heapb(size_t n)
{
	static int heap_lock[2];
	static void *end;
	void *p;
	struct chunk *w;

	/* The argument n already accounts for the caller's chunk
	 * overhead needs, but if the heap can't be extended in-place,
	 * we need room for an extra zero-sized sentinel chunk. */
	n += SIZE_ALIGN;

	lockb(heap_lock);

	p = __expand_heapb(&n);
	if (!p) {
		unlockb(heap_lock);
		return 0;
	}

	/* If not just expanding existing space, we need to make a
	 * new sentinel chunk below the allocated space. */
	if (p != end) {
		/* Valid/safe because of the prologue increment. */
		n -= SIZE_ALIGN;
		p = (char *)p + SIZE_ALIGN;
		w = MEM_TO_CHUNK(p);
		w->psize = 0 | C_INUSE;
	}

	/* Record new heap end and fill in footer. */
	end = (char *)p + n;
	w = MEM_TO_CHUNK(end);
	w->psize = n | C_INUSE;
	w->csize = 0 | C_INUSE;

	/* Fill in header, which may be new or may be replacing a
	 * zero-size sentinel header at the old end-of-heap. */
	w = MEM_TO_CHUNK(p);
	w->csize = n | C_INUSE;

	unlockb(heap_lock);

	return w;
}

static int adjust_sizeb(size_t *n)
{
    // printf("adjust_sizeb()\n");
	/* Result of pointer difference must fit in ptrdiff_t. */
	if (*n-1 > PTRDIFF_MAX - SIZE_ALIGN - PAGE_SIZE) {
		if (*n) {
			errno = ENOMEM;
			return -1;
		} else {
			*n = SIZE_ALIGN;
			return 0;
		}
	}
	*n = (*n + OVERHEAD + SIZE_ALIGN - 1) & SIZE_MASK;
	return 0;
}

static void unbinb(struct chunk *c, int i)
{
    // printf("unbinb()\n");
	if (c->prev == c->next)
		a_and_64(&mal.binmap, ~(1ULL<<i));
	c->prev->next = c->next;
	c->next->prev = c->prev;
	c->csize |= C_INUSE;
	NEXT_CHUNK(c)->psize |= C_INUSE;
}

static int alloc_fwdb(struct chunk *c)
{
    // printf("alloc_fwdb()\n");
	int i;
	size_t k;
	while (!((k=c->csize) & C_INUSE)) {
		i = bin_indexb(k);
		lock_binb(i);
		if (c->csize == k) {
			unbinb(c, i);
			unlock_binb(i);
			return 1;
		}
		unlock_binb(i);
	}
	return 0;
}

static int alloc_revb(struct chunk *c)
{
    // printf("alloc_revb()\n");
	int i;
	size_t k;
	while (!((k=c->psize) & C_INUSE)) {
		i = bin_indexb(k);
		lock_binb(i);
		if (c->psize == k) {
			unbinb(PREV_CHUNK(c), i);
			unlock_binb(i);
			return 1;
		}
		unlock_binb(i);
	}
	return 0;
}


/* pretrim - trims a chunk _prior_ to removing it from its bin.
 * Must be called with i as the ideal bin for size n, j the bin
 * for the _free_ chunk self, and bin j locked. */
static int pretrimb(struct chunk *self, size_t n, int i, int j)
{
    // printf("pretrimb()\n");
	size_t n1;
	struct chunk *next, *split;

	/* We cannot pretrim if it would require re-binning. */
	if (j < 40) return 0;
	if (j < i+3) {
		if (j != 63) return 0;
		n1 = CHUNK_SIZE(self);
		if (n1-n <= MMAP_THRESHOLD) return 0;
	} else {
		n1 = CHUNK_SIZE(self);
	}
	if (bin_indexb(n1-n) != j) return 0;

	next = NEXT_CHUNK(self);
	split = (void *)((char *)self + n);

	split->prev = self->prev;
	split->next = self->next;
	split->prev->next = split;
	split->next->prev = split;
	split->psize = n | C_INUSE;
	split->csize = n1-n;
	next->psize = n1-n;
	self->csize = n | C_INUSE;
	return 1;
}
void freeb(void *p);
static void trimb(struct chunk *self, size_t n)
{
    // printf("trimb()\n");
	size_t n1 = CHUNK_SIZE(self);
	struct chunk *next, *split;

	if (n >= n1 - DONTCARE) return;

	next = NEXT_CHUNK(self);
	split = (void *)((char *)self + n);

	split->psize = n | C_INUSE;
	split->csize = n1-n | C_INUSE;
	next->psize = n1-n | C_INUSE;
	self->csize = n | C_INUSE;

	freeb(CHUNK_TO_MEM(split));
}

void *mallocb(size_t n)
{
    // printf("mallocb()\n");
	struct chunk *c;
	int i, j;

	if (adjust_sizeb(&n) < 0) return 0;

	if (n > MMAP_THRESHOLD) {
		size_t len = n + OVERHEAD + PAGE_SIZE - 1 & -PAGE_SIZE;
		char *base = __mmap_b(0, len, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (base == (void *)-1) return 0;
		c = (void *)(base + SIZE_ALIGN - OVERHEAD);
		c->csize = len - (SIZE_ALIGN - OVERHEAD);
		c->psize = SIZE_ALIGN - OVERHEAD;
		return CHUNK_TO_MEM(c);
	}

	i = bin_index_upb(n);
	for (;;) {
		uint64_t mask = mal.binmap & -(1ULL<<i);
		if (!mask) {
			c = expand_heapb(n);
			if (!c) return 0;
			if (alloc_revb(c)) {
				struct chunk *x = c;
				c = PREV_CHUNK(c);
				NEXT_CHUNK(x)->psize = c->csize =
					x->csize + CHUNK_SIZE(c);
			}
			break;
		}
		j = first_setb(mask);
		lock_binb(j);
		c = mal.bins[j].head; // c gets set to 0x0 wich should not happen
		if (c != BIN_TO_CHUNK(j)) { // c gets set to 0x0 wich should not happen
			if (!pretrimb(c, n, i, j)) unbinb(c, j);
			unlock_binb(j);
			break;
		}
		unlock_binb(j);
	}

	/* Now patch up in case we over-allocated */
	trimb(c, n);

	return CHUNK_TO_MEM(c);
}

static void *__malloc0b(size_t n)
{
    // printf("__malloc0b()\n");
	void *p = mallocb(n);
	if (p && !IS_MMAPPED(MEM_TO_CHUNK(p))) {
		size_t *z;
		n = (n + sizeof *z - 1)/sizeof *z;
		for (z=p; n; n--, z++) if (*z) *z=0;
	}
	return p;
}

void *reallocb(void *p, size_t n)
{
    // printf("realloc()\n");
	struct chunk *self, *next;
	size_t n0, n1;
	void *new;

	if (!p) return mallocb(n);

	if (adjust_sizeb(&n) < 0) return 0;

	self = MEM_TO_CHUNK(p);
	n1 = n0 = CHUNK_SIZE(self);

	if (IS_MMAPPED(self)) {
		size_t extra = self->psize;
		char *base = (char *)self - extra;
		size_t oldlen = n0 + extra;
		size_t newlen = n + extra;
		/* Crash on realloc of freed chunk */
		if (extra & 1) a_crash();
		if (newlen < PAGE_SIZE && (new = mallocb(n))) {
			memcpy_b(new, p, n-OVERHEAD);
			freeb(p);
			return new;
		}
		newlen = (newlen + PAGE_SIZE-1) & -PAGE_SIZE;
		if (oldlen == newlen) return p;
		base = __mremapb(base, oldlen, newlen, MREMAP_MAYMOVE);
		if (base == (void *)-1)
			goto copy_realloc;
		self = (void *)(base + extra);
		self->csize = newlen - extra;
		return CHUNK_TO_MEM(self);
	}

	next = NEXT_CHUNK(self);

	/* Crash on corrupted footer (likely from buffer overflow) */
	if (next->psize != self->csize) a_crash();

	/* Merge adjacent chunks if we need more space. This is not
	 * a waste of time even if we fail to get enough space, because our
	 * subsequent call to free would otherwise have to do the merge. */
	if (n > n1 && alloc_fwdb(next)) {
		n1 += CHUNK_SIZE(next);
		next = NEXT_CHUNK(next);
	}
	/* FIXME: find what's wrong here and reenable it..? */
	if (0 && n > n1 && alloc_revb(self)) {
		self = PREV_CHUNK(self);
		n1 += CHUNK_SIZE(self);
	}
	self->csize = n1 | C_INUSE;
	next->psize = n1 | C_INUSE;

	/* If we got enough space, split off the excess and return */
	if (n <= n1) {
		//memmove(CHUNK_TO_MEM(self), p, n0-OVERHEAD);
		trimb(self, n);
		return CHUNK_TO_MEM(self);
	}
#define ALIGN 16

void *__expand_heapb(size_t *);

// 
copy_realloc:
    // printf("copy_realloc:\n");
	/* As a last resort, allocate a new chunk and copy to it. */
	new = mallocb(n-OVERHEAD);
	if (!new) return 0;
	memcpy_b(new, p, n0-OVERHEAD);
	freeb(CHUNK_TO_MEM(self));
	return new;
}

void freeb(void *p)
{
    // printf("freeb()\n");
	struct chunk *self, *next;
	size_t final_size, new_size, size;
	int reclaim=0;
	int i;

	if (!p) return;

	self = MEM_TO_CHUNK(p);

	if (IS_MMAPPED(self)) {
		size_t extra = self->psize;
		char *base = (char *)self - extra;
		size_t len = CHUNK_SIZE(self) + extra;
		/* Crash on double free */
		if (extra & 1) a_crash();
		__munmapb(base, len);
		return;
	}

	final_size = new_size = CHUNK_SIZE(self);
	next = NEXT_CHUNK(self);

	/* Crash on corrupted footer (likely from buffer overflow) */
	if (next->psize != self->csize) a_crash();

	for (;;) {
		if (self->psize & next->csize & C_INUSE) {
			self->csize = final_size | C_INUSE;
			next->psize = final_size | C_INUSE;
			i = bin_indexb(final_size);
			lock_binb(i);
			lockb(mal.free_lock);
			if (self->psize & next->csize & C_INUSE)
				break;
			unlockb(mal.free_lock);
			unlock_binb(i);
		}

		if (alloc_revb(self)) {
			self = PREV_CHUNK(self);
			size = CHUNK_SIZE(self);
			final_size += size;
			if (new_size+size > RECLAIM && (new_size+size^size) > size)
				reclaim = 1;
		}

		if (alloc_fwdb(next)) {
			size = CHUNK_SIZE(next);
			final_size += size;
			if (new_size+size > RECLAIM && (new_size+size^size) > size)
				reclaim = 1;
			next = NEXT_CHUNK(next);
		}
	}

	if (!(mal.binmap & 1ULL<<i))
		a_or_64(&mal.binmap, 1ULL<<i);

	self->csize = final_size;
	next->psize = final_size;
	unlockb(mal.free_lock);

	self->next = BIN_TO_CHUNK(i);
	self->prev = mal.bins[i].tail;
	self->next->prev = self;
	self->prev->next = self;

	/* Replace middle of large chunks with fresh zero pages */
	if (reclaim) {
		uintptr_t a = (uintptr_t)self + SIZE_ALIGN+PAGE_SIZE-1 & -PAGE_SIZE;
		uintptr_t b = (uintptr_t)next - SIZE_ALIGN & -PAGE_SIZE;
#if 1
		__madviseb((void *)a, b-a, MADV_DONTNEED);
#else
		__mmap_b((void *)a, b-a, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
#endif
	}

	unlock_binb(i);
}

static void *(*const __realloc_dep)(void *, size_t) = reallocb;

#define OVERHEAD (2*sizeof(size_t))
#define CHUNK_SIZE(c) ((c)->csize & -2)
#define MEM_TO_CHUNK(p) (struct chunk *)((char *)(p) - OVERHEAD)

static size_t malloc_usable_sizeb(void *p)
{
    // printf("malloc_usable_size()\n");
	return p ? CHUNK_SIZE(MEM_TO_CHUNK(p)) - OVERHEAD : 0;
}

/* This function should work with most dlmalloc-like chunk bookkeeping
 * systems, but it's only guaranteed to work with the native implementation
 * used in this library. */

static void *__memalignb(size_t align, size_t len)
{
    // printf("__memalignb()\n");
	unsigned char *mem, *new, *end;
	size_t header, footer;

	if ((align & -align) != align) {
		errno = EINVAL;
		return NULL;
	}

	if (len > SIZE_MAX - align) {
		errno = ENOMEM;
		return NULL;
	}

	if (align <= 4*sizeof(size_t)) {
		if (!(mem = mallocb(len)))
			return NULL;
		return mem;
	}

	if (!(mem = mallocb(len + align-1)))
		return NULL;

	new = (void *)((uintptr_t)mem + align-1 & -align);
	if (new == mem) return mem;

	header = ((size_t *)mem)[-1];

	if (!(header & 7)) {
		((size_t *)new)[-2] = ((size_t *)mem)[-2] + (new-mem);
		((size_t *)new)[-1] = ((size_t *)mem)[-1] - (new-mem);
		return new;
	}

	end = mem + (header & -8);
	footer = ((size_t *)end)[-2];

	((size_t *)mem)[-1] = header&7 | new-mem;
	((size_t *)new)[-2] = footer&7 | new-mem;
	((size_t *)new)[-1] = header&7 | end-new;
	((size_t *)end)[-2] = footer&7 | end-new;

	freeb(mem);
	return new;
}

weak_alias(__memalignb, memalignb);

static void *__memalignb(size_t, size_t);

static int posix_memalignb(void **res, size_t align, size_t len)
{
    // printf("posix_memalign()\n");
	if (align < sizeof(void *)) return EINVAL;
	void *mem = __memalignb(align, len);
	if (!mem) return errno;
	*res = mem;
	return 0;
}

static int *__errno_locationb(void)
{
    // printf("__errno_location()\n");
	return &__pthread_self()->errno_val;
}

static void dummy(void) { }
weak_alias(dummy, __vm_wait);

#define UNIT SYSCALL_MMAP2_UNIT
#define OFF_MASK ((-0x2000ULL << (8*sizeof(syscall_arg_t)-1)) | (UNIT-1))

static void *__mmap_b(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
    // printf("__mmap_b()\n");
	long ret;
	if (off & OFF_MASK) {
		errno = EINVAL;
		return MAP_FAILED;
	}
	if (len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
	}
	if (flags & MAP_FIXED) {
		__vm_wait();
	}
#ifdef SYS_mmap2
	ret = __syscallb(SYS_mmap2, start, len, prot, flags, fd, off/UNIT);
#else
	ret = __syscallb(SYS_mmap, start, len, prot, flags, fd, off);
#endif
	/* Fixup incorrect EPERM from kernel. */
	if (ret == -EPERM && !start && (flags&MAP_ANON) && !(flags&MAP_FIXED))
		ret = -ENOMEM;
	return (void *)__syscall_retb(ret);
}

weak_alias(__mmap_b, mmapb);

// LFS64(mmap);

struct __libc __libc;

size_t __hwcap;
size_t __sysinfo;
char *__progname_=0, *__progname__full=0;

weak_alias(__progname_, program_invocation_short_name);
weak_alias(__progname__full, program_invocation_name);

static void __waitb(volatile int *addr, volatile int *waiters, int val, int priv)
{
    // printf("__wait()\n");
	int spins=100;
	if (priv) priv = FUTEX_PRIVATE;
	while (spins-- && (!waiters || !*waiters)) {
		if (*addr==val) a_spin();
		else return;
	}
	if (waiters) a_inc(waiters);
	while (*addr==val) {
		__syscallb(SYS_futex, addr, FUTEX_WAIT|priv, val, 0) != -ENOSYS
		|| __syscallb(SYS_futex, addr, FUTEX_WAIT, val, 0);
	}
	if (waiters) a_dec(waiters);
}

static int __madviseb(void *addr, size_t len, int advice)
{
    // printf("__madviseb()\n");
	return syscallb(SYS_madvise, addr, len, advice);
}

weak_alias(__madviseb, madviseb);

static int __munmapb(void *start, size_t len)
{
    // printf("__munmapb()\n");
	__vm_wait();
	return syscallb(SYS_munmap, start, len);
}

weak_alias(__munmapb, munmapb);

static void *__mremapb(void *old_addr, size_t old_len, size_t new_len, int flags, ...)
{
    // printf("__mremapb()\n");
	va_list ap;
	void *new_addr = 0;

	if (new_len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
	}

	if (flags & MREMAP_FIXED) {
		__vm_wait();
		va_start(ap, flags);
		new_addr = va_arg(ap, void *);
		va_end(ap);
	}

	return (void *)syscallb(SYS_mremap, old_addr, old_len, new_len, flags, new_addr);
}

weak_alias(__mremapb, mremapb);

long __syscall_retb(unsigned long r)
{
    // printf("__syscall_ret()\n");
	if (r > -4096UL) {
		errno = -r;
		return -1;
	}
	return r;
}

// does not appear to be used
// static void *__simple_mallocb(size_t n)
// {
// 	static char *cur, *end;
// 	static volatile int lock[2];
// 	size_t align=1, pad;
// 	void *p;
// 
// 	if (!n) n++;
// 	while (align<n && align<ALIGN)
// 		align += align;
// 
// 	LOCK(lock);
// 
// 	pad = -(uintptr_t)cur & align-1;
// 
// 	if (n <= SIZE_MAX/2 + ALIGN) n += pad;
// 
// 	if (n > end-cur) {
// 		size_t m = n;
// 		char *new = __expand_heapb(&m);
// 		if (!new) {
// 			UNLOCK(lock);
// 			return 0;
// 		}
// 		if (new != end) {
// 			cur = new;
// 			n -= pad;
// 			pad = 0;
// 		}
// 		end = new + m;
// 	}
// 
// 	p = cur + pad;
// 	cur += n;
// 	UNLOCK(lock);
// 	return p;
// }

void *callocb(size_t m, size_t n)
{
    // printf("calloc()\n");
	if (n && m > (size_t)-1/n) {
		errno = ENOMEM;
		return 0;
	}
	return __malloc0b(n * m);
}
