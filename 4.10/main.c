#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <sys/types.h>
#include <string.h>

int CONGRUENT_A = 17;
int CONGRUENT_C = 3;

int* result_array;

struct thread_info {
    int result_offset;
    int count;
    int mod;
    int pos;
};

void print_help() {
    printf("Multiply matrix by vector via 3 processors with common memory\n");
    printf("using posix threads\n");
    printf("\E[1;11mUsage:\E[0;11m main --seed seed --count count of sequence member\n");
}

int euclid_reverse(int a, int mod) {
    int x = a, y = mod;
    int x0 = 1, x1 = 0, y0 = 0, y1 = 1;
    int q, r;
    int old_x0, old_x1;
    while (y != 0) {
        q = x / y;
        r = x % y;
        x = y;
        y = r;
        old_x0 = x0;
        old_x1 = x1;
        x0 = y0;
        x1 = y1;
        y0 = old_x0 - q * y0;
        y1 = old_x1 - q * y1;
    }
    if (x0 < 0) {
        x0 += mod;
    }
    return x0;
}

int generate_simple(int lim_min, int lim_max) {
    int candidate = rand() % (lim_max - lim_min) + lim_min;
    int i;
    int sqrt_res = (int)sqrt((float)candidate);
    unsigned char candidate_ok = 1;
    while (1) {
        for (i = 2; i < sqrt_res; i++) {
            if ((candidate % i == 0) && (candidate != i)) {
                //wrong candidate
                candidate_ok = 0;
                break;
            }
        }
        if (!candidate_ok) {
            candidate = rand() % (lim_max - lim_min) + lim_min;
            sqrt_res = (int)sqrt((float)candidate);
            candidate_ok = 1;
        } else {
            return candidate;
        }
    }
}

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

static void *count_thread(void *arg) {
    struct thread_info *args = (struct thread_info *)arg;

    int i;
    int result;
    int offset = args -> result_offset;
    int count = args -> count;
    int mod = args -> mod;
    for (i = 0; i < count; i++) {
        // printf("%i: %i %i %i\n", args -> pos, args -> result_offset, args -> count, args -> mod);
        result = euclid_reverse(CONGRUENT_A * (offset + i) + CONGRUENT_C, mod);
        memcpy(result_array + offset + i, &(result), sizeof(int));
    }
}

int main(int argc, const char* argv[]) {

    srand(time(NULL));

    int i;
    int count = -1;
    int NUM_THREADS = -1;

    for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "--count") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &count);
        } else if ((strcmp(argv[i], "--threads") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &NUM_THREADS);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        }
    }

    if (count < 0) {
        printf("Invalid count\n");
        print_help();
        exit(-1);
    }

    if (NUM_THREADS <= 0) {
        printf("Invalid number of threads\n");
        print_help();
        exit(-1);
    }

    int seed = generate_simple(100, 1000);

    result_array = (int *)malloc(sizeof(int) * count);

    pthread_t* thread_ids;
    thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_THREADS);
    struct thread_info *thread_args;
    thread_args = (struct thread_info *)malloc(sizeof(struct thread_info) * NUM_THREADS);

    int data_size = count / NUM_THREADS;
    int extra_data = count % NUM_THREADS;

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].result_offset = i * data_size;
        thread_args[i].count = data_size;
        if (i == NUM_THREADS - 1) {
            thread_args[i].count += extra_data;
        }
        thread_args[i].pos = i;
        thread_args[i].mod = seed;
    }

    unsigned long long total_time;

    unsigned long long current_time = getTimestamp();

    for (i = 0; i < NUM_THREADS; i++) {
        if(pthread_create(&(thread_ids[i]), (pthread_attr_t *)NULL, count_thread, (void *)&(thread_args[i])))
            printf("\E[1;31mError:\E[0;11m unable to create thread: %s\n", strerror(errno));
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_ids[i], (void**)NULL);
    }

    total_time = getTimestamp()-current_time;

    // for (i = 0; i < count; i++) {
    //     printf("%i ", result_array[i]);
    // }
    printf("\nTotal time: %llu\n", total_time);

    free(result_array);
    free(thread_ids);
    free(thread_args);
}