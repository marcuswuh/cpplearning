.PHONY: all clean test

all:
	rm -f server.exe client.exe test.exe
	g++ server.cpp -o server.exe
	g++ client.cpp -o client.exe
	g++ -std=c++17 -O2 test.cpp -o test.exe

test: test.exe
	./test.exe

test.exe: test.cpp lib/threadpool.hpp
	g++ -std=c++17 -O2 test.cpp -o test.exe

clean:
	rm -f server.exe client.exe test.exe