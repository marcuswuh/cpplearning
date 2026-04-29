// 前期回忆：
// 1. 线程对象创建后会马上启动。
// 2. 线程创建后必须使用join 或者 detach。
// 3. thread_obj.joinable() 表示线程是否在线，如果在线说明线程可以被std::thread管理。
// 4. 如果 thread_obj.joinable() 返回true，说明线程在线，必须使用join或者detach。
// 5. 如果线程已经被join或者detach，thread_obj.joinable() 返回false，说明线程已经不再被管理。
// 6. 当线程被detach后，线程会放到后台执行，但是当主线程推出后，后台线程也会强制退出，所以detach需要自己管理线程的生命周期。
// 7. 如果线程即没有被join也没有被detach，线程会一直占用资源，当主线程退出时，程序会调用std::terminate强制结束线程，并抛出异常。
// 8. 有时候，我们创建了线程，但是没有join或者detach，然后先执行了其他的重要步骤报错了，这个时候我们需要使用try-catch来捕获异常，确保线程能够被正确管理。

// IPC (inter-Process Communication)
// 本文件用于线程共享数据的示例。
// 线程间通讯的几种方式：互斥锁（mutex）、共享内存、条件变量、Feature and Promise，信号量（cpp20特性）、Barrier（cpp20特性）、原子操作、事件、消息队列、管道、读写锁。
// 互斥锁和读写锁是同一回事嘛？它们在使用时有区别，互斥锁是保证资源同一时间只在一个线程中被访问，而读写锁则允许多个线程同时读取资源，但在写入时会独占资源。


/** 知识补充： 
 * 1. 为什么需要加锁？
 * 2. 加锁需要注意什么？
 *      mutex锁住的是操作而不是数据（内存）本身，在使用mutex的时候需要注意这几个点：
 *      i. 对谁锁：锁住共享数据的访问，而不是锁住整个函数。会涉及到并发的数据都必须加锁。
 *      ii. 锁的粒度：尽量缩小锁的范围，减少锁的持有时间，提高并发性能。
 *      iii. 锁的类型：根据需求选择合适的锁类型，如独占锁、共享锁、读写锁等。
 *      iv. 锁的顺序：尽量按照一定的顺序加锁，避免不同线程之间的相互等待。
 *      v. 锁的持有时间：尽量缩短锁的持有时间，避免长时间占用锁。
 *      vi. 共享数据私有化：共享的数据必须私有化，并且避免任何函数返回（暴露）指向共享数据的引用。
 * 2. 多线程中发生死锁的几个条件
 *      i. 互斥条件：一个资源每次只能被一个线程持有。
 *      ii. 请求与保持条件： 一个线程持有了至少一个资源，并且又要请求新的资源，但是这个新的资源被别的线程持有了。
 *      iii. 不剥夺条件：资源只能被持有线程释放掉，不能被外部释放。
 *      iv. 循环等待条件：存在一个线程等待的资源链，形成环路。
 * 3. 如何避免死锁？
 *      i. 资源有序分配：对所有线程请求的资源进行排序，按照顺序申请资源，避免循环等待。
 *      ii. 申请资源时加锁：在申请资源时加锁，确保同一时间只有一个线程可以申请资源。
 *      iii. 超时机制：为资源申请设置超时时间，如果超时则放弃申请，避免长时间等待。
 * 4. 什么是无锁编程？
 *      无锁编程就是在多线程环境下，不使用锁机制来保护共享数据的一种编程方式。无锁编程可以提高程序的并发性能，减少线程间的竞争和上下文切换，但实现起来比较复杂，需要使用原子操作、CAS（Compare And Swap）等技术。
 * 5. 什么是transaction？
 *      事务是一组操作的集合，这些操作要么全部成功，要么全部失败。事务具有原子性、一致性、隔离性和持久性（ACID）特性。在多线程环境下，事务可以通过加锁、乐观锁等方式来实现。
 * */


// std::unique_lock ???
#include <iostream>
#include <thread>
#include <vector>
#include <numeric>
#include <mutex>
#include <list>

std::list<int> data;
std::mutex m; // 是可以定义多个互斥锁的，多个互斥锁可以同时锁住不同的资源。定义多个锁保护不同的资源可以提高并发性能。

void thread_function(int id)
{
    /* 这样也能加锁，但是每次都需要手动上锁和解锁。
    m.lock();
    // ....
    m.unlock();
    */
    int x;
    std::cout << "Thread " << id << " is running." << std::endl;
    {
        std::lock_guard<std::mutex> lg(m);// 使用lock_guard自动加锁和解锁
        // lock_guard会在作用域结束发生析构，自动解锁。
        // lock_guard 只会锁住被多个线程共享的数据访问。局部变量、函数参数等不受影响。
        data.push_back(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Thread " << id << " is done." << std::endl;
}

int main()
{
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back(thread_function, i);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    return 0;
}


// 以 stack 为例 考虑数据竞争。


// stack出现竞争的情况


// 重新组织代码来避免竞争情况


// 使用锁来避免竞争


// 死锁的发生和避免


// std::unique_lock


