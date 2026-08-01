// 只负责持有server socket和channel，并接受新连接。
#include "socket.hpp"
#include "channel.hpp" 
#include "event_loop.hpp"
class Acceptor
{
public:
    Acceptor(std::shared_ptr<EventLoop> loop,std::function<void(int)> new_connection_cb)
    {
        server_socket_ = std::make_unique<SocketTCP>("127.0.0.1", 8888);
        server_socket_->SetNonBlock();
        server_channel_ = std::make_unique<Channel>(loop);
        new_connection_cb_ = new_connection_cb;
    }

    void HandleNewConnection()
    {
        int client_fd = server_socket_->Accept();
        if(client_fd != -1)
        {
            new_connection_cb_(client_fd);
        }
    }
private:
    std::unique_ptr<SocketTCP> server_socket_;
    std::unique_ptr<Channel> server_channel_;
    // 新连接进来时的回调函数。
    std::function<void(int)> new_connection_cb_;
};