#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

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

static inline void my_clflush(const void *x) {
	asm volatile("clflush %0" : "+m" (*(volatile char *)(x)));
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
	uint64_t memcpy_time = 0;
	uint64_t clflush_time = 0;
	for (i = 0; i < SIZE; i += PAGE_SIZE) {
		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_prefetcht0(s + i + j);
		}
		prefetch_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		memcpy(buf, s + i, PAGE_SIZE);
		memcpy_time += timestamp_ns() - start64;

		start64 = timestamp_ns();
		for (j = 0; j < PAGE_SIZE; j += 64) {
			my_clflush(s + i + j);
		}
		clflush_time += timestamp_ns() - start64;
	}
	printf("prefetcht0 + clflush: For a page, prefetcht0: %f ns, memcpy: %f ns, clflush: %f ns\n", (double)prefetch_time / (SIZE / PAGE_SIZE), (double)memcpy_time / (SIZE / PAGE_SIZE), (double)clflush_time / (SIZE / PAGE_SIZE));

	return 0;
}
