#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::sim {

struct QuietTrace {
    static void drive(int, bool) noexcept {}
    static void sense(int, bool) noexcept {}
};

struct StdoutTrace {
    static void drive(const int pin, const bool level) noexcept
    {
        std::printf("arc-sim drive pin=%d level=%u\n", pin, level ? 1U : 0U);
    }

    static void sense(const int pin, const bool level) noexcept
    {
        std::printf("arc-sim sense pin=%d level=%u\n", pin, level ? 1U : 0U);
    }
};

enum class EventKind : std::uint8_t {
    drive,
    sense,
    mark,
};

struct Event {
    std::uint32_t tick{};
    EventKind kind{};
    int pin{};
    bool level{};
};

template <std::size_t Capacity>
struct TraceLog {
    static_assert(Capacity > 0U,
                  "[ARC ERROR] arc::sim::TraceLog needs storage. Action: set Capacity to the fixed event count.");

    static inline std::array<Event, Capacity> items{};
    static inline std::size_t used{};
    static inline std::uint32_t now{};
    static inline bool lost{};

    static void reset() noexcept
    {
        used = 0U;
        now = 0U;
        lost = false;
    }

    static void step(const std::uint32_t ticks = 1U) noexcept
    {
        now += ticks;
    }

    static void mark(const int id = 0, const bool level = true) noexcept
    {
        record(EventKind::mark, id, level);
    }

    static void drive(const int pin, const bool level) noexcept
    {
        record(EventKind::drive, pin, level);
    }

    static void sense(const int pin, const bool level) noexcept
    {
        record(EventKind::sense, pin, level);
    }

    [[nodiscard]] static std::span<const Event> events() noexcept
    {
        return std::span<const Event>{items.data(), used};
    }

    [[nodiscard]] static bool overflow() noexcept
    {
        return lost;
    }

private:
    static void record(
        const EventKind kind,
        const int pin,
        const bool level) noexcept
    {
        if (used == Capacity) {
            lost = true;
            return;
        }
        items[used++] = Event{
            .tick = now,
            .kind = kind,
            .pin = pin,
            .level = level,
        };
    }
};

template <typename T, std::size_t Capacity>
struct Fifo {
    static_assert(Capacity > 0U,
                  "[ARC ERROR] arc::sim::Fifo needs storage. Action: set Capacity to the fixed sample count.");
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::sim::Fifo payload must be trivially copyable. Action: use flat host samples.");

    std::array<T, Capacity> items{};
    std::size_t head{};
    std::size_t tail{};
    std::size_t used{};

    [[nodiscard]] static consteval std::size_t cap() noexcept
    {
        return Capacity;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return used;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return used == 0U;
    }

    [[nodiscard]] constexpr bool full() const noexcept
    {
        return used == Capacity;
    }

    constexpr void clear() noexcept
    {
        head = 0U;
        tail = 0U;
        used = 0U;
    }

    [[nodiscard]] constexpr bool try_push(const T& value) noexcept
    {
        if (full()) {
            return false;
        }
        items[head] = value;
        head = next(head);
        ++used;
        return true;
    }

    [[nodiscard]] constexpr std::size_t push(const std::span<const T> values) noexcept
    {
        if (!valid(values)) {
            return 0U;
        }

        std::size_t count{};
        for (const auto& value : values) {
            if (!try_push(value)) {
                break;
            }
            ++count;
        }
        return count;
    }

    [[nodiscard]] constexpr bool try_pop(T& out) noexcept
    {
        if (empty()) {
            return false;
        }
        out = items[tail];
        tail = next(tail);
        --used;
        return true;
    }

    [[nodiscard]] constexpr std::size_t pop(const std::span<T> out) noexcept
    {
        if (!valid(out)) {
            return 0U;
        }

        std::size_t count{};
        for (auto& value : out) {
            if (!try_pop(value)) {
                break;
            }
            ++count;
        }
        return count;
    }

private:
    [[nodiscard]] static constexpr std::size_t next(const std::size_t index) noexcept
    {
        return index + 1U == Capacity ? 0U : index + 1U;
    }

    template <typename U>
    [[nodiscard]] static constexpr bool valid(const std::span<U> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }
};

template <int Pin, typename Trace = QuietTrace>
struct Drive {
    static inline bool configured{};
    static inline bool level{};

    [[nodiscard]] static consteval int pin() noexcept
    {
        return Pin;
    }

    static void out() noexcept
    {
        configured = true;
    }

    template <bool Level>
    static void write() noexcept
    {
        set(Level);
    }

    static void hi() noexcept
    {
        set(true);
    }

    static void on() noexcept
    {
        hi();
    }

    static void lo() noexcept
    {
        set(false);
    }

    static void off() noexcept
    {
        lo();
    }

    static void toggle() noexcept
    {
        set(!level);
    }

    [[nodiscard]] static bool high() noexcept
    {
        return level;
    }

private:
    static void set(const bool next_level) noexcept
    {
        level = next_level;
        if constexpr (requires { Trace::drive(Pin, next_level); }) {
            Trace::drive(Pin, next_level);
        }
    }
};

template <int Pin, std::size_t Samples = 16U, typename Trace = QuietTrace>
struct Sense {
    static inline bool configured{};
    static inline bool level{};
    static inline Fifo<bool, Samples> samples{};

    [[nodiscard]] static consteval int pin() noexcept
    {
        return Pin;
    }

    static void in() noexcept
    {
        configured = true;
    }

    [[nodiscard]] static Status feed(const bool value) noexcept
    {
        if (!samples.try_push(value)) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }
        return ok();
    }

    [[nodiscard]] static std::size_t feed(const std::span<const bool> values) noexcept
    {
        return samples.push(values);
    }

    [[nodiscard]] static bool tick() noexcept
    {
        bool next{};
        if (!samples.try_pop(next)) {
            return false;
        }
        level = next;
        if constexpr (requires { Trace::sense(Pin, next); }) {
            Trace::sense(Pin, next);
        }
        return true;
    }

    [[nodiscard]] static bool high() noexcept
    {
        return level;
    }

    [[nodiscard]] static bool low() noexcept
    {
        return !level;
    }

    static void clear() noexcept
    {
        level = false;
        samples.clear();
    }
};

template <std::size_t Bytes, typename Trace = QuietTrace>
struct Spi {
    static_assert(Bytes > 0U,
                  "[ARC ERROR] arc::sim::Spi needs storage. Action: set Bytes to the fixed byte budget.");

    Fifo<std::uint8_t, Bytes> rx{};
    Fifo<std::uint8_t, Bytes> tx{};
    std::uint8_t idle{0xffU};
    std::size_t xfers{};

    constexpr void clear() noexcept
    {
        rx.clear();
        tx.clear();
        xfers = 0U;
    }

    [[nodiscard]] constexpr Status feed_rx(const std::span<const std::uint8_t> bytes) noexcept
    {
        if (!valid(bytes)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (bytes.size() > Bytes - rx.size()) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }
        static_cast<void>(rx.push(bytes));
        return ok();
    }

    [[nodiscard]] constexpr std::size_t drain_tx(const std::span<std::uint8_t> out) noexcept
    {
        return tx.pop(out);
    }

    [[nodiscard]] constexpr Status xfer(
        const std::span<const std::uint8_t> out,
        const std::span<std::uint8_t> in) noexcept
    {
        if (!valid(out) || !valid(in) || out.size() != in.size()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (out.size() > Bytes - tx.size()) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }

        for (std::size_t i = 0U; i < out.size(); ++i) {
            const auto mosi = out[i];
            auto miso = idle;
            static_cast<void>(tx.try_push(mosi));
            static_cast<void>(rx.try_pop(miso));
            in[i] = miso;
            ++xfers;
            if constexpr (requires { Trace::spi(mosi, miso); }) {
                Trace::spi(mosi, miso);
            }
        }
        return ok();
    }

private:
    template <typename T>
    [[nodiscard]] static constexpr bool valid(const std::span<T> data) noexcept
    {
        return data.empty() || data.data() != nullptr;
    }
};

template <typename Trace, typename... Inputs>
struct Harness {
    static void reset() noexcept
    {
        Trace::reset();
        (Inputs::clear(), ...);
    }

    [[nodiscard]] static std::size_t tick() noexcept
    {
        const auto moved = (std::size_t{0U} + ... + (Inputs::tick() ? 1U : 0U));
        Trace::step();
        return moved;
    }

    static void run(const std::size_t ticks) noexcept
    {
        for (std::size_t i = 0U; i < ticks; ++i) {
            static_cast<void>(tick());
        }
    }
};

}  // namespace arc::sim
