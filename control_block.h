#pragma once

#include <cstddef>
#include <utility>

template<typename T>
struct shared_ptr;

struct control_block {
    control_block() noexcept : refs(0), weak_refs(0) {}

    void add_strong() noexcept {
        if (refs == 0) {
            add_weak();
        }
        refs++;
    }
    void add_weak() noexcept {
        weak_refs++;
    }

    void del_strong() noexcept {
        refs--;
        if (refs == 0) {
            delete_object();
            del_weak();
        }
    }
    void del_weak() noexcept {
        weak_refs--;
        if (weak_refs == 0) {
            delete this;
        }
    }

    std::size_t strong_count() const noexcept {
        return refs;
    }
    std::size_t weak_count() const noexcept {
        return weak_refs;
    }

protected:
    virtual ~control_block() = default;
    virtual void delete_object() noexcept = 0;

private:
    std::size_t refs;
    std::size_t weak_refs;
};

template<typename T, typename D>
struct regular_control_block final : control_block {
    explicit regular_control_block(T* _ptr, D _deleter) : ptr(_ptr), deleter(_deleter) {}

    void delete_object() noexcept override {
        deleter(ptr);
        ptr = nullptr;
    }
private:
    T* ptr;
    D deleter;
};

template <typename T>
struct inplace_control_block final : control_block {
    template<typename... Args>
    explicit inplace_control_block(Args&&... args) {
        new (&stg) T(std::forward<Args>(args)...);
    }

    void delete_object() noexcept override {
        reinterpret_cast<T*>(&stg)->~T();
    }

    template<typename Y, typename... Args>
    friend shared_ptr<Y> make_shared(Args&& ... args);

private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type stg;
};

