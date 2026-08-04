.PHONY: all clean

all:
	rm -f server.exe client.exe
	g++ -std=c++17 -pthread server.cpp -o server.exe
	g++ -std=c++17 client.cpp -o client.exe

clean:
	rm -f server.exe client.exe
