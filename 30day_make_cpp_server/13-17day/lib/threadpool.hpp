#pragma once

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <stdexcept>
#include <type_traits>

class ThreadPool
{
public:
    explicit ThreadPool(int size = 4) : stop(false)
    {
        for (int i = 0; i < size; ++i)
        {
            threads.emplace_back([this]() {
                while (true)
                {
                    std::function<void()> task;
                    {
                        // unique_lock 提供更灵活的锁机制，condition_variable必须与unique_lock一起使用
                        // 默认创建时上锁，unique_lock比较重，非必要不使用
                        std::unique_lock<std::mutex> lock(tasks_mtx);
                        // cv会先释放锁，然后尝试一次拿锁。
                        // 如果拿到锁，并且条件成立则继续执行，否则线程继续睡眠。
                        cv.wait(lock, [this]() {
                            return stop || !tasks.empty();
                        });
                        // 必须同时满足 stop 和 任务做完。如果任务没做完则把任务做完了再退出。
                        if (stop && tasks.empty())
                        {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(tasks_mtx);
            stop = true;
        }
        cv.notify_all();
        for (std::thread& th : threads)
        {
            if (th.joinable())
            {
                th.join();
            }
        }
    }

    template<class F, class... Args>
    auto Add(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(tasks_mtx);
            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return res;
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex tasks_mtx;
    std::condition_variable cv;
    bool stop;
};
