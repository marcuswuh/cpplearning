/* 
Server类提供基本的服务器能力，持有EventLoop, Channel, SocketTCP这些资源。
*/
#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include "socket.hpp"
#include "channel.hpp"
#include "event_loop.hpp"

#define READ_BUFFER 1024

class Server
{
public:
    Server(std::shared_ptr<EventLoop> loop) : loop_(std::move(loop))
    {
        server_socket_ = std::make_unique<SocketTCP>("127.0.0.1", 8888);
        server_channel_ = std::make_unique<Channel>(server_socket_->GetSocketFD(), loop_);
        server_channel_->SetReadCallback(std::bind(&Server::HandleNewConnection, this));
        server_channel_->EnableReadingEvent();
    }

    void HandleReadEvent(int sockfd)
    {
        char buf[READ_BUFFER] = {};
        while (true)
        {
            ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
            if (bytes_read > 0)
            {
                printf("message from client fd %d: %s\n", sockfd, buf);
                write(sockfd, buf, static_cast<size_t>(bytes_read));
            }
            else if (bytes_read == -1 && errno == EINTR)
            {
                continue;
            }
            else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                break;
            }
            else if (bytes_read == 0)
            {
                printf("EOF, client fd %d disconnected\n", sockfd);
                RemoveClientChannel(sockfd);
                break;
            }
            else if (bytes_read < 0)
            {
                perror("read");
                RemoveClientChannel(sockfd);
                break;
            }
        }
    }

    void HandleNewConnection()
    {
        while (true)
        {
            int client_fd = server_socket_->Accept();
            if (client_fd < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                perror("accept");
                break;
            }

            printf("new client fd %d! IP: %s Port: %d\n",
                   client_fd,
                   inet_ntoa(server_socket_->GetClientAddrFromFD(client_fd).sin_addr),
                   ntohs(server_socket_->GetClientAddrFromFD(client_fd).sin_port));

            auto client_channel = std::make_unique<Channel>(client_fd, loop_);
            client_channel->SetReadCallback(
                std::bind(&Server::HandleReadEvent, this, client_fd));
            client_channel->EnableReadingEvent();
            client_channels_.push_back(std::move(client_channel));
        }
    }

private:
    void RemoveClientChannel(int sockfd)
    {
        for (auto it = client_channels_.begin(); it != client_channels_.end(); ++it)
        {
            if ((*it)->GetSocketFD() != sockfd)
            {
                continue;
            }
            loop_->RemoveChannel(**it);
            close(sockfd);
            server_socket_->RemoveClient(sockfd);
            client_channels_.erase(it);
            return;
        }
    }

    std::shared_ptr<EventLoop> loop_;
    std::unique_ptr<SocketTCP> server_socket_;
    std::unique_ptr<Channel> server_channel_;
    std::vector<std::unique_ptr<Channel>> client_channels_;
};
