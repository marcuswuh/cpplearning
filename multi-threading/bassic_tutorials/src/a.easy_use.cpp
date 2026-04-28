/**
 * @brief 多线程基础教程，演示std::thread的基本用法。
 * @note 四个example函数，演示了线程的创建、等待、分离、异常处理等基本用法。
 * @author marcuswu
 * **/
#include <iostream>
#include <thread>
#include <chrono>

void print_thread_cb()
{
    // 循环5次
    for ( int i = 0; i < 5; ++i )
    {
        std::cout << "Thread is running: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return void();
}


/**
 * @brief 案例1，演示线程的创建、分离函数的使用。
 * 
 * @note 思考下面几个问题：
 * 
 * 1. 即不调用detach也不调用join，会发生什么？
 *  cpp标准库会检查线程是否joinable(),也就是线程是否还在线，如果是，则会调用std::terminate()函数，导致程序异常退出。
 * 
 * 2. detach是将函数放在后台运行，如果不执行detach和join函数，线程也是在后台运行的。所以detach有什么特殊作用？
 *      即使不调用detach 和 join, 线程仍然可以正常在后台工作，也就是joinable()== true，线程受到std::thread的管理。当std::thread析构了之后，会调用std::terminate()，导致程序异常退出。
 * 如果调用了detach, 线程就不再受std::thread的管理了，线程会在后台继续运行，直到任务完成或主线程退出。
 * 
 * 3. 当一个线程detach之后，线程还能恢复么？
 *    不能恢复，线程一旦detach，就不再受std::thread的管理了，无法再调用join或detach。
 * detach之后，线程的所有权和控制权被转移到cpp运行时库，它会保证线程的相关资源在线程推出后能够被正确地回收。
 * 参考unix的守护进程概念，这样被分离的线程可以被称作 **守护线程**。
 * 
 * 4. 什么时候使用join，什么时候使用detach？
 *    使用join可以等待线程完成，适用于需要等待线程执行完毕的情况，比如需要获取线程的返回值或需要确保线程执行完毕后再继续执行主线程的代码。
 *    使用detach可以让线程在后台运行，不需要等待线程完成，适用于不需要获取线程返回值或不需要确保线程执行完毕的情况。如资源监控、日志记录等。
 * 简而言之，detach适合两种场景：1.需要长时间在后台运行（通常会贯穿整个进程的生命） 2.第二种场景则是“即用即忘”的任务。
 *
 * 5. 如果detach了一个线程，主线程退出时，子线程会强制退出吗？
 *    是的，如果主线程退出时，子线程仍然在运行，子线程会被强制退出。这可能会导致资源泄漏或数据不一致等问题，所以在设计多线程程序时，需要注意资源的释放和数据的一致性。
 */
int example1()
{
    std::cout << "hi,I'm in example1." << std::endl;
    std::thread print_thread(print_thread_cb);

    // join 和 detach必须调用其中一个，否则会报错
    // print_thread.join(); //阻塞，等待任务完成
    print_thread.detach(); //分离线程，主线程退出时，子线程会强制退出，如果没有设计好资源的释放，可能会导致资源泄漏

    //循环n次
    for ( int i = 0; i < 6; ++i )
    {
        std::cout << "Main thread is running: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}


/**
 * @brief 案例2，演示joinable()函数的使用。
 * @note 思考下面几个问题：
 * 1. joinable()函数的真实含义是什么？
 *   joinable()函数用于检查线程是否被操作过，如果函数被join过或detach过，则返回false，表示线程不再受std::thread的管理。如果线程仍然在线且未被join或detach，则返回true。
 * 参考这张表：
 * 线程状态,                   joinable(),          说明
 * 
 * 刚创建未启动,                false,               线程未启动
 * 线程运行中未join或detach     true,                线程在线
 * join()完成                  false,               线程已运行结束
 * detach()后                  false,               线程独立，不再受std::thread的管理
 * 
 * 2. joinable()函数的返回值是否可以作为线程是否在线的判断依据？
 *  joinable()函数的返回值可以作为线程是否在线的判断依据，但需要注意的是，线程在线并不代表线程正在运行。线程可能处于等待状态或阻塞状态，所以不能仅仅依靠joinable()函数来判断线程是否在线。
 *   另外，joinable()函数只能在std::thread对象的生命周期内调用，如果std::thread对象已经析构，则无法调用joinable()函数。
 *
**/
int example2()
{
    std::cout << "hi,I'm in example2." << std::endl;
    std::thread print_thread(print_thread_cb); //当创建线程时，函数对象会被copy到线程的存储器中.
    print_thread.detach();
    std::cout << print_thread.joinable() << std::endl; // 输出0，表示线程已经被detach，不再受std::thread的管理
    return 0;
}


/** 案例3， 函数对象的线程创建
 * 
 */
class ThreadObj //这就是一个函数对象，通过重构operation方法来实现类似函数调用的效果.
{
public:
    void operator()()
    {
        for (int i = 0; i < 5; ++i)
        {
            std::cout << "Thread is running: " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

void example3()
{
    std::cout << "hi,I'm in example3." << std::endl;
    ThreadObj thread_obj;
    std::thread print_thread(thread_obj);// 这里是接受一个实例来创建线程。
    // std::thread print_thread(ThreadObj()); // 这是错误的写法，编译器会认为这是一个函数声明。
    // 同样的 int func(int()); 中的int()被看作是一个无参数但是返回类型是int的函数。
    // std::thread print_thread((ThreadObj())); // 这是正确的写法，这里是一个临时对象
    // std::thread print_thread{ThreadObj()}; // 这也是一种正确的写法.
    print_thread.detach();
    std::cout << print_thread.joinable() << std::endl; // 输出0，表示线程已经被detach，不再受std::thread的管理
}



int main()
{
    // return example1();
    return example2();
}
