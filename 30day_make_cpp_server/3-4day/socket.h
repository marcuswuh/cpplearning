#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

#define CHECK(func) if(func<0){perror(#func);return;}

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
        CHECK(bind(socketfd_, (struct sockaddr*)&addr, sizeof(addr)));
        CHECK(listen(socketfd_, 128));
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
private:
    int socketfd_;
    std::string ip_ = "0.0.0.0";
    int port_ = 8889;
    std::vector<struct sockaddr_in> clients_addr_;
    std::vector<int> clients_sockfd_;
};

class Epoll
{
public:
    Epoll(SocketTCP& server, int size_events = 1024):
    size_events_(size_events), server_(server)
    {
        epollfd_ = epoll_create1(0);
        if(epollfd_ < 0)perror("epoll_create");
        ev_.data.fd = server_.GetSocketFD(); // server端句柄，如果可读说明有新连接进来了。
        ev_.events = EPOLLIN | EPOLLET;// 可读事件 or 边沿事件
        // EOPLL_CTL_ADD 表示把socketfd注册到epoll中，并且当ev_发生了，通过epollfd告诉我一声
        if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, server_.GetSocketFD(), &ev_) < 0)perror("epoll_ctl");
    }
    ~Epoll()
    {
        if(epollfd_ != -1)
        {
            close(epollfd_);
        }
    }

    std::string ReadClientData(int fd)
    {
        char buf[1024];
        while (true)
        {
            ssize_t read_len = read(fd, buf, sizeof(buf));
            if (read_len > 0)
            {
                return std::string(buf, static_cast<size_t>(read_len));
            }
            if (read_len == 0)
            {
                return {};
            }
            return {};
        }
    }

    void CloseClient(int fd)
    {
        server_.RemoveClient(fd);
        epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }

    void OnEventConnect()
    {
        int client_fd = server_.Accept();
        if (client_fd < 0)
        {
            return;
        }
        ev_.data.fd = client_fd;
        ev_.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, client_fd, &ev_) < 0)perror("epoll_ctl");
    }

    void OnEventError(int fd)
    {
        CloseClient(fd);
    }

    void OnEventRead(int fd)
    {
        std::string data = ReadClientData(fd);
        std::cout << "(from "<< inet_ntoa(server_.GetClientAddrFromFD(fd).sin_addr) 
                  << ":" << ntohs(server_.GetClientAddrFromFD(fd).sin_port) << "): " << data << std::endl;
    }

    /* 循环等待事件发生，并处理事件
    */
    void Loop()
    {
        while (true)
        {
            int nfds = epoll_wait(epollfd_, events_, size_events_, -1);
            if (nfds < 0)
            {
                perror("epoll_wait");
                return;
            }
            for(int i = 0; i < nfds; i++)
            {
                int fd = events_[i].data.fd;
                // 事件里存 server句柄并且是可读，说明有新连接进来了。
                if(fd == server_.GetSocketFD() && (events_[i].events & EPOLLIN))
                {
                    OnEventConnect();
                }
                // 异常、关闭 事件
                else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    OnEventError(fd);
                }
                // 可读事件
                else if(events_[i].events & EPOLLIN)
                {
                    OnEventRead(fd);
                }
                // 未知事件
                else
                {
                    std::cout << "unknown event: " << events_[i].events << std::endl;
                }
            }
        }
    }
private:
    int epollfd_;
    int size_events_;
    struct epoll_event events_[1024];
    struct epoll_event ev_{};
    SocketTCP& server_;
};