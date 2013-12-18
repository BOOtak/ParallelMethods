#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <string.h>

struct thread_info {
    int** matr_addr;
    int* vector_addr;
    int result_offset;
    int count;
    int pos;
};

int MATR_SIZE_X = 1000;
int MATR_SIZE_Y = 1000;
int NUM_THREADS = 3;

int* result_vector;


static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

void print_help() {
    printf("Multiply matrix by vector via 3 processors with common memory\n");
    printf("using posix threads\n");
    printf("\E[1;11mUsage:\E[0;11m main --matr_path [file containing matrix] --vector_path [file containing vector]\n");
}

static void *count_thread(void *arg) {
    struct thread_info *args = (struct thread_info *)arg;

    int i, j;
    // int* result_addr_part = result_vector + (args -> result_offset);
    int result = 0;
    // printf("%i here! %i %li\n", args -> pos, args -> count, args -> result_offset);
    for (i = 0; i < args -> count; i++) {
        result = 0;
        for (j = 0; j < MATR_SIZE_X; j++) {
            // printf("%i: %i %i: %i %i\n", args -> pos, i, j, (args -> matr_addr[i][j]), (args -> vector_addr[j]));
            result += (args -> matr_addr[i][j]) * (args -> vector_addr[j]);
        }
        memcpy(result_vector + (args -> result_offset) + i, &(result), sizeof(int));    
    }
}

int main(int argc, const char* argv[]) {

    char* matr_path = 0;
    char* vector_path = 0;
    int i, j;
    for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "--matr_path") == 0) && (argc >= i + 1)) {
            matr_path = (char *)malloc(sizeof(char) * (strlen(argv[i + 1]) + 1));
            memset(matr_path, 0, sizeof(char) * (strlen(argv[i + 1]) + 1));
            memcpy(matr_path, argv[i+1], sizeof(char) * strlen(argv[i + 1]));
        }
        else if ((strcmp(argv[i], "--vector_path") == 0) && (argc >= i + 1)) {
            vector_path = (char *)malloc(sizeof(char) * (strlen(argv[i + 1]) + 1));
            memset(vector_path, 0, sizeof(char) * (strlen(argv[i + 1]) + 1));
            memcpy(vector_path, argv[i+1], sizeof(char) * strlen(argv[i + 1]));
        }
    }

    if (matr_path == 0) {
        printf("\E[1;31mError:\E[0;11m Matrix path not defined or incorrect\n");
        print_help();
        exit(-1);
    }

    if (vector_path == 0) {
        printf("\E[1;31mError:\E[0;11m Vector path not defined or incorrect\n");
        print_help();
        exit(-1);
    }

    printf("path: %s; threads: %i\n", matr_path, NUM_THREADS);

    FILE *matr_in;
    if((matr_in = fopen(matr_path, "r")) == NULL) {
        printf("\E[1;31mError:\E[0;11m open file failed: %s\n", strerror(errno));
        exit(-1);
    }

    int** int_array;
    int_array = (int **)malloc(sizeof(int *) * MATR_SIZE_Y);
    for (i = 0; i < MATR_SIZE_Y; i++) {
        int_array[i] = (int *)malloc(sizeof(int) * MATR_SIZE_Y);
        for (j = 0; j < MATR_SIZE_X; j++) {
            if(fscanf(matr_in, "%i", &(int_array[i][j])) < 1) {
                printf("\E[1;31mError:\E[0;11m unable to parse next int: %s\n", strerror(errno));
                exit(-1);
            }
        }
    }

    // printf("%i %i %i %i %i\n", int_array[0][0], int_array[0][1], int_array[0][4], int_array[1][0], int_array[2][0]);

    fclose(matr_in);

    FILE *vector_in;
    if((vector_in = fopen(vector_path, "r")) == NULL) {
        printf("\E[1;31mError:\E[0;11m open file failed: %s\n", strerror(errno));
        exit(-1);
    }

    int* int_vector;
    int_vector = (int *)malloc(sizeof(int) * MATR_SIZE_Y);
    for (i = 0; i < MATR_SIZE_Y; i++) {
        if(fscanf(vector_in, "%i", &(int_vector[i])) < 1) {
            printf("\E[1;31mError:\E[0;11m unable to parse next int: %s\n", strerror(errno));
            exit(-1);
        }
    }

    fclose(vector_in);

    result_vector = (int *)malloc(sizeof(int) * MATR_SIZE_Y);

    // printf("%i %i %i %i\n", int_vector[0], int_vector[1], int_vector[2], int_vector[19]);

    pthread_t* thread_ids;
    thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_THREADS);
    struct thread_info *thread_args;
    thread_args = (struct thread_info *)malloc(sizeof(struct thread_info) * NUM_THREADS);

    int data_size = MATR_SIZE_Y / NUM_THREADS; //size of one block of data
    int extra_data = MATR_SIZE_Y - (data_size * NUM_THREADS); //extra size of last block

    unsigned long long total_time;

    unsigned long long current_time = getTimestamp();

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].matr_addr = &(int_array[i * data_size]);
        thread_args[i].vector_addr = int_vector;
        thread_args[i].result_offset = i*data_size;
        thread_args[i].count = data_size;
        if (i == NUM_THREADS - 1) {
            thread_args[i].count += extra_data;
        }
        thread_args[i].pos = i;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if(pthread_create(&(thread_ids[i]), (pthread_attr_t *)NULL, count_thread, (void *)&(thread_args[i])))
            printf("\E[1;31mError:\E[0;11m unable to create thread: %s\n", strerror(errno));
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_ids[i], (void**)NULL);
    }

    total_time = getTimestamp()-current_time;

    for (i = 0; i < MATR_SIZE_Y; i++) {
        printf("%i ", result_vector[i]);
    }

    printf("\n%llu\n", total_time);

    free(matr_path);
    free(vector_path);
    for (i = 0; i < MATR_SIZE_Y; i++) {
        free(int_array[i]);
    }
    free(int_array);
    free(int_vector);
    free(result_vector);
    free(thread_args);
    free(thread_ids);

    return 0;
}
