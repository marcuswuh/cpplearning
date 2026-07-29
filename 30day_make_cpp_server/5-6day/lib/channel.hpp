#pragma once

/**
 * Channel 桥接 fd 与 EventLoop，维护事件状态与回调。
 * EnableReadingEvent 实现在 event_loop.hpp 末尾（避免循环 include）。
 */
#include <functional>
#include <cstdint>

class EventLoop;

class Channel
{
public:
    Channel(int sfd, EventLoop* loop) : sfd_(sfd), loop_(loop) {}

    void SetReadCallback(const std::function<void()>& read_callback)
    {
        read_callback_ = read_callback;
    }

    void HandlerReadEvent()
    {
        read_callback_();
    }

    void SetRevents(uint32_t revents)
    {
        revents_ = revents;
    }

    int GetSocketFD() const
    {
        return sfd_;
    }

    uint32_t GetRegisteredEvents() const
    {
        return regievents_;
    }

    int GetReturnedEvents() const
    {
        return revents_;
    }

    bool IsRegistered() const
    {
        return is_registered_;
    }

    void SetRegisteredTrue()
    {
        is_registered_ = true;
    }

    void SetRegisteredEvents(uint32_t events)
    {
        regievents_ = events;
    }

    void EnableReadingEvent();  // 实现在 event_loop.hpp 末尾

private:
    int sfd_;
    uint32_t regievents_ = 0;
    int revents_ = 0;
    std::function<void()> read_callback_;
    bool is_registered_ = false;
    EventLoop* loop_;
};
