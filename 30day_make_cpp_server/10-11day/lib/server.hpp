#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include "socket.hpp"
#include "event_loop.hpp"
#include "acceptor.hpp"
#include "connection.hpp"

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

        int sub_loops_size = std::thread::hardware_concurrency();
        sub_loops_.reserve(sub_loops_size);
        for (int i = 0; i < sub_loops_size; ++i)
        {
            sub_loops_.push_back(std::make_shared<EventLoop>());
        }

        thread_pool_ = std::make_unique<ThreadPool>(sub_loops_size);
    }

    void NewConnectionCallback(SocketTCPClient client)
    {
        int fd = client.GetSockFD();
        auto close_callback = [this, fd]() {
            main_loop_->QueueInLoop([this, fd]() {
                connections_.erase(fd);
            });
        };

        int random = fd % sub_loops_.size(); // 随机选择一个子reactor

        auto connection = std::make_unique<Connection>(
            std::make_unique<SocketTCPClient>(std::move(client)),
            sub_loops_[random],
            std::move(close_callback));

        connections_[fd] = std::move(connection);
    }

private:
    std::shared_ptr<EventLoop> main_loop_;// 主reactor
    std::unique_ptr<Acceptor> acceptor_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    std::vector<std::shared_ptr<EventLoop>> sub_loops_;// 子reactor
    std::unique_ptr<ThreadPool> thread_pool_; // 线程池
};
