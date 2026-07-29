#pragma once
#include <functional>
#include <unistd.h>
#include "socket.hpp"
#include "channel.hpp"
#include "event_loop.hpp"

#define READ_BUFFER 1024
class Server
{
public:
    Server(EventLoop *_loop) : loop_(_loop)
    {
        SocketTCP *serv_sock = new SocketTCP("127.0.0.1", 8888);
        Channel *servChannel = new Channel(serv_sock->GetSocketFD(), loop_);

        std::function<void()> cb = std::bind(&Server::HandleNewConnection, this, serv_sock);
        servChannel->SetReadCallback(cb);
        servChannel->EnableReadingEvent();
    }

    ~Server()
    {
        
    }

    void HandleReadEvent(int sockfd)
    {
        char buf[READ_BUFFER]={};
        while(true)
        {
            ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
            if(bytes_read > 0)
            {
                printf("message from client fd %d: %s\n", sockfd, buf);
                write(sockfd, buf, sizeof(buf));
            } else if(bytes_read == -1 && errno == EINTR){  //客户端正常中断、继续读取
                printf("continue reading");
                continue;
            } else if(bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
                printf("finish reading once, errno: %d\n", errno);
                break;
            } else if(bytes_read == 0){  //EOF，客户端断开连接
                printf("EOF, client fd %d disconnected\n", sockfd);
                close(sockfd);   //关闭socket会自动将文件描述符从epoll树上移除
                break;
            }
        }
    }

    void HandleNewConnection(SocketTCP *serv_sock)
    {
        int client_fd = serv_sock->Accept();
        printf("new client fd %d! IP: %s Port: %d\n", client_fd, inet_ntoa(serv_sock->GetClientAddrFromFD(client_fd).sin_addr), ntohs(serv_sock->GetClientAddrFromFD(client_fd).sin_port));
        
        Channel *clntChannel = new Channel(client_fd, loop_);
        std::function<void()> cb = std::bind(&Server::HandleReadEvent, this, client_fd);

        clntChannel->SetReadCallback(cb);
        clntChannel->EnableReadingEvent();
    }
private:
    EventLoop *loop_;
};