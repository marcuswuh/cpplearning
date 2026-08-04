#pragma once

#include <functional>
#include <memory>
#include "socket.hpp"
#include "channel.hpp"
#include "event_loop.hpp"

class Acceptor
{
public:
    Acceptor(std::shared_ptr<EventLoop> loop, std::function<void(SocketTCPClient)> new_connection_cb)
        : loop_(std::move(loop)), new_connection_cb_(std::move(new_connection_cb))
    {
        server_socket_ = std::make_unique<SocketTCPServer>("127.0.0.1", 8888);
        server_socket_->SetNonBlock();
        server_channel_ = std::make_unique<Channel>(loop_);
        server_channel_->SetCallback([this]() {
            HandleNewConnection();
        });
        server_channel_->EnableReadingEvent(server_socket_->GetSockFD());
    }

    void HandleNewConnection()
    {
        while (true)
        {
            auto client = server_socket_->Accept();
            if (!client)
            {
                break;
            }
            new_connection_cb_(std::move(*client));
        }
    }

private:
    std::shared_ptr<EventLoop> loop_;
    std::unique_ptr<SocketTCPServer> server_socket_;
    std::unique_ptr<Channel> server_channel_;
    std::function<void(SocketTCPClient)> new_connection_cb_;
};
