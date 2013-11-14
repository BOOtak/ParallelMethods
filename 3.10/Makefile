all: main.o main_one_thread.o
	gcc main.o -o main -pthread -lrt -O3
	gcc main_one_thread.o -o main_one_thread -lrt -O3
main.o: main.c
main_one_thread.o: main_one_thread.c
clean:
	rm -rf *.o