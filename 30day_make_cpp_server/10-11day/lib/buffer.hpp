#pragma once

#include <string>

class Buffer
{
public:
    void Append(const char* data, size_t len)
    {
        for(size_t i = 0; i < len; i++)
        {
            if(data[i] != '\0')
            {
                 buffer_.push_back(data[i]);
            }
            else break;
        }
    }
    size_t Size() const
    {
        return buffer_.size();
    }

    const char* CStr() const
    {
        return buffer_.c_str();
    }

    void Clear()
    {
        buffer_.clear();
    }

    std::string GetLine() const
    {
        auto pos = buffer_.find('\n');
        if (pos == std::string::npos)
        {
            return buffer_;
        }
        return buffer_.substr(0, pos);
    }

private:
    std::string buffer_;
};
