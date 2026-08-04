#pragma once

#include <functional>
#include <cstdint>
#include <memory>

class EventLoop;

class Channel
{
public:
    explicit Channel(std::shared_ptr<EventLoop> loop) : loop_(loop) {}

    void SetCallback(std::function<void()> callback)
    {
        callback_ = std::move(callback);

    }

    void HandlerEvent();

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

    int GetFd() const
    {
        return fd_;
    }

    bool IsRegistered() const
    {
        return is_registered_;
    }

    void SetRegisteredEvents(uint32_t events)
    {
        regievents_ = events;
    }

    void SetRegisteredTrue()
    {
        is_registered_ = true;
    }

    void SetRegisteredFalse()
    {
        is_registered_ = false;
    }

    void EnableReadingEvent(int fd);

private:
    uint32_t regievents_ = 0;
    int revents_ = 0;
    int fd_ = -1;
    std::function<void()> callback_;
    bool is_registered_ = false;
    std::weak_ptr<EventLoop> loop_;
};
