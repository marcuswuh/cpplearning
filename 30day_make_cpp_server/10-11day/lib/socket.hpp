// 封装socket基本操作，不区分server还是client
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <optional>
#include <string>
#include "utils.hpp"

class SocketTCPClient;

class SocketTCPBase
{
public:
    virtual ~SocketTCPBase() = default;

    bool SetNonBlock()
    {
        int flags = fcntl(socketfd_, F_GETFL, 0);
        CHECK_RB(flags != -1, "fcntl F_GETFL");
        CHECK_RB(fcntl(socketfd_, F_SETFL, flags | O_NONBLOCK) != -1, "fcntl F_SETFL");
        return true;
    }

    int GetSockFD() const
    {
        return socketfd_;
    }

    std::string GetIP() const
    {
        return inet_ntoa(addr_.sin_addr);
    }

    uint16_t GetPort() const
    {
        return ntohs(addr_.sin_port);
    }

protected:
    int socketfd_ = -1;
    struct sockaddr_in addr_{};
};

class SocketTCPServer : public SocketTCPBase
{
public:
    SocketTCPServer(const std::string& ip = "0.0.0.0", int port = 8889)
    {
        socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
        addr_ = {};
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(static_cast<uint16_t>(port));
        addr_.sin_addr.s_addr = inet_addr(ip.c_str());
        CHECK_RV(bind(socketfd_, reinterpret_cast<struct sockaddr*>(&addr_), sizeof(addr_)) >= 0, "bind");
        CHECK_RV(listen(socketfd_, 128) >= 0, "listen");
    }

    ~SocketTCPServer() override
    {
        if (socketfd_ != -1)
        {
            close(socketfd_);
            socketfd_ = -1;
        }
    }

    std::optional<SocketTCPClient> Accept();

private:
};

class SocketTCPClient : public SocketTCPBase
{
public:
    SocketTCPClient() = default;

    SocketTCPClient(const std::string& target_ip, int target_port)
    {
        CHECK_RV(Connect(target_ip, target_port), "Connect");
        CHECK_RV(SetNonBlock(), "SetNonBlock");
    }

    SocketTCPClient(int sockfd, struct sockaddr_in addr)
    {
        socketfd_ = sockfd;
        addr_ = addr;
    }

    SocketTCPClient(SocketTCPClient&& other) noexcept
    {
        socketfd_ = other.socketfd_;
        addr_ = other.addr_;
        other.socketfd_ = -1;
        other.addr_ = {};
    }

    SocketTCPClient& operator=(SocketTCPClient&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            socketfd_ = other.socketfd_;
            addr_ = other.addr_;
            other.socketfd_ = -1;
            other.addr_ = {};
        }
        return *this;
    }

    SocketTCPClient(const SocketTCPClient&) = delete;
    SocketTCPClient& operator=(const SocketTCPClient&) = delete;

    ~SocketTCPClient() override
    {
        Close();
    }

    bool Connect(const std::string& ip, int port)
    {
        socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        CHECK_RB(connect(socketfd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) >= 0, "connect");
        addr_ = addr;
        return true;
    }

    void Close()
    {
        if (socketfd_ != -1)
        {
            close(socketfd_);
            socketfd_ = -1;
        }
    }
};

inline std::optional<SocketTCPClient> SocketTCPServer::Accept()
{
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(socketfd_, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    if (client_sockfd != -1)
    {
        return SocketTCPClient(client_sockfd, client_addr);
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK)
    {
        perror("accept");
    }
    return std::nullopt;
}
