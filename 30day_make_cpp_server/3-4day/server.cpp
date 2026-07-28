#include "socket.h"

int main(int argc, char *argv[])
{
    SocketTCP server("127.0.0.1", 8889);
    Epoll epoll(server, 1024);
    epoll.Loop();
    return 0;
}