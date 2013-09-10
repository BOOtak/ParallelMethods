#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

#include <limits.h>

struct thread_info {
    int* array_addr;
    int offset;
    int pos;
};

int int_min = -1, int_max = -1;
int* ret_arr;

int print_help() {
    printf("\E[1;11mUsage:\E[0;11m main -c COUNT--path FILE [--min][--max][-j count]\n \
        --min for minimum delimiter (0 by default)\n \
        --max for maximum delimiter (INT_MAX by default)\n \
        -j COUNT for COUNT threads (1 by default)\n \
        --path FILE\n \
        -c COUNT to read COUNT integers from file\n");
}

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

static void *count_thread(void *arg) {
    struct thread_info *args = (struct thread_info *)arg;
    unsigned long i = 0;
    int count = 0;
    // printf("thread %i, count: %i\n", args -> pos, args -> offset);
    for (i = 0; i< (args -> offset); i++) {
        if ((int_min <= (args -> array_addr[i]))
            && ((args -> array_addr[i]) <= int_max)) {
            count++;
        }
    }
    memcpy(ret_arr+(args -> pos), &count, sizeof(int));
}

int main(int argc, const char* argv[]) {
    int i, num_threads = -1;
    char* path = 0;
    unsigned long int_count = -1;

    for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "-j") == 0) && (argc >= i+1)) {
            sscanf(argv[i+1], "%i", &num_threads);
        } else if ((strcmp(argv[i], "--min") == 0) && (argc >= i+1)) {
            sscanf(argv[i+1], "%i", &int_min);
        } else if ((strcmp(argv[i], "--max") == 0) && (argc >= i+1)) {
            sscanf(argv[i+1], "%i", &int_max);
        } else if ((strcmp(argv[i], "--path") == 0) && (argc >= i+1)) {
            path = (char*)malloc(sizeof(char)*(strlen(argv[i+1])+1));
            memset(path, 0, sizeof(char)*(strlen(argv[i+1])+1));
            memcpy(path, argv[i+1], sizeof(char)*strlen(argv[i+1]));
        } else if ((strcmp(argv[i], "-c") == 0) && (argc >= i+1)) {
            sscanf(argv[i+1], "%lu", &int_count);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        }
    }

    if (path == 0) {
        printf("\E[1;31mError:\E[0;11m Path not defined or incorrect\n");
        print_help();
        exit(-1);
    }
    if (int_min < 0) {
        printf("--min is incorrect or undefined, will use default (0)\n");
        int_min = 0;
    }
    if (int_max <= 0) {
        printf("--max is incorrect or undefined, will use default (INT_MAX)\n");
        int_max = INT_MAX;
    }
    if (int_min > int_max) {
        printf("\E[1;31mError:\E[0;11m Incorrect min and max limits\n");
        print_help();
        exit(-1);
    }
    if (num_threads <= 0) {
        num_threads = 1;
    }
    if (int_count == -1) {
        printf("\E[1;31mError:\E[0;11m incorrect count\n");
        print_help();
        exit(-1);
    }

    printf("path: %s\nthreads: %i min: %i max: %i\n", path, num_threads, int_min, int_max);

    FILE *in;
    if((in = fopen(path, "r")) == NULL) {
        printf("\E[1;31mError:\E[0;11m open file failed: %s\n", strerror(errno));
        exit(-1);
    }
    
    int* int_array = (int*)malloc(sizeof(int)*int_count);


    int scanned = 0;
    for(i=0; i < int_count; i++) {
        if(fscanf(in, "%i", &(int_array[i])) < 1) {
            printf("\E[1;31mError:\E[0;11m unable to parse next int: %s\n", strerror(errno));
            if (errno == 0) {
                int_count = scanned;
                break;
            }
        }
        scanned++;
    }

    pthread_t* thread_ids;
    thread_ids = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);
    struct thread_info *thread_args;
    thread_args = (struct thread_info *)malloc(sizeof(struct thread_info)*num_threads);

    int amount = int_count/num_threads;
    int ostatok = int_count%num_threads;
    ret_arr = (int*)malloc(sizeof(int)*num_threads);
    memset(ret_arr, 0, sizeof(int)*num_threads);
    int ret_sum = 0;

    unsigned long long total_time;

    for (i = 0; i < num_threads; i++) {
        thread_args[i].pos = i;
        thread_args[i].array_addr = int_array+i*amount;
        thread_args[i].offset = amount;
        if (i == num_threads-1)
            thread_args[i].offset+=ostatok;
    }

    unsigned long long current_time = getTimestamp();

    for (i = 0; i < num_threads; i++) {
        if(pthread_create(&(thread_ids[i]), (pthread_attr_t *)NULL, count_thread, (void *)&(thread_args[i])))
            printf("\E[1;31mError:\E[0;11m unable to create thread: %s\n", strerror(errno));
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(thread_ids[i], (void**)NULL);
        ret_sum += ret_arr[i];
    }

    total_time = getTimestamp()-current_time;

    printf("Total sum: %i, total: %llu nanoseconds\n", ret_sum, total_time);

    free(path);
    free(thread_ids);
    free(thread_args);
    free(int_array);
    free(ret_arr);
    fclose(in);
    return 0;
}