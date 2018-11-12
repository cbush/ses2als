#pragma once

#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

inline void format(std::ostream &os, const char *fmt)
{
    os << fmt;
}

template <typename T, typename... Args>
inline void format(std::ostream &os, const char *fmt, T const &value, Args const &... args)
{
    while (*fmt)
    {
        if (*fmt == '%')
        {
            if (fmt[1] == '%')
            {
                ++fmt;
            }
            else
            {
                // TODO: %x, %02d, etc.
                os << value;
                fmt += 2;
                format(os, fmt, args...);
                return;
            }
        }
        os << *fmt;
        ++fmt;
    }
}

#define log_(category, ...)                               \
    std::printf(category ":%s %d: ", __FILE__, __LINE__); \
    format(std::cout, __VA_ARGS__);                       \
    std::printf("\n")

#define logv(...) log_("v", __VA_ARGS__)
#define logi(...) log_("i", __VA_ARGS__)
#define logw(...) log_("w", __VA_ARGS__)
#define loge(...) log_("e", __VA_ARGS__)

#define ASSERT(EXPRESSION, ...) \
    if (!(EXPRESSION))          \
    {                           \
        loge(__VA_ARGS__);      \
        assert(false);          \
    }

class exception : public std::exception
{
public:
    virtual ~exception() noexcept override = default;

    template <typename... T>
    exception(const char *fmt, T const &... args)
        : _ss(new std::stringstream)
    {
        format(*_ss, fmt, args...);
    }

public:
    const char *what() const noexcept override
    {
        return _ss->str().c_str();
    }

protected:
    std::shared_ptr<std::stringstream> _ss;
};

