/*
QA:
1. 这里one和val没什么用，为什么要存在？
原因一：write和read这两个是通用函数，函数范式要求传一个buffer进去，避免不了。
原因二：在通知、唤醒这个场景下，one和val确实作用不大，所以通常会封装一层把这两个临时变量隐藏掉。
原因三：one和val其实也并非无用，val可以知道距离上一次被通知了多少次，如果有特殊的需求可以依次做条件处理。
比如一次write到10，read就执行10次这种策略。
比如主线程手里有5个task要做，那么线程池中唤醒5个线程去执行。
*/
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <thread>

int main() {
    // 创建一个 eventfd，初值为 0
    int efd = eventfd(0, 0);
    if (efd < 0)
    {
        perror("eventfd");
        return 1;
    }

    // 子线程：往 eventfd 写一个值，相当于“发通知”
    std::thread t([efd] 
    {
        sleep(1);
        // 这里的one没啥用，就是写入一个值，相当于“发通知”
        uint64_t one = 1;
        write(efd, &one, sizeof(one));
        printf("writer: notified\n");
    });

    // 这里的val也没啥用，就是读取一个值，相当于“唤醒”。
    uint64_t val = 0;
    printf("reader: waiting...\n");
    read(efd, &val, sizeof(val));
    printf("reader: woke up, val=%llu\n",
           static_cast<unsigned long long>(val));

    t.join();
    close(efd);
    return 0;
}