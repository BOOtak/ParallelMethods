#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <sys/types.h>
#include <string.h>

byte* result_array;

struct thread_info {
    byte result_offset;
    byte count;
    byte mod;
    byte pos;
};

void print_help() {
    printf("Calculates statistics criteria of pseudo-random sequence\n");
    printf("using posix threads\n");
    printf("\E[1;11mUsage:\E[0;11m main -j number of processors\n");
}

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

static void *count_thread(void *arg) {
    struct thread_info *args = (struct thread_info *)arg;

    byte i;
    byte result;
    byte offset = args -> result_offset;
    byte count = args -> count;
    byte mod = args -> mod;
    for (i = 0; i < count; i++) {
        // printf("%i: %i %i %i\n", args -> pos, args -> result_offset, args -> count, args -> mod);
        
        memcpy(result_array + offset + i, &(result), sizeof(byte));
    }
}

byte main(byte argc, const char* argv[]) {
    byte i;
    byte NUM_THREADS = -1, count = -1;

	for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "-j") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &NUM_THREADS);
        } else if ((strcmp(argv[i], "--count") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &count);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        }
    }

    if (NUM_THREADS <= 0) {
        printf("Invalid number of threads\n");
        print_help();
        exit(-1);
    }

    if (count < 0) {
        printf("Invalid count\n");
        print_help();
        exit(-1);
    }

    byte seed[7] = {2, 4, 23, 7, 5, 34, 3};

    result_array = (byte *)malloc(sizeof(byte) * count);

    pthread_t* thread_ids;
    thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_THREADS);
    struct thread_info *thread_args;
    thread_args = (struct thread_info *)malloc(sizeof(struct thread_info) * NUM_THREADS);

    
}
