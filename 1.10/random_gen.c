#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, const char* argv[]) {
	srand(time(NULL));
	FILE *in;
	in = fopen(argv[1], "a");
	long int i;
	for (i=0; i< 1000; i++) {
		fprintf(in, "%i.%i ", rand()%1000, rand()%1000);
	}
}