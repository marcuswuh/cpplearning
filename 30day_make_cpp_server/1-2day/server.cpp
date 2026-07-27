#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define CHECK(func) if(func<0){perror(#func);return 1;}
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8889

int main(int argc,char *argv[])
{
    // domain: ip地址类型，AF_INET表示IPv4，AF_INET6表示IPv6
    // type: 套接字类型，SOCK_STREAM表示流式套接字(TCP)，SOCK_DGRAM表示数据报套接字(UDP)
    // protocol: 协议，0表示默认协议，会根据type和domain自动选择合适的协议，一般填0
    // 返回值：成功返回套接字描述符，失败返回-1
    int sockfd = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr  = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    // sockaddr_in需要转换成通用sockaddr结构体才能传给bind函数
    // 其实是类似union的思想，但是sock接口很老，历史原因采用了结构体类型转换的方案来实现。
    CHECK(bind(sockfd,(sockaddr*)&server_addr,sizeof(server_addr)));
    CHECK(listen(sockfd,128));
    
    struct sockaddr_in clnt_addr{};
    socklen_t clnt_addr_len = sizeof(clnt_addr);

    int clnt_sockfd = accept(sockfd,(sockaddr*)&clnt_addr,&clnt_addr_len);
    std::cout<<"clnt_sockfd: "<<clnt_sockfd
        << "ip:" << inet_ntoa(clnt_addr.sin_addr)
        << "port:" << ntohs(clnt_addr.sin_port)
        << std::endl;

    while(true)
    {
        char buf[1024] = {0};
        ssize_t read_len = read(clnt_sockfd,buf,sizeof(buf)-1);
        if(read_len > 0)
        {
            std::cout<<"client: "<<buf<<std::endl;
            write(clnt_sockfd, buf, sizeof(buf));
        }
        else if(read_len == 0)
        {
            std::cout<<"client disconnected"<<std::endl;
            break;
        }
        else if (read_len < 0)
        {
            std::cout<<"read error"<<std::endl;
            continue;
        }
    }
    close(clnt_sockfd);
    close(sockfd);
}