#include "timer.h"

#include <unistd.h>
#include <time.h>

tp timer_get(void) {
	struct timespec rt;
	clock_gettime(CLOCK_MONOTONIC, &rt);
	return rt.tv_sec * 1000000000 + rt.tv_nsec;
}

float timer_diff(tp ref) {
	tp cur = timer_get();
	tp diff = cur - ref;
	return ((float) diff / 1000000);
}
