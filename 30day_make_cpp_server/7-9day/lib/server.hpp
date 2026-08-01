/* 
Server类 仅具备 管理acceptor和connection的能力。
*/
#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include <unordered_map>
#include <iostream>
#include <utility>

#include "socket.hpp"
#include "channel.hpp"
#include "event_loop.hpp"
#include "acceptor.hpp"
#include "connection.hpp"

#define READ_BUFFER 1024

class Server
{
public:
    Server(std::shared_ptr<EventLoop> loop) : loop_(std::move(loop))
    {
        acceptor_ = std::make_unique<Acceptor>(loop_, 
            std::bind(&Server::HandleNewConnection, this, std::placeholders::_1));
    }
    
    void HandleNewConnection(int fd)
    {
        auto connection = std::make_unique<Connection>(fd, loop_);
        connections_[fd] = std::move(connection);
    }


private:
    std::shared_ptr<EventLoop> loop_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
};
