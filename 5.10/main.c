#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <sys/types.h>
#include <string.h>

#define FIBONACCI_LAG 7

int* result_array;

struct thread_info {
    int result_offset;
    int count;
    int mod;
    int pos;
    int seed[FIBONACCI_LAG];
};

int test_seed[FIBONACCI_LAG] = {3, 42, 95, 32, 5, 85, 12};

int MOD = 563;
int AL = 37;
int CL = 84;
int AR = 29;
int CR = 83;

// int simple_numbers[] = {101,103,107,109,113,127,131,137,139,149,151,157,163,167,
//     173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,
//     277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,
//     397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,
//     509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,
//     641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,
//     761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,
//     887,907,911,919,929,937,941,947,953,967,971,977,983,991,997};

void print_help() {
    printf("\E[1;11mUsage:\E[0;11m main -j number of processors\n");
    printf("--seed seed\n");
    printf("--count amount of data to generate.\n");
}

void print_hello() {
    printf("Calculates statistics criteria of pseudo-random sequence\n");
    printf("using posix threads\n");
    print_help();
}

void print_error(char* msg) {
    printf("\E[1;31mError:\E[0;11m ");
    printf("%s", msg);
}

int congruent_next(a, x, c, mod) {
    return (a * x + c) % mod;
}

int** generate_lehmer_tree(int proc_num, int sequence_len, int seed, int al, int cl, int ar, int cr, int mod) {
    int** seed_array = (int**)malloc(proc_num * sizeof(int*));
    int i, j;
    for (i = 0; i < proc_num; i++) {
        seed_array[i] = (int*)malloc(sequence_len * sizeof(int));
    }
    for (i = 0; i < proc_num; i++) {
        if (i == 0) {
            seed_array[i][0] = seed;
        } else {
            seed_array[i][0] = congruent_next(al, seed_array[i-1][0], cl, mod);
        }
        for (j = 1; j < sequence_len; j++) {
            seed_array[i][j] = congruent_next(ar, seed_array[i][j-1], cr, mod);
        }
    }
    return seed_array;
}

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

//Lagged Multiplicative Fibonacci Generator
//U[i] = (U[i - 3]) * (U[i - 7]) mod P
static int lmfg(int sequence[FIBONACCI_LAG]) {
    // i - j = FIBONACCI_LAG - j
    return (sequence[FIBONACCI_LAG - 3] * sequence[FIBONACCI_LAG - 7]) % MOD;
}

static void *count_thread(void *arg) {
    struct thread_info *args = (struct thread_info *)arg;

    int i;
    int result;
    int offset = args -> result_offset;
    int count = args -> count;
    int mod = args -> mod;
    int pos = args -> pos;

    // first FIBONACCI_LAG elements of result array are elements from seed
    memcpy(result_array + offset, &(args -> seed), sizeof(int) * FIBONACCI_LAG);
    
    for (i = FIBONACCI_LAG; i < count; i++) {
        // printf("Thread %i: %i %i %i\n", pos, result_offset, count, mod);
        result = lmfg(result_array + offset + i - FIBONACCI_LAG);
        memcpy(result_array + offset + i, &(result), sizeof(int));
    }
}

int main(int argc, const char* argv[]) {
    srand(time(NULL));
    int i, j;
    int NUM_THREADS = -1, count = -1, seed = -1, LINES = 24, COLUMNS = 80;

	for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "-j") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &NUM_THREADS);
        } else if ((strcmp(argv[i], "--count") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &count);
        } else if ((strcmp(argv[i], "--seed") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &seed);
        } else if ((strcmp(argv[i], "--LINES") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &LINES);
        } else if ((strcmp(argv[i], "--COLUMNS") == 0) && (argc >= i + 1)) {
            sscanf(argv[i+1], "%i", &COLUMNS);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_hello();
            exit(0);
        }
    }

    if (NUM_THREADS <= 0) {
        print_error("Invalid number of threads\n");
        print_help();
        exit(-1);
    }

    if (count < 0) {
        print_error("Invalid count\n");
        print_help();
        exit(-1);
    }

    if (seed < 0) {
        print_error("Invalid seed\n");
        print_help();
        exit(-1);
    }

    result_array = (int *)malloc(sizeof(int) * count);

    pthread_t* thread_ids;
    thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_THREADS);
    struct thread_info *thread_args;
    thread_args = (struct thread_info *)malloc(sizeof(struct thread_info) * NUM_THREADS);

    int data_size = count / NUM_THREADS;
    int extra_data = count % NUM_THREADS;

    printf("Module: %i\n", MOD);

    int** lehmer_seed = generate_lehmer_tree(NUM_THREADS, FIBONACCI_LAG, seed, AL, CL, AR, CR, MOD);

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].result_offset = i * data_size;
        thread_args[i].count = data_size;
        if (i == NUM_THREADS - 1) {
            thread_args[i].count += extra_data;
        }
        thread_args[i].pos = i;
        memcpy(&(thread_args[i].seed), lehmer_seed[i], FIBONACCI_LAG * sizeof(int));
    }

    unsigned long long total_time;
    unsigned long long current_time = getTimestamp();

    for (i = 0; i < NUM_THREADS; i++) {
        if(pthread_create(&(thread_ids[i]), (pthread_attr_t *)NULL, count_thread, (void *)&(thread_args[i]))) {
            printf("\E[1;31mError:\E[0;11m unable to create thread: %s\n", strerror(errno));
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_ids[i], (void**)NULL);
    }

    total_time = getTimestamp() - current_time;

    int sof_column = count / COLUMNS;
    int* col_values = (int*)malloc(sizeof(int) * COLUMNS);
    int lim = 0;
    int max = 0;
    for (i = 0; i < count; i++) {
        if (max < result_array[i]) {
            max = result_array[i];
        }
    }
    int interval = max / COLUMNS;
    for (i = 0; i < COLUMNS; i++) {
        col_values[i] = 0;
        lim += interval;
        for (j = 0; j < count; j++) {
            if ((result_array[j] < lim) && (result_array[j] > (lim - interval))) {
                col_values[i]++;
            }
        }
    }
    // for (i = 0; i < COLUMNS; i++) {
    //     printf("%i ", col_values[i]);
    // }
    // printf("\n");
    int max_count = 0;

    for (i = 0; i < COLUMNS; i++) {
        if (max_count < col_values[i]) {
            max_count = col_values[i];
        }
    }

    int vertical_size = max_count / LINES;

    for (i = 0; i < LINES; i++) {
        for (j = 0; j < COLUMNS; j++) {
            if ((max_count - i * vertical_size) <= (col_values[j])) {
                printf("\E[1;42m \E[0;11m");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }

    free(result_array);
    free(thread_ids);
    free(thread_args);
    printf("\nTotal time: %llu\n", total_time);
}
