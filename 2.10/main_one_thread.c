#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void print_help() {
    printf("\E[1;11mUsage:\E[0;11m main --path FILE\n");
}

int float_count = 1000;

static unsigned long long getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((unsigned long long)((t.tv_sec)*1000000000LL + t.tv_nsec));
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

    unsigned long long cur_time = getTimestamp();
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

    int amount = ((float_count-3)/4)+1;
    int arr_offset = 0;

    int first, second, third, fourth;
    float buf = 0;
    
    for (i = amount-1; i >= 0; i--) {
        first = arr_offset+i*2;
        second = arr_offset+i*2+1;
        third = float_count-1-second;
        fourth = float_count-1-first;


        // printf("%f %f %f %f;\n", float_array[first], float_array[second],
        //                         float_array[third], float_array[fourth]);

        buf += float_array[first];
        buf += float_array[second];
        buf += float_array[fourth];

        if (second != third)
            buf += float_array[third];
        else
            printf("middle\n");

        printf("%f, ", buf);

    }

    printf("%llu\n", getTimestamp()-cur_time);
}