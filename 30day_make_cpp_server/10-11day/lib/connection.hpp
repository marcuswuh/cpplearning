#pragma once

#include <functional>
#include <memory>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include "socket.hpp"
#include "channel.hpp"
#include "event_loop.hpp"
#include "buffer.hpp"

class Connection
{
public:
    using CloseCallback = std::function<void()>;

    Connection(std::unique_ptr<SocketTCPClient> client_socket,
               std::shared_ptr<EventLoop> loop,
               CloseCallback close_callback)
        : loop_(std::move(loop)),
          client_socket_(std::move(client_socket)),
          close_callback_(std::move(close_callback)),
          disconnected_(false)
    {
        channel_ = std::make_unique<Channel>(loop);
        channel_->SetCallback([this]() {
            Echo(client_socket_->GetSockFD());
        });
        client_socket_->SetNonBlock();
        channel_->EnableReadingEvent(client_socket_->GetSockFD());
    }

    bool IsDisconnected() const
    {
        return disconnected_;
    }

private:
    void Echo(int sockfd)
    {
        char buf[1024];
        while (true)
        {
            std::memset(buf, 0, sizeof(buf));
            ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
            if (bytes_read > 0)
            {
                read_buffer_.Append(buf, static_cast<size_t>(bytes_read));
            }
            else if (bytes_read == -1 && errno == EINTR)
            {
                continue;
            }
            else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                if (read_buffer_.Size() > 0)
                {
                    printf("message from client fd %d: %s\n", sockfd, read_buffer_.CStr());
                    write(sockfd, read_buffer_.CStr(), read_buffer_.Size());
                    read_buffer_.Clear();
                }
                break;
            }
            else if (bytes_read == 0 ||
                     (bytes_read == -1 && (errno == ECONNRESET || errno == EPIPE)))
            {
                printf("client fd %d disconnected\n", sockfd);
                HandleDisconnect();
                break;
            }
            else if (bytes_read == -1)
            {
                perror("read");
                HandleDisconnect();
                break;
            }
        }
    }

    void HandleDisconnect()
    {
        if (disconnected_)
        {
            return;
        }
        disconnected_ = true;

        if (auto loop = loop_.lock())
        {
            loop->RemoveChannel(channel_->GetFd(), *channel_);
        }
        client_socket_->Close();

        if (close_callback_)
        {
            close_callback_();
        }
    }

    std::weak_ptr<EventLoop> loop_;
    std::unique_ptr<Channel> channel_;
    std::unique_ptr<SocketTCPClient> client_socket_;
    CloseCallback close_callback_;
    Buffer read_buffer_;
    bool disconnected_;
};
