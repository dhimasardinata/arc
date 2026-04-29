#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_err.h"

namespace arc {

template <typename T>
concept AnyIoBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

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

}  // namespace arc
