#pragma once

#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <type_traits>
#include <utility>

#include "esp_err.h"

namespace arc {

struct TaskArena {
    void* data{};
    std::size_t bytes{};
    std::size_t used{};

    [[nodiscard]] void* allocate(
        const std::size_t size,
        const std::size_t alignment) noexcept
    {
        if (data == nullptr || alignment == 0U || (alignment & (alignment - 1U)) != 0U || used > bytes) {
            return nullptr;
        }
        const auto base = reinterpret_cast<std::uintptr_t>(data);
        constexpr auto max_addr = ~std::uintptr_t{};
        if (used > max_addr - base) {
            return nullptr;
        }
        const auto current = base + used;
        const auto align_mask = static_cast<std::uintptr_t>(alignment - 1U);
        if (current > max_addr - align_mask) {
            return nullptr;
        }
        const auto aligned = (current + align_mask) & ~align_mask;
        if (size > max_addr - aligned) {
            return nullptr;
        }
        const auto end = aligned + size;
        if (end < base || end - base > bytes) {
            return nullptr;
        }
        used = end - base;
        return reinterpret_cast<void*>(aligned);
    }

    void reset() noexcept
    {
        used = 0U;
    }
};

template <typename T = void>
struct Task;

template <>
struct Task<void> {
    struct promise_type {
        static void* operator new(
            const std::size_t size,
            TaskArena& arena) noexcept
        {
            return arena.allocate(size, alignof(promise_type));
        }

        [[nodiscard]] static Task get_return_object_on_allocation_failure() noexcept
        {
            return {};
        }

        static void operator delete(void*, std::size_t) noexcept {}

        [[nodiscard]] Task get_return_object() noexcept
        {
            return Task{Handle::from_promise(*this)};
        }

        [[nodiscard]] std::suspend_always initial_suspend() const noexcept
        {
            return {};
        }
        [[nodiscard]] std::suspend_always final_suspend() const noexcept
        {
            return {};
        }
        void return_void() const noexcept {}
        [[noreturn]] void unhandled_exception() const noexcept
        {
            std::terminate();
        }
    };

    using Handle = std::coroutine_handle<promise_type>;

    constexpr Task() noexcept = default;
    explicit constexpr Task(const Handle handle) noexcept
        : handle_(handle)
    {
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    constexpr Task(Task&& other) noexcept
        : handle_(std::exchange(other.handle_, {}))
    {
    }

    Task& operator=(Task&& other) noexcept
    {
        if (this != &other) {
            destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    ~Task()
    {
        destroy();
    }

    [[nodiscard]] bool resume() noexcept
    {
        if (!handle_ || handle_.done()) {
            return false;
        }
        handle_.resume();
        return !handle_.done();
    }

    [[nodiscard]] bool done() const noexcept
    {
        return !handle_ || handle_.done();
    }

    void destroy() noexcept
    {
        if (handle_) {
            handle_.destroy();
            handle_ = {};
        }
    }

private:
    Handle handle_{};
};

struct SleepUntil {
    std::uint64_t deadline{};

    [[nodiscard]] bool await_ready() const noexcept
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};

[[nodiscard]] constexpr SleepUntil sleep_until(const std::uint64_t deadline) noexcept
{
    return {.deadline = deadline};
}

[[nodiscard]] constexpr SleepUntil sleep_for(
    const std::uint64_t now,
    const std::uint64_t delta) noexcept
{
    return {.deadline = now + delta};
}

}  // namespace arc
