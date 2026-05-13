#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_err.h"

#include "arc/result.hpp"
#include "arc/rtos.hpp"

namespace arc {

template <typename T>
concept AnyIoBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

namespace detail {

template <typename Fn>
[[nodiscard]] inline Status any_status(Fn&& fn) noexcept
{
    using Ret = std::remove_cvref_t<decltype(fn())>;
    if constexpr (std::is_same_v<Ret, esp_err_t>) {
        return status(fn());
    } else if constexpr (std::is_same_v<Ret, Status>) {
        return fn();
    } else {
        fn();
        return ok();
    }
}

}  // namespace detail

struct AnyOut {
    using StatusFn = Status (*)(void*) noexcept;
    using HighFn = Result<bool> (*)(void*) noexcept;

    void* ctx{};
    StatusFn out_fn{};
    StatusFn hi_fn{};
    StatusFn lo_fn{};
    StatusFn toggle_fn{};
    HighFn high_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return out_fn != nullptr &&
            hi_fn != nullptr &&
            lo_fn != nullptr &&
            toggle_fn != nullptr &&
            high_fn != nullptr;
    }

    template <typename Pin>
    [[nodiscard]] static constexpr AnyOut bind() noexcept
    {
        return AnyOut{
            .ctx = nullptr,
            .out_fn = &out_static<Pin>,
            .hi_fn = &hi_static<Pin>,
            .lo_fn = &lo_static<Pin>,
            .toggle_fn = &toggle_static<Pin>,
            .high_fn = &high_static<Pin>,
        };
    }

    template <typename Pin>
    [[nodiscard]] static constexpr AnyOut bind(Pin& pin) noexcept
    {
        return AnyOut{
            .ctx = &pin,
            .out_fn = &out_object<Pin>,
            .hi_fn = &hi_object<Pin>,
            .lo_fn = &lo_object<Pin>,
            .toggle_fn = &toggle_object<Pin>,
            .high_fn = &high_object<Pin>,
        };
    }

    [[nodiscard]] Status out() const noexcept
    {
        if (out_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return out_fn(ctx);
    }

    [[nodiscard]] Status hi() const noexcept
    {
        if (hi_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return hi_fn(ctx);
    }

    [[nodiscard]] Status lo() const noexcept
    {
        if (lo_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return lo_fn(ctx);
    }

    [[nodiscard]] Status write(const bool level) const noexcept
    {
        return level ? hi() : lo();
    }

    [[nodiscard]] Status toggle() const noexcept
    {
        if (toggle_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return toggle_fn(ctx);
    }

    [[nodiscard]] Result<bool> high() const noexcept
    {
        if (high_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return high_fn(ctx);
    }

    [[nodiscard]] Result<bool> low() const noexcept
    {
        const auto value = high();
        if (!value) {
            return fail(value.error());
        }
        return !*value;
    }

private:
    template <typename Pin>
    [[nodiscard]] static Status out_static(void*) noexcept
    {
        return detail::any_status([]() { return Pin::out(); });
    }

    template <typename Pin>
    [[nodiscard]] static Status hi_static(void*) noexcept
    {
        return detail::any_status([]() { return Pin::hi(); });
    }

    template <typename Pin>
    [[nodiscard]] static Status lo_static(void*) noexcept
    {
        return detail::any_status([]() { return Pin::lo(); });
    }

    template <typename Pin>
    [[nodiscard]] static Status toggle_static(void*) noexcept
    {
        return detail::any_status([]() { return Pin::toggle(); });
    }

    template <typename Pin>
    [[nodiscard]] static Result<bool> high_static(void*) noexcept
    {
        return Pin::high();
    }

    template <typename Pin>
    [[nodiscard]] static Status out_object(void* const ctx) noexcept
    {
        return detail::any_status([&]() { return static_cast<Pin*>(ctx)->out(); });
    }

    template <typename Pin>
    [[nodiscard]] static Status hi_object(void* const ctx) noexcept
    {
        return detail::any_status([&]() { return static_cast<Pin*>(ctx)->hi(); });
    }

    template <typename Pin>
    [[nodiscard]] static Status lo_object(void* const ctx) noexcept
    {
        return detail::any_status([&]() { return static_cast<Pin*>(ctx)->lo(); });
    }

    template <typename Pin>
    [[nodiscard]] static Status toggle_object(void* const ctx) noexcept
    {
        return detail::any_status([&]() { return static_cast<Pin*>(ctx)->toggle(); });
    }

    template <typename Pin>
    [[nodiscard]] static Result<bool> high_object(void* const ctx) noexcept
    {
        return static_cast<Pin*>(ctx)->high();
    }
};

struct AnyIn {
    using StatusFn = Status (*)(void*) noexcept;
    using HighFn = Result<bool> (*)(void*) noexcept;

    void* ctx{};
    StatusFn in_fn{};
    HighFn high_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return in_fn != nullptr && high_fn != nullptr;
    }

    template <typename Pin>
    [[nodiscard]] static constexpr AnyIn bind() noexcept
    {
        return AnyIn{
            .ctx = nullptr,
            .in_fn = &in_static<Pin>,
            .high_fn = &high_static<Pin>,
        };
    }

    template <typename Pin>
    [[nodiscard]] static constexpr AnyIn bind(Pin& pin) noexcept
    {
        return AnyIn{
            .ctx = &pin,
            .in_fn = &in_object<Pin>,
            .high_fn = &high_object<Pin>,
        };
    }

    [[nodiscard]] Status in() const noexcept
    {
        if (in_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return in_fn(ctx);
    }

    [[nodiscard]] Result<bool> high() const noexcept
    {
        if (high_fn == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return high_fn(ctx);
    }

    [[nodiscard]] Result<bool> low() const noexcept
    {
        const auto value = high();
        if (!value) {
            return fail(value.error());
        }
        return !*value;
    }

private:
    template <typename Pin>
    [[nodiscard]] static Status in_static(void*) noexcept
    {
        return detail::any_status([]() { return Pin::in(); });
    }

    template <typename Pin>
    [[nodiscard]] static Result<bool> high_static(void*) noexcept
    {
        return Pin::high();
    }

    template <typename Pin>
    [[nodiscard]] static Status in_object(void* const ctx) noexcept
    {
        return detail::any_status([&]() { return static_cast<Pin*>(ctx)->in(); });
    }

    template <typename Pin>
    [[nodiscard]] static Result<bool> high_object(void* const ctx) noexcept
    {
        return static_cast<Pin*>(ctx)->high();
    }
};

struct AnyI2c {
    using SendFn = esp_err_t (*)(void*, const void*, std::size_t, int) noexcept;
    using RecvFn = esp_err_t (*)(void*, void*, std::size_t, int) noexcept;
    using XferFn = esp_err_t (*)(void*, const void*, std::size_t, void*, std::size_t, int) noexcept;

    void* ctx{};
    SendFn send_fn{};
    RecvFn recv_fn{};
    XferFn xfer_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return send_fn != nullptr && recv_fn != nullptr && xfer_fn != nullptr;
    }

    template <typename Dev>
    [[nodiscard]] static constexpr AnyI2c bind() noexcept
    {
        return AnyI2c{
            .ctx = nullptr,
            .send_fn = &send_static<Dev>,
            .recv_fn = &recv_static<Dev>,
            .xfer_fn = &xfer_static<Dev>,
        };
    }

    template <typename Dev>
    [[nodiscard]] static constexpr AnyI2c bind(Dev& dev) noexcept
    {
        return AnyI2c{
            .ctx = &dev,
            .send_fn = &send_object<Dev>,
            .recv_fn = &recv_object<Dev>,
            .xfer_fn = &xfer_object<Dev>,
        };
    }

    [[nodiscard]] esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms = 1000) const noexcept
    {
        if (send_fn == nullptr || (data == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        return send_fn(ctx, data, bytes, timeout_ms);
    }

    [[nodiscard]] esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const int timeout_ms = 1000) const noexcept
    {
        if (recv_fn == nullptr || (data == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        return recv_fn(ctx, data, bytes, timeout_ms);
    }

    [[nodiscard]] esp_err_t xfer(
        const void* const tx,
        const std::size_t tx_bytes,
        void* const rx,
        const std::size_t rx_bytes,
        const int timeout_ms = 1000) const noexcept
    {
        if (xfer_fn == nullptr ||
            (tx == nullptr && tx_bytes != 0U) ||
            (rx == nullptr && rx_bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        return xfer_fn(ctx, tx, tx_bytes, rx, rx_bytes, timeout_ms);
    }

    template <typename T, std::size_t Extent>
        requires AnyIoBytes<T>
    [[nodiscard]] esp_err_t send(
        const std::span<T, Extent> data,
        const int timeout_ms = 1000) const noexcept
    {
        return send(data.data(), data.size_bytes(), timeout_ms);
    }

    template <typename T, std::size_t Extent>
        requires(AnyIoBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] esp_err_t recv(
        const std::span<T, Extent> data,
        const int timeout_ms = 1000) const noexcept
    {
        return recv(data.data(), data.size_bytes(), timeout_ms);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(AnyIoBytes<Tx> && AnyIoBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] esp_err_t xfer(
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const int timeout_ms = 1000) const noexcept
    {
        return xfer(tx.data(), tx.size_bytes(), rx.data(), rx.size_bytes(), timeout_ms);
    }

private:
    template <typename Dev>
    [[nodiscard]] static esp_err_t send_static(
        void*,
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        return Dev::send(data, bytes, timeout_ms);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t recv_static(
        void*,
        void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        return Dev::recv(data, bytes, timeout_ms);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t xfer_static(
        void*,
        const void* const tx,
        const std::size_t tx_bytes,
        void* const rx,
        const std::size_t rx_bytes,
        const int timeout_ms) noexcept
    {
        return Dev::xfer(tx, tx_bytes, rx, rx_bytes, timeout_ms);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t send_object(
        void* const ctx,
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        return static_cast<Dev*>(ctx)->send(data, bytes, timeout_ms);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t recv_object(
        void* const ctx,
        void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        return static_cast<Dev*>(ctx)->recv(data, bytes, timeout_ms);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t xfer_object(
        void* const ctx,
        const void* const tx,
        const std::size_t tx_bytes,
        void* const rx,
        const std::size_t rx_bytes,
        const int timeout_ms) noexcept
    {
        return static_cast<Dev*>(ctx)->xfer(tx, tx_bytes, rx, rx_bytes, timeout_ms);
    }
};

struct AnySpi {
    using SendFn = esp_err_t (*)(void*, const void*, std::size_t, std::uint32_t, std::uint32_t) noexcept;
    using RecvFn = esp_err_t (*)(void*, void*, std::size_t, std::uint32_t, std::uint32_t) noexcept;
    using XferFn = esp_err_t (*)(void*, const void*, void*, std::size_t, std::uint32_t, std::uint32_t) noexcept;

    void* ctx{};
    SendFn send_fn{};
    RecvFn recv_fn{};
    XferFn xfer_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return send_fn != nullptr && recv_fn != nullptr && xfer_fn != nullptr;
    }

    template <typename Dev>
    [[nodiscard]] static constexpr AnySpi bind() noexcept
    {
        return AnySpi{
            .ctx = nullptr,
            .send_fn = &send_static<Dev>,
            .recv_fn = &recv_static<Dev>,
            .xfer_fn = &xfer_static<Dev>,
        };
    }

    template <typename Dev>
    [[nodiscard]] static constexpr AnySpi bind(Dev& dev) noexcept
    {
        return AnySpi{
            .ctx = &dev,
            .send_fn = &send_object<Dev>,
            .recv_fn = &recv_object<Dev>,
            .xfer_fn = &xfer_object<Dev>,
        };
    }

    [[nodiscard]] esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) const noexcept
    {
        if (send_fn == nullptr || (data == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        return send_fn(ctx, data, bytes, hz, flags);
    }

    [[nodiscard]] esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) const noexcept
    {
        if (recv_fn == nullptr || (data == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        return recv_fn(ctx, data, bytes, hz, flags);
    }

    [[nodiscard]] esp_err_t xfer(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) const noexcept
    {
        if (xfer_fn == nullptr ||
            (tx == nullptr && bytes != 0U) ||
            (rx == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        return xfer_fn(ctx, tx, rx, bytes, hz, flags);
    }

    template <typename T, std::size_t Extent>
        requires AnyIoBytes<T>
    [[nodiscard]] esp_err_t send(
        const std::span<T, Extent> data,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) const noexcept
    {
        return send(data.data(), data.size_bytes(), hz, flags);
    }

    template <typename T, std::size_t Extent>
        requires(AnyIoBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] esp_err_t recv(
        const std::span<T, Extent> data,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) const noexcept
    {
        return recv(data.data(), data.size_bytes(), hz, flags);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(AnyIoBytes<Tx> && AnyIoBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] esp_err_t xfer(
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) const noexcept
    {
        if (tx.size_bytes() != rx.size_bytes()) {
            return ESP_ERR_INVALID_ARG;
        }
        return xfer(tx.data(), rx.data(), tx.size_bytes(), hz, flags);
    }

private:
    template <typename Dev>
    [[nodiscard]] static esp_err_t send_static(
        void*,
        const void* const data,
        const std::size_t bytes,
        const std::uint32_t hz,
        const std::uint32_t flags) noexcept
    {
        return Dev::send(data, bytes, hz, flags);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t recv_static(
        void*,
        void* const data,
        const std::size_t bytes,
        const std::uint32_t hz,
        const std::uint32_t flags) noexcept
    {
        return Dev::recv(data, bytes, hz, flags);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t xfer_static(
        void*,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz,
        const std::uint32_t flags) noexcept
    {
        return Dev::xfer(tx, rx, bytes, hz, flags);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t send_object(
        void* const ctx,
        const void* const data,
        const std::size_t bytes,
        const std::uint32_t hz,
        const std::uint32_t flags) noexcept
    {
        return static_cast<Dev*>(ctx)->send(data, bytes, hz, flags);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t recv_object(
        void* const ctx,
        void* const data,
        const std::size_t bytes,
        const std::uint32_t hz,
        const std::uint32_t flags) noexcept
    {
        return static_cast<Dev*>(ctx)->recv(data, bytes, hz, flags);
    }

    template <typename Dev>
    [[nodiscard]] static esp_err_t xfer_object(
        void* const ctx,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz,
        const std::uint32_t flags) noexcept
    {
        return static_cast<Dev*>(ctx)->xfer(tx, rx, bytes, hz, flags);
    }
};

struct AnyUart {
    using WriteFn = Result<std::size_t> (*)(void*, const void*, std::size_t) noexcept;
    using ReadFn = Result<std::size_t> (*)(void*, void*, std::size_t, std::uint32_t) noexcept;
    using WaitFn = esp_err_t (*)(void*, std::uint32_t) noexcept;
    using FlushFn = esp_err_t (*)(void*) noexcept;
    using AvailableFn = esp_err_t (*)(void*, std::size_t&) noexcept;

    void* ctx{};
    WriteFn write_fn{};
    ReadFn read_fn{};
    WaitFn wait_fn{};
    FlushFn flush_fn{};
    AvailableFn available_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return write_fn != nullptr &&
            read_fn != nullptr &&
            wait_fn != nullptr &&
            flush_fn != nullptr &&
            available_fn != nullptr;
    }

    template <typename Port>
    [[nodiscard]] static constexpr AnyUart bind() noexcept
    {
        return AnyUart{
            .ctx = nullptr,
            .write_fn = &write_static<Port>,
            .read_fn = &read_static<Port>,
            .wait_fn = &wait_static<Port>,
            .flush_fn = &flush_static<Port>,
            .available_fn = &available_static<Port>,
        };
    }

    template <typename Port>
    [[nodiscard]] static constexpr AnyUart bind(Port& port) noexcept
    {
        return AnyUart{
            .ctx = &port,
            .write_fn = &write_object<Port>,
            .read_fn = &read_object<Port>,
            .wait_fn = &wait_object<Port>,
            .flush_fn = &flush_object<Port>,
            .available_fn = &available_object<Port>,
        };
    }

    [[nodiscard]] Result<std::size_t> write(
        const void* const data,
        const std::size_t bytes) const noexcept
    {
        if (write_fn == nullptr || (data == nullptr && bytes != 0U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return write_fn(ctx, data, bytes);
    }

    [[nodiscard]] Result<std::size_t> read(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms = 0U) const noexcept
    {
        if (read_fn == nullptr || (data == nullptr && bytes != 0U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return read_fn(ctx, data, bytes, timeout_ms);
    }

    template <typename Rep, typename Period>
    [[nodiscard]] Result<std::size_t> read(
        void* const data,
        const std::size_t bytes,
        const std::chrono::duration<Rep, Period> timeout) const noexcept
    {
        return read(data, bytes, rtos::milliseconds(timeout));
    }

    template <typename T, std::size_t Extent>
        requires AnyIoBytes<T>
    [[nodiscard]] Result<std::size_t> write(const std::span<T, Extent> data) const noexcept
    {
        return write(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires(AnyIoBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] Result<std::size_t> read(
        const std::span<T, Extent> data,
        const std::uint32_t timeout_ms = 0U) const noexcept
    {
        return read(data.data(), data.size_bytes(), timeout_ms);
    }

    template <typename T, std::size_t Extent, typename Rep, typename Period>
        requires(AnyIoBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] Result<std::size_t> read(
        const std::span<T, Extent> data,
        const std::chrono::duration<Rep, Period> timeout) const noexcept
    {
        return read(data.data(), data.size_bytes(), timeout);
    }

    [[nodiscard]] esp_err_t wait(const std::uint32_t timeout_ms = 1000U) const noexcept
    {
        if (wait_fn == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return wait_fn(ctx, timeout_ms);
    }

    template <typename Rep, typename Period>
    [[nodiscard]] esp_err_t wait(const std::chrono::duration<Rep, Period> timeout) const noexcept
    {
        return wait(rtos::milliseconds(timeout));
    }

    [[nodiscard]] esp_err_t flush() const noexcept
    {
        if (flush_fn == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return flush_fn(ctx);
    }

    [[nodiscard]] esp_err_t available(std::size_t& bytes) const noexcept
    {
        if (available_fn == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return available_fn(ctx, bytes);
    }

private:
    template <typename Port>
    [[nodiscard]] static Result<std::size_t> write_static(
        void*,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        return Port::write(data, bytes);
    }

    template <typename Port>
    [[nodiscard]] static Result<std::size_t> read_static(
        void*,
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms) noexcept
    {
        return Port::read(data, bytes, timeout_ms);
    }

    template <typename Port>
    [[nodiscard]] static esp_err_t wait_static(void*, const std::uint32_t timeout_ms) noexcept
    {
        return Port::wait(timeout_ms);
    }

    template <typename Port>
    [[nodiscard]] static esp_err_t flush_static(void*) noexcept
    {
        return Port::flush();
    }

    template <typename Port>
    [[nodiscard]] static esp_err_t available_static(void*, std::size_t& bytes) noexcept
    {
        return Port::available(bytes);
    }

    template <typename Port>
    [[nodiscard]] static Result<std::size_t> write_object(
        void* const ctx,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        return static_cast<Port*>(ctx)->write(data, bytes);
    }

    template <typename Port>
    [[nodiscard]] static Result<std::size_t> read_object(
        void* const ctx,
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms) noexcept
    {
        return static_cast<Port*>(ctx)->read(data, bytes, timeout_ms);
    }

    template <typename Port>
    [[nodiscard]] static esp_err_t wait_object(void* const ctx, const std::uint32_t timeout_ms) noexcept
    {
        return static_cast<Port*>(ctx)->wait(timeout_ms);
    }

    template <typename Port>
    [[nodiscard]] static esp_err_t flush_object(void* const ctx) noexcept
    {
        return static_cast<Port*>(ctx)->flush();
    }

    template <typename Port>
    [[nodiscard]] static esp_err_t available_object(void* const ctx, std::size_t& bytes) noexcept
    {
        return static_cast<Port*>(ctx)->available(bytes);
    }
};

}  // namespace arc
