#pragma once

/* 
class Epoll:
1. 提供基本的epoll操作，支持对sfd 事件的注册、更新、删除。
2. 支持阻塞拉取事件，返回事件的sfd和事件类型。
class EventLoop:
1. 用来管理epoll事件，并处理回调函数。
2. EventLoop看作是 Epoll的封装，对外提供接口，管理Channel。 Epoll完全隔离。
*/
#include <sys/epoll.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "channel.hpp"
#include "utils.hpp"


class Epoll
{
public:
    Epoll(int size_events = 1024):
    size_events_(size_events)
    {
        epollfd_ = epoll_create1(0);
        if(epollfd_ < 0)perror("epoll_create");
    }
    ~Epoll()
    {
        if(epollfd_ != -1)
        {
            close(epollfd_);
        }
    }

    /* 阻塞等待事件，返回就绪数量；事件存放在内部缓冲区，下次 Poll 前有效 */
    int Poll()
    {
        int nfds = epoll_wait(epollfd_, events_, size_events_, -1);
        if (nfds < 0)
        {
            perror("epoll_wait");
        }
        return nfds;
    }

    const epoll_event& Event(int index) const
    {
        return events_[index];
    }

    /* 注册事件到epoll */
    bool RegisterEpoll(int sfd, uint32_t events, void* data)
    {
        epoll_event ev;
        ev.events = events;
        ev.data.ptr = data;
        CHECK_RB(epoll_ctl(epollfd_, EPOLL_CTL_ADD, sfd, &ev) == 0, "epoll add error");
        return true;
    }

    /* 更新事件到epoll */
    bool UpdateEpoll(int sfd, uint32_t events, void* data)
    {
        epoll_event ev;
        ev.events = events;
        ev.data.ptr = data;
        CHECK_RB(epoll_ctl(epollfd_, EPOLL_CTL_MOD, sfd, &ev) == 0, "epoll modify error");
        return true;
    }

    /* 删除事件到epoll */
    bool RemoveEpoll(int sfd)
    {
        CHECK_RB(epoll_ctl(epollfd_, EPOLL_CTL_DEL, sfd, nullptr) == 0, "epoll remove error");
        return true;
    }

private:
    int epollfd_;
    int size_events_;
    struct epoll_event events_[1024];
    struct epoll_event ev_{};
};


class EventLoop
{
public:
    EventLoop(int size_events = 1024) : size_events_(size_events) {}

    void UpdateChannel(Channel& channel)
    {
        if (channel.IsRegistered())
        {
            epoll_.UpdateEpoll(channel.GetSocketFD(), channel.GetRegisteredEvents(), &channel);
        }
        else
        {
            epoll_.RegisterEpoll(channel.GetSocketFD(), channel.GetRegisteredEvents(), &channel);
            channel.SetRegisteredTrue();
        }
    }

    void RemoveChannel(Channel& channel)
    {
        if (channel.IsRegistered())
        {
            epoll_.RemoveEpoll(channel.GetSocketFD());
            channel.SetRegisteredFalse();
        }
        else
        {
            std::cout << "Channel is not registered" << std::endl;
        }
    }

    /* 返回就绪 Channel 指针（不拷贝 Channel 对象） */
    std::vector<Channel*> PollChannels()
    {
        int nfds = epoll_.Poll();
        if (nfds < 0)
        {
            return {};
        }

        std::vector<Channel*> channels;
        channels.reserve(static_cast<size_t>(nfds));
        for (int i = 0; i < nfds; i++)
        {
            const epoll_event& ev = epoll_.Event(i);
            auto* channel = static_cast<Channel*>(ev.data.ptr);
            channel->SetRevents(ev.events);
            channels.push_back(channel);
        }
        return channels;
    }

    /* 整个事件循环，阻塞函数 */
    void Loop()
    {
        while (true)
        {
            int nfds = epoll_.Poll();
            if (nfds < 0)
            {
                continue;
            }
            for (int i = 0; i < nfds; i++)
            {
                const epoll_event& ev = epoll_.Event(i);
                auto* channel = static_cast<Channel*>(ev.data.ptr);
                channel->SetRevents(ev.events);
                channel->HandlerReadEvent();
            }
        }
    }

private:
    Epoll epoll_;
    int size_events_;
};

inline void Channel::EnableReadingEvent()
{
    SetRegisteredEvents(EPOLLIN | EPOLLET);
    CHECK_RV(fcntl(GetSocketFD(), F_SETFL, O_NONBLOCK) >= 0, "fcntl");
    if(auto loop = loop_.lock())
    {
        loop->UpdateChannel(*this);
    }
}
