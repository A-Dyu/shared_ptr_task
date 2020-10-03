#pragma once

#include "control_block.h"
#include <memory>

template <typename T>
struct weak_ptr;

template <typename T>
struct shared_ptr {

    shared_ptr() : cblock(nullptr), ptr(nullptr) {}

    explicit shared_ptr(T* ptr) : shared_ptr(ptr, std::default_delete<T>()) {}

    shared_ptr(std::nullptr_t) : shared_ptr() {}

    template<typename Y, typename D,
            typename = std::enable_if_t<
                    std::is_convertible<Y*, T*>::value> >
    shared_ptr(Y* ptr, D deleter) : ptr(ptr) {
        try {
            cblock = new regular_control_block<Y, D>(ptr, deleter);
        } catch(...) {
            deleter(ptr);
            throw;
        }
        cblock->add_strong();
    }

    template<typename Y,
    typename = std::enable_if_t<
            std::is_convertible<Y*, T*>::value> >
    explicit shared_ptr(Y* ptr) : shared_ptr(ptr, std::default_delete<Y>()) {}

    template<typename U>
    shared_ptr(shared_ptr<U> const& other, T* ptr) noexcept : shared_ptr(other.cblock, ptr) {}

    template<typename Y,
            typename = std::enable_if_t<
                    std::is_convertible<Y*, T*>::value> >
    shared_ptr(shared_ptr<Y> const& other) noexcept : shared_ptr(other, other.ptr) {}

    shared_ptr(shared_ptr<T> const& other) noexcept : shared_ptr(other.cblock, other.ptr) {}

    shared_ptr(shared_ptr<T>&& other) noexcept : shared_ptr() {
        swap(other);
    }

    shared_ptr& operator =(shared_ptr<T> const& other) {
        if (this != &other) {
            shared_ptr<T> safe(other);
            swap(safe);
        }
        return *this;
    }

    shared_ptr& operator =(shared_ptr<T>&& other) {
        if (this != &other) {
            reset();
            swap(other);
        }
        return *this;
    }

    ~shared_ptr() {
        if (cblock != nullptr) {
            cblock->del_strong();
        }
    }

    void reset() noexcept {
        shared_ptr<T> empty;
        swap(empty);
    }

    template<typename Y, typename D,
            typename = std::enable_if_t<
                    std::is_convertible<Y*, T*>::value> >
    void reset(Y* new_ptr, D deleter) {
        shared_ptr<T> new_shared(new_ptr, deleter);
        swap(new_shared);
    }

    template<typename Y,
            typename = std::enable_if_t<
                    std::is_convertible<Y*, T*>::value> >
    void reset(Y* ptr) {
        reset(ptr, std::default_delete<Y>());
    }


    T* get() const noexcept {
        return ptr;
    }

    T& operator*() const noexcept {
        return *ptr;
    }

    T* operator->() const noexcept {
        return ptr;
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    std::size_t use_count() const noexcept {
        return cblock == nullptr ? 0 : cblock->strong_count();
    }

    void swap(shared_ptr<T>& other) noexcept {
        using std::swap;
        swap(cblock, other.cblock);
        swap(ptr, other.ptr);
    }

    template<typename Y>
    friend struct weak_ptr;

    template<typename Y>
    friend struct shared_ptr;

private:
    control_block* cblock;
    T* ptr;

    shared_ptr(control_block* _cblock, T* _ptr) : cblock(_cblock), ptr(_ptr) {
        if (cblock != nullptr) {
            cblock->add_strong();
        }
    }

    template<typename Y, typename... Args>
    friend shared_ptr<Y> make_shared(Args&& ... args);
};


template<typename T, typename... Args>
shared_ptr<T> make_shared(Args&& ... args) {
    auto* cblock = new inplace_control_block<T>(std::forward<Args>(args)...);
    return shared_ptr<T>(cblock, reinterpret_cast<T*>(&cblock->stg));
}

template<typename T, typename Y>
bool operator ==(shared_ptr<T> const& a, shared_ptr<Y> const& b) {
    return a.get() == b.get();
}

template<typename T>
bool operator ==(shared_ptr<T> const& a, std::nullptr_t b) {
    return a.get() == nullptr;
}

template<typename T>
bool operator ==(std::nullptr_t a, shared_ptr<T> const& b) {
    return b.get() == nullptr;
}

template<typename T, typename Y>
bool operator !=(shared_ptr<T> const& a, shared_ptr<Y> const& b) {
    return !(a == b);
}

template<typename T>
bool operator !=(shared_ptr<T> const& a, std::nullptr_t b) {
    return a.get() != nullptr;
}

template<typename T>
bool operator !=(std::nullptr_t a, shared_ptr<T> const& b) {
    return b.get() != nullptr;
}

template<typename T>
struct weak_ptr {

    weak_ptr() noexcept : cblock(nullptr), ptr(nullptr) {}

    template<typename Y,
            typename = std::enable_if_t<
                    std::is_convertible<Y*, T*>::value> >
    weak_ptr(weak_ptr<Y> const& other) noexcept : weak_ptr(other.cblock, other.ptr) {}

    template<typename Y,
            typename = std::enable_if_t<
                    std::is_convertible<Y*, T*>::value> >
    weak_ptr(shared_ptr<Y> const& other) noexcept : weak_ptr(other.cblock, other.ptr) {}

    weak_ptr(weak_ptr<T> const& other) noexcept : weak_ptr(other.cblock, other.ptr) {}

    weak_ptr(weak_ptr<T>&& other) : weak_ptr() {
        swap(other);
    }

    ~weak_ptr() {
        if (cblock != nullptr) {
            cblock->del_weak();
        }
    }

    weak_ptr& operator =(weak_ptr const& other) {
        weak_ptr<T> safe(other);
        swap(safe);
        return *this;
    }

    weak_ptr& operator =(weak_ptr&& other) {
        if (this != &other) {
            weak_ptr<T> empty;
            weak_ptr(std::move(other)).swap(*this);
        }
        return *this;
    }

    std::size_t use_count() const noexcept {
        return cblock == nullptr ? 0 : cblock->strong_count();
    }

    shared_ptr<T> lock() const noexcept {
        if (cblock == nullptr || cblock->strong_count() == 0) {
            return shared_ptr<T>();
        }
        return shared_ptr<T>(cblock, ptr);
    }

    explicit operator bool() const noexcept {
        if (cblock->strong_count() == 0) {
            return false;
        }
        return ptr != nullptr;
    }

    void swap(weak_ptr<T>& other) noexcept {
        using std::swap;
        swap(cblock, other.cblock);
        swap(ptr, other.ptr);
    }

private:
    control_block* cblock;
    T* ptr;

    weak_ptr(control_block* cblock, T* ptr) : cblock(cblock), ptr(ptr) {
        if (cblock != nullptr) {
            cblock->add_weak();
        }
    }
};
