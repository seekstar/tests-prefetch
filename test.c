#include <stdio.h>
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

#define SIZE (1 << 30)
#define PAGE_SIZE 4096

int main() {
	static char s[SIZE];
	size_t i;
	clock_t start, end;

	memset(s, 0, SIZE);

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

	return 0;
}
