/*
 * Copyright 2019 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <runos/core/exception.hpp>
#include <runos/core/throw.hpp>

#include <memory>
#include <functional> // less
#include <type_traits>
#include <utility>

namespace runos {

struct bad_pointer_access : exception_root
                          , logic_error_tag
                          //,  bad_access??
{ };

template<class T>
class safe_ptr
{
public:
    using element_type = T;

    constexpr safe_ptr() noexcept = default;
    constexpr safe_ptr( std::nullptr_t ) noexcept { }

    safe_ptr(T* ptr) noexcept
        : ptr_(ptr)
    { }

    template<class Y>
    explicit safe_ptr(Y* ptr) noexcept
        : ptr_(ptr)
    { }

    template<class Y>
    explicit safe_ptr(safe_ptr<Y> r) noexcept
        : ptr_(r.ptr_)
    { }

    safe_ptr(safe_ptr const&) noexcept = default;
    safe_ptr(safe_ptr &&) noexcept = default;

    template<class Y>
    void swap(Y*& r) noexcept
    {
        std::swap(ptr_, r);
    }

    template<class Y>
    void swap(safe_ptr<Y>& r) noexcept
    {
        std::swap(ptr_, r.ptr_);
    }

    template<class Y>
    safe_ptr& operator=(Y* ptr) noexcept
    {
        ptr_ = ptr;
        return *this;
    }

    template<class Y>
    safe_ptr& operator=(safe_ptr<Y> ptr) noexcept
    {
        ptr_ = ptr.ptr_;
        return *this;
    }

    safe_ptr& operator=(safe_ptr const&) noexcept = default;
    safe_ptr& operator=(safe_ptr &&) noexcept = default;

    void reset() noexcept
    {
        ptr_ = nullptr;
    }

    template<class Y>
    void reset(Y* ptr) noexcept
    {
        ptr_ = ptr;
    }

    element_type* get() const noexcept
    {
        return ptr_;
    }

    element_type& operator*() const
    {
        return *not_null();
    }

    element_type* operator->() const
    {
        return not_null();
    }

    explicit operator bool() const noexcept
    {
        return ptr_;
    }

    T* not_null() const
    {
        auto ret = get();
        THROW_IF(!ret, bad_pointer_access());
        return ret;
    }
    
private:
    T* ptr_ = nullptr;
};

namespace safe {

template<class T>
class shared_ptr : public std::shared_ptr<T>
{
    using base = std::shared_ptr<T>;
public:
    using element_type = typename base::element_type;
    using base::base;
    using base::reset;
    using base::swap;
    using base::get;
    using base::use_count;
    using base::unique;
    using base::operator bool;
    using base::owner_before;

    shared_ptr(const base& p)
        : base(p)
    {
    }

    shared_ptr& operator=( std::nullptr_t ) noexcept
    {
        base::operator=(nullptr);
        return *this;
    }

    shared_ptr& operator=( const base& r ) noexcept
    {
        base::operator=(r);
        return *this;
    }

    template< class Y > 
    shared_ptr& operator=( const std::shared_ptr<Y>& r ) noexcept
    {
        base::operator=(r);
        return *this;
    }

    shared_ptr& operator=( base&& r ) noexcept
    {
        base::operator=(std::move(r));
        return *this;
    }

    template< class Y > 
    shared_ptr& operator=( std::shared_ptr<Y>&& r ) noexcept
    {
        base::operator=(std::move(r));
        return *this;
    }

    template< class Y, class Deleter >  // 4
    shared_ptr& operator=( std::unique_ptr<Y, Deleter>&& r )
    {
        base::operator=(std::move(r));
        return *this;
    }

    element_type& operator*() const
    {
        return not_null().operator*();
    }

    element_type* operator->() const
    {
        return not_null().operator->();
    }

    const base& not_null() const
    {
        THROW_IF(!get(), bad_pointer_access());
        return *this;
    }
};

template< class T, class... Args >
shared_ptr<T> make_shared( Args&&... args )
{
    return shared_ptr<T>(std::make_shared(std::forward<Args>(args)...));
}

template< class T, class Alloc, class... Args >
shared_ptr<T> allocate_shared( const Alloc& alloc, Args&&... args )
{
    return shared_ptr<T>(std::allocate_shared(alloc, std::forward<Args>(args)...));
}

template< class T, class U > 
shared_ptr<T> static_pointer_cast( const std::shared_ptr<U>& r )
{
    return shared_ptr<T>(std::static_pointer_cast<T>(r));
}

template< class T, class U > 
shared_ptr<T> dynamic_pointer_cast( const std::shared_ptr<U>& r )
{
    return shared_ptr<T>(std::dynamic_pointer_cast<T>(r));
}

template< class T, class U > 
shared_ptr<T> const_pointer_cast( const std::shared_ptr<U>& r )
{
    return shared_ptr<T>(std::const_pointer_cast<T>(r));
}

template< class T, class Deleter = std::default_delete<T> >
class unique_ptr : public std::unique_ptr<T, Deleter>
{
    using base = std::unique_ptr<T, Deleter>;
public:
    using pointer = typename base::pointer;
    using element_type = typename base::element_type;
    using deleter_type = typename base::deleter_type;
    using base::base;
    using base::release;
    using base::reset;
    using base::swap;
    using base::get;
    using base::get_deleter;
    using base::operator bool;

    unique_ptr& operator=( base&& r )
	{
        base::operator=(std::move(r));
        return *this;
    }

	template< class U, class E >
	unique_ptr& operator=( std::unique_ptr<U,E>&& r )
    {
        base::operator=(std::move(r));
        return *this;
    }

	unique_ptr& operator=( std::nullptr_t )
    {
        base::operator=(nullptr);
        return *this;
    }

    element_type& operator*() const
    {
        return not_null().operator*();
    }

    element_type* operator->() const
    {
        return not_null().operator->();
    }

    const base& not_null() const {
        THROW_IF(!get(), bad_pointer_access());
        return *this;
    }
};

template< class T, class Deleter >
class unique_ptr<T[], Deleter> : public std::unique_ptr<T[], Deleter>
{
    using base = std::unique_ptr<T[], Deleter>;
public:
    using pointer = typename base::pointer;
    using element_type = typename base::element_type;
    using deleter_type = typename base::deleter_type;
    using base::base;
    using base::release;
    using base::reset;
    using base::swap;
    using base::get;
    using base::get_deleter;
    using base::operator bool;

    unique_ptr& operator=( base&& r )
	{
        base::operator=(std::move(r));
        return *this;
    }

	unique_ptr& operator=( std::nullptr_t )
    {
        base::operator=(nullptr);
        return *this;
    }

    element_type& operator*() const
    {
        return not_null().operator*();
    }

    element_type* operator->() const
    {
        return not_null().operator->();
    }

    element_type& operator[](std::size_t i) const
    {
        return not_null()[i];
    }

    const base& not_null() const {
        THROW_IF(!get(), bad_pointer_access());
        return *this;
    }
};

template<class T>
class weak_ptr : public std::weak_ptr<T>
{
    using base = std::weak_ptr<T>;
public:
    using element_type = typename base::element_type;
    using base::base;
    using base::reset;
    using base::swap;
    using base::use_count;
    using base::expired;
    using base::owner_before;

    weak_ptr& operator=( const base& r ) noexcept
    {
        base::operator=(r);
        return *this;
    }

    template< class Y > 
    weak_ptr& operator=( const std::weak_ptr<Y>& r ) noexcept
    {
        base::operator=(r);
        return *this;
    }

    template< class Y > 
    weak_ptr& operator=( const std::shared_ptr<Y>& r ) noexcept
    {
        base::operator=(r);
        return *this;
    }

    weak_ptr& operator=( base&& r ) noexcept
    {
        base::operator=(std::move(r));
        return *this;
    }

    template< class Y > 
    weak_ptr& operator=( std::weak_ptr<Y>&& r ) noexcept
    {
        base::operator=(std::move(r));
        return *this;
    }
    
    // lock
    shared_ptr<T> lock() const noexcept
    {
        return shared_ptr<T>{ std::move(base::lock()) };
    }
};

} // safe

} // runos

namespace std {
    template<class T>
    struct hash<runos::safe::shared_ptr<T>>
        : hash<std::shared_ptr<T>>
    {
        using argument_type = runos::safe::shared_ptr<T>;
        using result_type = size_t;
    };

    template<class T, class Deleter>
    struct hash<runos::safe::unique_ptr<T, Deleter>>
        : hash<std::unique_ptr<T, Deleter>>
    {
        using argument_type = runos::safe::unique_ptr<T>;
        using result_type = size_t;
    };

    template<class T>
    struct hash<runos::safe_ptr<T>>
        : hash<T*>
    {
        using argument_type = runos::safe_ptr<T>;
        using result_type = size_t;
        result_type operator()(argument_type const& p) const noexcept
        {
            return hash<T*>::operator()(p.get());
        }
    };

    template<class T>
    void swap(runos::safe_ptr<T> lhs, runos::safe_ptr<T> rhs) noexcept
    {
        lhs.swap(rhs);
    }
}

namespace runos {

/*
 * Comparators
 */

// Compare two pointers
template < class T, class U > // 1
bool operator==( const safe_ptr<T>& lhs, const safe_ptr<U>& rhs ) noexcept
{ return lhs.get() == rhs.get(); }

template< class T, class U > // 2
bool operator!=( const safe_ptr<T>& lhs, const safe_ptr<U>& rhs ) noexcept
{ return !(lhs == rhs); }

template< class T, class U > // 3
bool operator<( const safe_ptr<T>& lhs, const safe_ptr<U>& rhs ) noexcept
{
    using V = std::common_type_t<T*, U*>;
    return std::less<V>()(lhs.get(), rhs.get());
}

template< class T, class U > // 4
bool operator>( const safe_ptr<T>& lhs, const safe_ptr<U>& rhs ) noexcept
{ return rhs < lhs; }

template< class T, class U >  // 5
bool operator<=( const safe_ptr<T>& lhs, const safe_ptr<U>& rhs ) noexcept
{ return !(rhs < lhs); }

template< class T, class U >  // 6
bool operator>=( const safe_ptr<T>& lhs, const safe_ptr<U>& rhs ) noexcept
{ return !(lhs < rhs); }

// Compare with nullptr
template< class T >  // 7
bool operator==( const safe_ptr<T>& lhs, std::nullptr_t ) noexcept
{ return !lhs; }

template< class T > // 8
bool operator==( std::nullptr_t , const safe_ptr<T>& rhs ) noexcept
{ return !rhs; }

template< class T > // 9
bool operator!=( const safe_ptr<T>& lhs, std::nullptr_t ) noexcept
{ return (bool)lhs; }

template< class T > // 10
bool operator!=( std::nullptr_t , const safe_ptr<T>& rhs ) noexcept
{ return (bool)rhs; }

template< class T > // 11
bool operator<( const safe_ptr<T>& lhs, std::nullptr_t ) noexcept
{ return std::less<T*>()(lhs.get(), nullptr); }

template< class T > // 12
bool operator<( std::nullptr_t, const safe_ptr<T>& rhs ) noexcept
{ return std::less<T*>()(nullptr, rhs.get()); }

template< class T > // 13
bool operator>( const safe_ptr<T>& lhs, std::nullptr_t ) noexcept
{ return nullptr < lhs; }

template< class T > // 14
bool operator>( std::nullptr_t, const safe_ptr<T>& rhs ) noexcept
{ return rhs < nullptr; }

template< class T > // 15
bool operator<=( const safe_ptr<T>& lhs, std::nullptr_t) noexcept
{ return !(nullptr < lhs); }

template< class T > // 16
bool operator<=( std::nullptr_t, const safe_ptr<T>& rhs ) noexcept
{ return !(rhs < nullptr); }

template< class T > // 17
bool operator>=( const safe_ptr<T>& lhs, std::nullptr_t ) noexcept
{ return !(lhs < nullptr); }

template< class T > // 18
bool operator>=( std::nullptr_t, const safe_ptr<T>& rhs ) noexcept
{ return !(nullptr < rhs); }

} // runos
