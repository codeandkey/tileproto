#pragma once
#include <stdint.h>

typedef uint64_t tp;

tp timer_get(void);
float timer_diff(tp ref); /* in ms */
