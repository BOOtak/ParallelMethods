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

struct first_run_thread_info {
    float* array_addr;
    int offset;
    int amount;
    int pos;
};

struct second_run_thread_info {
    float* start_addr;
    float* first_result_addr;
    float* second_result_addr;
    int pos;
    int amount;
};

int float_count = 1000;

float* first_ret_arr;
float* second_ret_arr;

void print_help() {
    printf("\E[1;11mUsage:\E[0;11m main --path FILE\n");
}

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
}

static void *first_run_thread(void *arg) {
    struct first_run_thread_info *args = (struct first_run_thread_info *)arg;

    int i, count = 0;
    float buf = 0.0f;

    int amount = args -> amount;
    int arr_offset = (args -> offset);

    int first, second, third, fourth, index;

    for (i = amount-1; i >= 0; i--) {
        first = arr_offset+i*2;
        second = arr_offset+i*2+1;
        third = float_count-1-second;
        fourth = float_count-1-first;

        index = i+(args -> offset)/2;

        // printf("%i, %i, %i: %f %f %f %f; index: %i\n", (args -> pos), amount, args -> offset,
        //                                     (args -> array_addr)[first], (args -> array_addr)[second],
        //                                     (args -> array_addr)[third], (args -> array_addr)[fourth],
        //                                     index);

        buf += (args -> array_addr)[first];
        buf += (args -> array_addr)[second];
        buf += (args -> array_addr)[fourth];

        if (second != third)
            buf += (args -> array_addr)[third];
        else
            printf("%i: middle\n", (args->pos));

        memcpy(&(first_ret_arr[index]), &buf, sizeof(float));
    }
}

static void *second_run_thread(void* arg) {
    struct second_run_thread_info *args = (struct second_run_thread_info*)arg;
    int i;
    float buf=0;
    int index = 0;

    for (i = (args -> amount)-1; i >= 0; i--) {
        index = (args -> amount) - (args -> pos)*(args -> amount)+i;
        // printf("%i: %f %f %f, index: %i;\n", args -> pos, *(args -> first_result_addr),
        //                             (args -> second_result_addr)[i], *(args -> start_addr),
        //                             index);
        buf = *(args -> start_addr) +
                *(args -> first_result_addr) +
                (args -> second_result_addr)[i];

        memcpy(&(second_ret_arr[index]), &buf, sizeof(float));
        // printf("%i: %f\n", args -> pos, buf);

    }

}

int main(int argc, const char* argv[]) {

    char* path = 0;
    int i;

    for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "--path") == 0) && (argc >= i+1)) {
            path = (char*)malloc(sizeof(char)*(strlen(argv[i+1])+1));
            memset(path, 0, sizeof(char)*(strlen(argv[i+1])+1));
            memcpy(path, argv[i+1], sizeof(char)*strlen(argv[i+1]));
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

    printf("\E[1;11mPath:\E[0;11m %s\n", path);

    FILE *in;
    if((in = fopen(path, "r")) == NULL) {
        printf("\E[1;31mError:\E[0;11m open file failed: %s\n", strerror(errno));
        exit(-1);
    }
    
    float* float_array = (float*)malloc(sizeof(float)*float_count);

    int scanned = 0;
    for(i=0; i < float_count; i++) {
        if(fscanf(in, "%f", &(float_array[i])) < 1) {
            if (errno == 0) {
                printf("\E[1;31mError:\E[0;11m Unexpected EOF, scanned: %i of 1000\n", scanned);
                float_count = scanned;
                break;
            }
            printf("\E[1;31mError:\E[0;11m unable to parse next int: %s\n", strerror(errno));
        }
        scanned++;
    }

    fclose(in);

    int sum_count = ((float_count-3)/4)+1;
    printf("sum_count: %i\n", sum_count);
    // we counting partial sums:
    // for example, when we have 11 elements, we are counting these sums:
    // [1, 11]; [3, 9]; [5, 7];
    // total amount of partial sums is 3

    first_ret_arr = (float*)malloc(sizeof(float)*sum_count);
    memset(first_ret_arr, 0, sizeof(float)*sum_count);

    int num_threads = 3;

    pthread_t* thread_ids;
    thread_ids = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);
    struct first_run_thread_info *first_thread_args;
    first_thread_args = (struct first_run_thread_info *)
                        malloc(sizeof(struct first_run_thread_info)*num_threads);

    for (i = 0; i < num_threads; i++) {
        first_thread_args[i].array_addr = float_array;
        first_thread_args[i].offset = i*(sum_count/3)*2;
        first_thread_args[i].pos = i;

        if (i == num_threads-1)
            first_thread_args[i].amount = sum_count/3+sum_count%3;
        else
            first_thread_args[i].amount = sum_count/3;
        // printf("%i\n", first_thread_args[i].amount);
    }

    second_ret_arr = (float*)malloc(sizeof(float)*sum_count/3*2);

    struct second_run_thread_info *second_run_thread_args;
    second_run_thread_args = (struct second_run_thread_info*)
                                malloc(sizeof(struct second_run_thread_info)*num_threads-1);

    unsigned long long cur_time = getTimestamp();

    for (i=0; i< num_threads; i++) {
        if(pthread_create(&(thread_ids[i]), (pthread_attr_t *)NULL, first_run_thread,
            (void *)&(first_thread_args[i]))) {
            printf("\E[1;31mError:\E[0;11m unable to create thread: %s\n", strerror(errno));
        }
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(thread_ids[i], (void**)NULL);
    }

    // for (i = 0; i < sum_count; i++) {
    //     printf("%i: %f\n", i, first_ret_arr[i]);
    // }

    // printf("first level passed\n");
    free(thread_ids);
    free(first_thread_args);

    thread_ids = (pthread_t*)malloc(sizeof(pthread_t)*2);

    
    float zero_addr = 0;

    second_run_thread_args[0].start_addr = &first_ret_arr[sum_count-(sum_count/3+sum_count%3)];
    second_run_thread_args[1].start_addr = &first_ret_arr[sum_count-(sum_count/3+sum_count%3)];
    second_run_thread_args[0].first_result_addr = &zero_addr;
    second_run_thread_args[1].first_result_addr = &first_ret_arr[sum_count/3];
    second_run_thread_args[0].second_result_addr = &first_ret_arr[sum_count/3];
    second_run_thread_args[1].second_result_addr = &first_ret_arr[0];
    second_run_thread_args[0].pos = 0;
    second_run_thread_args[1].pos = 1;
    second_run_thread_args[0].amount = sum_count/3;
    second_run_thread_args[1].amount = sum_count/3;

    // printf("index: %i\n", sum_count/3*2);

    for (i=0; i< num_threads-1; i++) {
        if(pthread_create(&(thread_ids[i]), (pthread_attr_t *)NULL, second_run_thread,
            (void *)&(second_run_thread_args[i]))) {
            printf("\E[1;31mError:\E[0;11m unable to create thread: %s\n", strerror(errno));
        }
    }

    for (i = 0; i < num_threads-1; i++) {
        pthread_join(thread_ids[i], (void**)NULL);
    }

    for (i = 0; i < sum_count/3*2; i++) {
        printf("%f, ", second_ret_arr[i]);
    }

    for (i = sum_count/3*2; i < sum_count; i++) {
        printf("%f, ", first_ret_arr[i]);
    }

    printf("%llu\n", getTimestamp() - cur_time);

    free(path);
    free(float_array);
    free(thread_ids);
    free(second_run_thread_args);
    free(first_ret_arr);
    free(second_ret_arr);

    return 0;
}