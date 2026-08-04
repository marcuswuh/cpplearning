#pragma once

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <vector>
#include <functional>
#include <mutex>
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
        wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (wakeup_fd_ < 0)
        {
            perror("eventfd");
        }
        epoll_.RegisterEpoll(wakeup_fd_, EPOLLIN, &wakeup_token_);
    }

    ~EventLoop()
    {
        if (wakeup_fd_ != -1)
        {
            close(wakeup_fd_);
        }
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
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_tasks_.push_back(std::move(cb));
        }
        Wakeup(); // 唤醒Poll里面的epoll_wait
    }

    void Loop()
    {
        while (true)
        {
            int nfds = epoll_.Poll(); // 这里会挂起/睡眠，除非有事件过来唤醒
            if (nfds < 0)
            {
                continue;
            }

            for (int i = 0; i < nfds; i++)
            {
                const epoll_event& ev = epoll_.Event(i);
                if (ev.data.ptr == &wakeup_token_) // 如果是一个唤醒事件，跳过
                {
                    HandleWakeup(); //把前面的消费掉，不然影响epoll挂起
                    continue;
                }
                else// 否则是一个IO事件，在这里处理IO事件
                {
                    auto* channel = static_cast<Channel*>(ev.data.ptr);
                    channel->SetRevents(ev.events);
                    channel->HandlerEvent(); // 处理IO事件
                }
            }

            std::vector<std::function<void()>> tasks;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                tasks.swap(pending_tasks_);
            }
            for (auto& task : tasks)
            {
                task(); // 执行注册任务，把进来的sfd注册到epoll里面
            }
        }
    }

private:
    void Wakeup()
    {
        uint64_t one = 1;
        ssize_t n = write(wakeup_fd_, &one, sizeof(one));
        if (n != sizeof(one))
        {
            perror("wakeup write");
        }
    }

    void HandleWakeup()
    {
        uint64_t value = 0;
        ssize_t n = read(wakeup_fd_, &value, sizeof(value));
        if (n != sizeof(value))
        {
            perror("wakeup read");
        }
    }

    Epoll epoll_;
    int size_events_;
    int wakeup_fd_ = -1;
    char wakeup_token_{};
    std::mutex mutex_;
    std::vector<std::function<void()>> pending_tasks_;
};

inline void Channel::HandlerEvent()
{
    if (callback_)
    {
        callback_();
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
