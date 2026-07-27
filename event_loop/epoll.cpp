#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <thread>

int main()
{
    // 1) 创建用于跨线程唤醒的 eventfd
    int efd = eventfd(0, EFD_NONBLOCK);
    if (efd < 0)
    {
        perror("eventfd");
        return 1;
    }

    // 2) 创建 epoll 实例，并把 eventfd 加进去监听“可读”
    int epfd = epoll_create1(0);
    if (epfd < 0)
    {
        perror("epoll_create1");
        return 1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;   // 关心可读事件
    ev.data.fd = efd;      // 事件到来时，用这个认出是哪个 fd
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev) < 0)
    {
        perror("epoll_ctl");
        return 1;
    }

    // 3) 工作线程：1 秒后敲门铃
    std::thread worker([efd] {
        sleep(1);
        uint64_t one = 1;
        write(efd, &one, sizeof(one));
        printf("worker: notified event loop\n");
    });

    // 4) 事件循环：阻塞在 epoll_wait，直到有事件
    printf("loop: waiting in epoll_wait...\n");
    epoll_event events[1];
    int n = epoll_wait(epfd, events, 1, -1);  // -1 = 一直等
    if (n < 0)
    {
        perror("epoll_wait");
        return 1;
    }

    // 5) 被 eventfd 唤醒后，读一下清掉计数器
    if (events[0].data.fd == efd)
    {
        uint64_t val = 0;
        read(efd, &val, sizeof(val));  // 清空门铃
        printf("loop: woke up by eventfd, val=%llu\n",
               static_cast<unsigned long long>(val));
    }

    worker.join();
    close(efd);
    close(epfd);
    return 0;
}