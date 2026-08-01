#pragma once
#include <system_error>
class Connection
{
public:
    Connection(int fd,std::shared_ptr<EventLoop> loop):
    client_fd_(fd), loop_(loop)
    {
        channel_ = std::make_unique<Channel>(loop);
        channel_->SetReadCallback([this]() {
            echo(client_fd_);
        });
        channel_->EnableReadingEvent();

    }
    ~Connection()
    {

    }

    void Connection::echo(int sockfd)
    {
        char buf[1024];     //这个buf大小无所谓
        while(true)
        {    //由于使用非阻塞IO，读取客户端buffer，一次读取buf大小数据，直到全部读取完毕
            bzero(&buf, sizeof(buf));
            ssize_t bytes_read = read(sockfd, buf, sizeof(buf));
            if(bytes_read > 0)
            {
                readBuffer->append(buf, bytes_read);
            }
            else if(bytes_read == -1 && errno == EINTR)
            {  //客户端正常中断、继续读取
                printf("continue reading");
                continue;
            }
            else if(bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
            {//非阻塞IO，这个条件表示数据全部读取完毕
                printf("finish reading once\n");
                printf("message from client fd %d: %s\n", sockfd, readBuffer->c_str());
                errif(write(sockfd, readBuffer->c_str(), readBuffer->size()) == -1, "socket write error");
                readBuffer->clear();
                break;
            }
            else if(bytes_read == 0)
            {  //EOF，客户端断开连接
                printf("EOF, client fd %d disconnected\n", sockfd);
                // close(sockfd);   //关闭socket会自动将文件描述符从epoll树上移除
                deleteConnectionCallback(sock);
                break;
            }
        }
    }

    bool IsDisconnected() const
    {
        return disconnected_;
    }

private:
    std::unique_ptr<Channel> channel_;
    std::weak_ptr<EventLoop> loop_;
    int client_fd_;
    bool disconnected_;
};
