#include "lib/server.hpp"
#include "lib/event_loop.hpp"
int main()
{
    EventLoop loop;
    Server server(&loop);
    loop.Loop();
    return 0;
}