#pragma once

#include <sys/epoll.h>
#include <vector>
#include <functional>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "channel.hpp"
#include "utils.hpp"

class Epoll
{
public:
    explicit Epoll(int size_events = 1024) : size_events_(size_events)
    {
        epollfd_ = epoll_create1(0);
        if (epollfd_ < 0)
        {
            perror("epoll_create");
        }
    }

    ~Epoll()
    {
        if (epollfd_ != -1)
        {
            close(epollfd_);
        }
    }

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

    bool RegisterEpoll(int sfd, uint32_t events, void* data)
    {
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = data;
        CHECK_RB(epoll_ctl(epollfd_, EPOLL_CTL_ADD, sfd, &ev) == 0, "epoll add error");
        return true;
    }

    bool UpdateEpoll(int sfd, uint32_t events, void* data)
    {
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = data;
        CHECK_RB(epoll_ctl(epollfd_, EPOLL_CTL_MOD, sfd, &ev) == 0, "epoll modify error");
        return true;
    }

    bool RemoveEpoll(int sfd)
    {
        CHECK_RB(epoll_ctl(epollfd_, EPOLL_CTL_DEL, sfd, nullptr) == 0, "epoll remove error");
        return true;
    }

private:
    int epollfd_ = -1;
    int size_events_;
    epoll_event events_[1024]{};
};

class EventLoop
{
public:
    explicit EventLoop(int size_events = 1024) : size_events_(size_events)
    {
    }

    void UpdateChannel(int fd, Channel& channel)
    {
        if (channel.IsRegistered())
        {
            epoll_.UpdateEpoll(fd, channel.GetRegisteredEvents(), &channel);
        }
        else
        {
            epoll_.RegisterEpoll(fd, channel.GetRegisteredEvents(), &channel);
        }
    }

    void RemoveChannel(int fd, Channel& channel)
    {
        if (channel.IsRegistered())
        {
            epoll_.RemoveEpoll(fd);
            channel.SetRegisteredFalse();
        }
    }

    void QueueInLoop(std::function<void()> cb)
    {
        pending_tasks_.push_back(std::move(cb));
    }

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
                channel->HandlerEvent();
            }

            std::vector<std::function<void()>> tasks;
            tasks.swap(pending_tasks_);
            for (auto& task : tasks)
            {
                task();
            }
        }
    }

private:
    Epoll epoll_;
    int size_events_;
    std::vector<std::function<void()>> pending_tasks_;
};

inline void Channel::HandlerEvent()
{
    if (callback_)
    {
        if (auto loop = loop_.lock())
        {
            loop->QueueInLoop(callback_);
        }
    }
}

inline void Channel::EnableReadingEvent(int fd)
{
    fd_ = fd;
    SetRegisteredEvents(EPOLLIN | EPOLLET);
    CHECK_RV(fcntl(fd, F_SETFL, O_NONBLOCK) >= 0, "fcntl");
    if (auto loop = loop_.lock())
    {
        loop->UpdateChannel(fd, *this);
        SetRegisteredTrue();
    }
}
