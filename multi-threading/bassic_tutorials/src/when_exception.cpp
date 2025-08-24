// 当遇到异常时，我们仍然希望能够正确的使用和处理线程。
#include <iostream>
#include <thread>
#include <chrono>

int print_thread_cb()
{
    // 循环5次
    for (int i = 0; i < 5; ++i)
    {
        std::cout << "Thread is running: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}

/**
 * @brief 案例函数1，演示异常情况。如下，我们在线程开始和join之间还需要做一些操作，但是这些操作存在故障。这会导致我们希望的join无法正常执行。
**/
int example1()
{
    std::cout << "hi,I'm in example3." << std::endl;
    std::thread print_thread(print_thread_cb); //创建后线程会立即开始执行
    {//如果此时做了一些操作
        throw std::runtime_error("An error occurred in the main thread."); // <- 这些操作异常了，会导致下一步的join无法正常执行
    }
    print_thread.join();
    return 0;
}

/**
 * @brief 案例函数2，
**/

int example2()
{
    std::cout << "hi,I'm in example4." << std::endl;
    std::thread print_thread(print_thread_cb);
    try
    {
        // 我们执行了一些必要的操作，但是报错了。下面是模拟异常抛出。
        throw std::system_error(std::make_error_code(std::errc::operation_not_permitted), "An error occurred in the main thread.");
    }
    catch (const std::system_error& e)
    {
        print_thread.join(); //即使异常了，我们也能保证我们的想要的线程能够正常被执行。
        throw; // 重新抛出异常
    }
    print_thread.join(); // 确保线程被正确管理，即使在异常处理块中也要调用join
    return 0;
}

int main()
{
    // return example1();
    return example2();
}
