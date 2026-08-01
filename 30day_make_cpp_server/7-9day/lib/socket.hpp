#pragma once
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h>
#include "utils.hpp"

class SocketTCP
{
public:
    SocketTCP(const std::string& ip = "0.0.0.0", int port = 8889):
    ip_(ip), port_(port)
    {
        socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr{
            .sin_family = AF_INET,
            .sin_port = htons(port_),
            .sin_addr = {.s_addr = inet_addr(ip_.c_str())} };
        CHECK_RV(bind(socketfd_, (struct sockaddr*)&addr, sizeof(addr)) >= 0, "bind");
        CHECK_RV(listen(socketfd_, 128) >= 0, "listen");
    }
    ~SocketTCP()
    {
        if(socketfd_ != -1)
        {
            close(socketfd_);
            for(auto sockfd : clients_sockfd_)
            {
                close(sockfd);
            }
            clients_addr_.clear();
            clients_sockfd_.clear();
        }
    }
    int Accept()
    {
        struct sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd = accept(socketfd_, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_sockfd != -1)
        {
            clients_addr_.push_back(client_addr);
            clients_sockfd_.push_back(client_sockfd);
            std::cout << "client connected: " << client_sockfd << std::endl;
            std::cout << "client ip: " << inet_ntoa(client_addr.sin_addr) << std::endl;
            std::cout << "client port: " << ntohs(client_addr.sin_port) << std::endl;
            return clients_sockfd_.back();
        }
        return -1;
    }

    int GetSocketFD() const
    {
        return socketfd_;
    }

    int GetLastClientFD() const
    {
        return clients_sockfd_.back();
    }

    struct sockaddr_in GetClientAddrFromFD(int fd) const
    {
        auto it = std::find(clients_sockfd_.begin(), clients_sockfd_.end(), fd);
        if (it != clients_sockfd_.end()) 
        {
            size_t i = it - clients_sockfd_.begin();
            return clients_addr_[i];
        }
        return {};
    }

    void RemoveClient(int fd)
    {
        auto it = std::find(clients_sockfd_.begin(), clients_sockfd_.end(), fd);
        if (it == clients_sockfd_.end())
        {
            return;
        }
        size_t i = static_cast<size_t>(it - clients_sockfd_.begin());
        struct sockaddr_in client_addr = clients_addr_[i];
        clients_sockfd_.erase(clients_sockfd_.begin() + static_cast<long>(i));
        clients_addr_.erase(clients_addr_.begin() + static_cast<long>(i));
        std::cout << "client disconnected: " << inet_ntoa(client_addr.sin_addr)
                  << ":" << ntohs(client_addr.sin_port) << std::endl;
    }

    void SetNonBlock()
    {
        int flags = fcntl(socketfd_, F_GETFL, 0);
        CHECK_RV(flags != -1, "fcntl F_GETFL");
        CHECK_RV(fcntl(socketfd_, F_SETFL, flags | O_NONBLOCK) != -1, "fcntl F_SETFL");
    }
private:
    int socketfd_;
    std::string ip_ = "0.0.0.0";
    int port_ = 8889;
    std::vector<struct sockaddr_in> clients_addr_;
    std::vector<int> clients_sockfd_;
};
