all: main.o
	gcc main.o -o main -pthread -lrt -lm -O3
main.o: main.c
clean:
	rm -rf *.o