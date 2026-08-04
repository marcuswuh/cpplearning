#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include "lib/threadpool.hpp"

namespace {

constexpr int kDefaultTaskCount = 10000;
constexpr int kRoundCount = 3;

// 模拟 CPU 任务：太轻会测到 mostly 锁竞争，太重会测到 mostly 纯计算
void WorkTask(int id)
{
    volatile long sum = 0;
    for (int i = 0; i < 5000000; ++i)
    {
        sum += (id + i) * (id - i + 1);
    }
    (void)sum;
}

double RunOnce(int thread_count, int task_count)
{
    ThreadPool pool(thread_count);
    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<size_t>(task_count));

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < task_count; ++i)
    {
        futures.push_back(pool.Add(WorkTask, i));
    }

    for (auto& future : futures)
    {
        future.get();
    }

    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double RunBenchmark(int thread_count, int task_count, int rounds)
{
    double total_ms = 0.0;
    for (int i = 0; i < rounds; ++i)
    {
        total_ms += RunOnce(thread_count, task_count);
    }
    return total_ms / rounds;
}

void PrintResult(const char* label, int thread_count, int task_count, double avg_ms)
{
    const double tasks_per_sec = task_count / (avg_ms / 1000.0);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << label
              << " | threads=" << thread_count
              << " | tasks=" << task_count
              << " | avg=" << avg_ms << " ms"
              << " | throughput=" << tasks_per_sec << " tasks/s"
              << '\n';
}

}  // namespace

int main(int argc, char* argv[])
{
    int task_count = kDefaultTaskCount;
    if (argc >= 2)
    {
        task_count = std::stoi(argv[1]);
    }

    std::cout << "ThreadPool benchmark\n";
    std::cout << "rounds=" << kRoundCount
              << ", each task runs a CPU loop (50k iter)\n\n";

    const double ms4 = RunBenchmark(4, task_count, kRoundCount);
    const double ms8 = RunBenchmark(8, task_count, kRoundCount);

    PrintResult("result", 4, task_count, ms4);
    PrintResult("result", 8, task_count, ms8);

    std::cout << '\n';
    if (ms8 > 0.0)
    {
        std::cout << "8 threads vs 4 threads speedup: "
                  << (ms4 / ms8) << "x\n";
    }

    return 0;
}
