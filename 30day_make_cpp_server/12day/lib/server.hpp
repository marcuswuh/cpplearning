#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <thread>
#include "socket.hpp"
#include "event_loop.hpp"
#include "acceptor.hpp"
#include "connection.hpp"
#include "threadpool.hpp"

class Server
{
public:
    explicit Server(std::shared_ptr<EventLoop> main_loop) : main_loop_(std::move(main_loop))
    {
        acceptor_ = std::make_unique<Acceptor>(
            main_loop_,
            [this](SocketTCPClient client) {
                NewConnectionCallback(std::move(client));
            });

        const unsigned int sub_loops_size = std::thread::hardware_concurrency()>0 ? std::thread::hardware_concurrency() : 1;
        sub_loops_.reserve(sub_loops_size);
        for (unsigned int i = 0; i < sub_loops_size; ++i)
        {
            sub_loops_.push_back(std::make_shared<EventLoop>());
        }

        // 用线程池拉起 N 个 SubReactor，每个线程跑各自的 EventLoop::Loop()
        thread_pool_ = std::make_unique<ThreadPool>(static_cast<int>(sub_loops_size));
        for (auto& loop : sub_loops_)
        {
            thread_pool_->Add([loop]() {
                loop->Loop();
            });
        }
    }

    void NewConnectionCallback(SocketTCPClient client)
    {
        int fd = client.GetSockFD();
        auto sub_loop = sub_loops_[next_loop_];
        next_loop_ = (next_loop_ + 1) % sub_loops_.size();

        auto close_callback = [this, fd]() {
            main_loop_->QueueInLoop([this, fd]() {
                connections_.erase(fd);
            });
        };

        // 创建一个连接对象，挂到子reactor上面
        auto connection = std::make_unique<Connection>(
            std::make_unique<SocketTCPClient>(std::move(client)),
            sub_loop,
            std::move(close_callback));

        Connection* raw = connection.get();
        connections_[fd] = std::move(connection);

        // 在所属 SubReactor 线程中注册 epoll，避免跨线程操作 epoll
        sub_loop->QueueInLoop([raw]() {
            raw->Start(); // 在所属 SubReactor 线程中注册 epoll，避免跨线程操作 epoll
        });
    }

private:
    std::shared_ptr<EventLoop> main_loop_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    std::vector<std::shared_ptr<EventLoop>> sub_loops_;
    std::unique_ptr<ThreadPool> thread_pool_;
    size_t next_loop_ = 0;
};
