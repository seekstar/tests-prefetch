#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifndef PRIszt
// POSIX
#define PRIszt "zu"
// Windows is "Iu"
#endif

static inline void my_prefetchnta(const void *x) {
	asm volatile("prefetchnta %0" : : "m" (*(const char *)x));
}
static inline void my_prefetcht0(const void *x) {
	asm volatile("prefetcht0 %0" : : "m" (*(const char *)x));
}
static inline void my_prefetcht1(const void *x) {
	asm volatile("prefetcht1 %0" : : "m" (*(const char *)x));
}
static inline void my_prefetcht2(const void *x) {
	asm volatile("prefetcht2 %0" : : "m" (*(const char *)x));
}

// Used in NOVA
static inline void my_clflush(const void *x) {
	asm volatile("clflush %0" : "+m" (*(volatile char *)(x)));
}

static inline void my_clflush_ro(const void *x) {
	asm volatile("clflush %0" : : "m" (*(volatile char *)(x)));
}

static inline uint64_t timespec_to_ns(const struct timespec *t) {
	return (uint64_t)t->tv_sec * 1000000000 + t->tv_nsec;
}
static inline uint64_t timestamp_ns() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return timespec_to_ns(&t);
}

#define SIZE (1 << 30)
#define PAGE_SIZE 4096

void prefetcht0_next_block_stride(const char *s, const char *buf, size_t num, size_t stride) {
	assert(num * stride <= PAGE_SIZE);
	assert(stride % 64 == 0);
	uint64_t prefetch_time = 0;
	uint64_t memcmp_time = 0;
	for (size_t i = 0; i < SIZE - PAGE_SIZE; i += PAGE_SIZE) {
		uint64_t start64 = timestamp_ns();
		for (size_t j = 0; j < num * stride; j += stride) {
			my_prefetcht0(s + i + 1 + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
			printf("WTF???\n");
		}
		memcmp_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 next block (num=%" PRIszt ", stride=%" PRIszt ") + memcmp: For a page, prefetcht0: %f ns, memcmp: %f ns\n", num, stride, (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcmp_time / (SIZE / PAGE_SIZE));
}

int main() {
	static char s[SIZE];
	static char buf[PAGE_SIZE];
	size_t i, j;
	clock_t start, end;

	memset(s, 0, SIZE);

	start = clock();
	for (i = 0; i < SIZE; i += PAGE_SIZE) {
		if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
			printf("WTF???\n");
		}
	}
	end = clock();
	printf("memcmp: %f ns for a page\n", (double)(end - start) / CLOCKS_PER_SEC * 1e9 / (SIZE / PAGE_SIZE));

	start = clock();
	for (i = 0; i < SIZE; i += 64) {
		my_prefetcht0(s + i);
	}
	end = clock();
	printf("prefetcht0: %f ns for a page\n", (double)(end - start) / CLOCKS_PER_SEC * 1e9 / (SIZE / PAGE_SIZE));

	start = clock();
	for (i = 0; i < SIZE; i += 64) {
		my_prefetcht1(s + i);
	}
	end = clock();
	printf("prefetcht1: %f ns for a page\n", (double)(end - start) / CLOCKS_PER_SEC * 1e9 / (SIZE / PAGE_SIZE));

	start = clock();
	for (i = 0; i < SIZE; i += 64) {
		my_prefetcht2(s + i);
	}
	end = clock();
	printf("prefetcht2: %f ns for a page\n", (double)(end - start) / CLOCKS_PER_SEC * 1e9 / (SIZE / PAGE_SIZE));

	start = clock();
	for (i = 0; i < SIZE; i += 64) {
		my_prefetchnta(s + i);
	}
	end = clock();
	printf("prefetchnta: %f ns for a page\n", (double)(end - start) / CLOCKS_PER_SEC * 1e9 / (SIZE / PAGE_SIZE));

	uint64_t start64 = 0;
	uint64_t prefetch_time = 0;
	uint64_t memcmp_time = 0;
	for (i = 0; i < SIZE; i += PAGE_SIZE) {
		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_prefetcht0(s + i + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
			printf("WTF???\n");
		}
		memcmp_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 + memcmp: For a page, prefetcht0: %f ns, memcmp: %f ns\n", (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcmp_time / (SIZE / PAGE_SIZE));

	prefetch_time = 0;
	memcmp_time = 0;
	for (i = 0; i < SIZE - PAGE_SIZE; i += PAGE_SIZE) {
		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_prefetcht0(s + i + 1 + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
			printf("WTF???\n");
		}
		memcmp_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 next + memcmp: For a page, prefetcht0: %f ns, memcmp: %f ns\n", (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcmp_time / (SIZE / PAGE_SIZE));

	prefetch_time = 0;
	memcmp_time = 0;
	for (i = 0; i < SIZE - PAGE_SIZE; i += PAGE_SIZE) {
		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_prefetcht0(s + i + 1 + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		for (j = 0; j < 8; ++j) {
			if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
				printf("WTF???\n");
			}
		}
		memcmp_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 next + memcmp 8 times: For a page, prefetcht0: %f ns, memcmp: %f ns\n", (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcmp_time / (SIZE / PAGE_SIZE));

	prefetch_time = 0;
	memcmp_time = 0;
	uint64_t clflush_time = 0;
	for (i = 0; i < SIZE; i += PAGE_SIZE) {
		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_prefetcht0(s + i + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
			printf("WTF???\n");
		}
		memcmp_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_clflush(s + i + j);
		}
		clflush_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 + memcmp + clflush: For a page, prefetcht0: %f ns, memcmp: %f ns, clflush: %f ns\n", (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcmp_time / (SIZE / PAGE_SIZE), (double)clflush_time / (SIZE / PAGE_SIZE));

	prefetch_time = 0;
	memcmp_time = 0;
	clflush_time = 0;
	for (i = 0; i < SIZE; i += PAGE_SIZE) {
		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_prefetcht0(s + i + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		if (memcmp(buf, s + i, PAGE_SIZE) != 0) {
			printf("WTF???\n");
		}
		memcmp_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_clflush_ro(s + i + j);
		}
		clflush_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 + memcmp + clflush_ro: For a page, prefetcht0: %f ns, memcmp: %f ns, clflush: %f ns\n", (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcmp_time / (SIZE / PAGE_SIZE), (double)clflush_time / (SIZE / PAGE_SIZE));

	// Seems like 8 prefetches are async. If more, then prefetches become sync
	for (i = 64; i <= 512; i *= 2) {
		for (j = 1; j <= PAGE_SIZE / i; j *= 2) {
			prefetcht0_next_block_stride(s, buf, j, i);
		}
	}

	return 0;
}
