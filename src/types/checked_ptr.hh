#pragma once

#include <stdexcept>
#include <type_traits>

struct nullptr_dereference : std::runtime_error {
    nullptr_dereference()
        : std::runtime_error("")
    { }
};

template<class T>
class checked_ptr {
    T* ptr {nullptr};
public:
    checked_ptr() noexcept = default;

    explicit checked_ptr(T* ptr) noexcept
        : ptr(ptr)
    { }

    checked_ptr& operator=(T* ptr_) noexcept
    {
        ptr = ptr_;
        return *this;
    }

    typename std::add_lvalue_reference<T>::type operator*() const
    {
        if (ptr == nullptr)
            throw nullptr_dereference();
        return *ptr;
    }

    const T* operator->() const
    {
        if (ptr == nullptr)
            throw nullptr_dereference();
        return ptr;
    }

    T* operator->()
    {
        if (ptr == nullptr)
            throw nullptr_dereference();
        return ptr;
    }

    T* get() const
    { return ptr; }

    explicit operator bool() const noexcept
    { return ptr != nullptr; }
};
