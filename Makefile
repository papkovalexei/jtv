CC=g++
FLAGS= -larchive -lz -lbz2
all: jtv

jtv: main.o parse_func.o
	$(CC) main.o parse_func.o -o jtv $(FLAGS)
main.o: main.cpp parse_func.hpp
	$(CC) -c main.cpp
parse_func.o: parse_func.hpp parse_func.cpp
	$(CC) -c parse_func.cpp
clean:
	rm -rf *.o *.gch