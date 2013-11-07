#include <stdio.h>
#include <pthread.h>
#include <time.h>

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

int main(int argc, const char* argv[]) {
    return 0;
}