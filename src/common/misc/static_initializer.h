//static template

#ifndef SKYWELL_COMMON_STATIC_INI_H_
#define SKYWELL_COMMON_STATIC_INI_H_

#include <utility>

namespace skywell {

template <typename T, typename Tag=void>
class static_initializer
{
private:
    T* inst_;

public:
    template <typename... Args>
    explicit
    static_initializer(Args&&... args)
    {
        static T t(std::forward<Args>(args)...);
        inst_ = &t;
    }

    static_initializer()
    {
        static T t;
        inst_ = &t;
    }

public:
    T&
    get() noexcept
    {
        return *inst_;
    }

    const T&
    get() const noexcept
    {
        return const_cast<static_initializer&>(*this).get();
    }

    T&
    operator*() noexcept
    {
        return get();
    }

    const T&
    operator*() const noexcept
    {
        return get();
    }

    T*
    operator->() noexcept
    {
        return &get();
    }

    const T*
    operator->() const noexcept
    {
        return &get();
    }
};

} //skywell

#endif

