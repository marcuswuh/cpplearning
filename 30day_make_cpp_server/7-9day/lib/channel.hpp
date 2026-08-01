#pragma once

/**
 * Channel 事件回调和事件参数管理器。
 * EnableReadingEvent 实现在 event_loop.hpp 末尾（避免循环 include）。
 */
#include <functional>
#include <cstdint>
#include <memory>

class EventLoop;

class Channel
{
public:
    Channel(std::shared_ptr<EventLoop> loop) : loop_(loop) {}

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



    void SetRegisteredEvents(uint32_t events)
    {
        regievents_ = events;
    }

    void EnableReadingEvent();  // 实现在 event_loop.hpp 末尾
private:
    /* 有没有在epoll上注册过 */
    void SetRegisteredTrue()
    {
        is_registered_ = true;
    }

    void SetRegisteredFalse()
    {
        is_registered_ = false;
    }
private:
    uint32_t regievents_ = 0;
    int revents_ = 0;
    std::function<void()> read_callback_;
    bool is_registered_ = false;
    std::weak_ptr<EventLoop> loop_;
};
