#include "lib/server.hpp"

int main()
{
    auto loop = std::make_shared<EventLoop>();
    auto server = std::make_unique<Server>(loop);
    loop->Loop();
    return 0;
}
