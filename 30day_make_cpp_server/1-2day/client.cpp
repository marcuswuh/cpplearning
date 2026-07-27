#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

int main(int argc,char ** argv)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8889);
    connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)); 
    while(true)
    {
        char buf[1024] = {0};     //定义缓冲区
        std::cin>>buf;
        write(sockfd, buf, strlen(buf));
        read(sockfd, buf, sizeof(buf));
        std::cout<<"message from server: "<<buf<<std::endl;
    }
}
