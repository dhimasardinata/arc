#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_async_memcpy.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"

#include "arc/cache.hpp"
#include "arc/sdk.hpp"
#include "arc/seq.hpp"

namespace arc {

enum class CopyBackend : std::uint8_t {
    auto_dma,
    ahb,
    axi,
    cp,
};

template <typename Hook>
concept CopyHook = requires {
    { Hook::done() } -> std::same_as<bool>;
};

template <typename Dst, typename Src>
concept CopySpan =
    !std::is_const_v<Dst> &&
    std::is_trivially_copyable_v<std::remove_cv_t<Dst>> &&
    std::same_as<std::remove_cv_t<Dst>, std::remove_cv_t<Src>>;

template <bool Strict = false>
struct CopyTicket {
    std::uint32_t target{};
    void* dst{};
    std::size_t bytes{};
};

template <std::uint32_t Backlog = 4,
          std::size_t BurstBytes = 64,
          std::uint32_t Weight = 0,
          CopyBackend Backend = CopyBackend::auto_dma>
struct Copy {
    static_assert(Backlog != 0U, "copy backlog must be non-zero");

    using Ticket = CopyTicket<false>;
    using StrictTicket = CopyTicket<true>;

    static void boot()
    {
        if (state.driver != nullptr) {
            return;
        }

        async_memcpy_config_t config{};
        config.backlog = Backlog;
        config.weight = Weight;
        config.dma_burst_size = BurstBytes;
        config.flags = 0;

        install(&config);
    }

    template <typename Hook = void>
    static esp_err_t send(void* const dst, const void* const src, const std::size_t bytes) noexcept
    {
        return send_impl<Hook>(dst, src, bytes, nullptr);
    }

    template <typename Hook = void, typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t send(std::span<Dst> dst, std::span<Src> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return send<Hook>(dst.data(), src.data(), src.size_bytes());
    }

    static esp_err_t copy(void* const dst, const void* const src, const std::size_t bytes) noexcept
    {
        std::uint32_t target{};
        const auto ret = send_impl<void>(dst, src, bytes, &target);
        if (ret != ESP_OK) {
            return ret;
        }
        wait(target);
        return ESP_OK;
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t copy(std::span<Dst> dst, std::span<Src> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return copy(dst.data(), src.data(), src.size_bytes());
    }

    static esp_err_t copy_coherent(void* const dst, const void* const src, const std::size_t bytes) noexcept
    {
        Ticket ticket{};
        const auto ret = send_coherent(ticket, dst, src, bytes);
        if (ret != ESP_OK) {
            return ret;
        }
        return finish_coherent(ticket);
    }

    static esp_err_t copy_coherent_strict(
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        StrictTicket ticket{};
        const auto ret = send_coherent_strict(ticket, dst, src, bytes);
        if (ret != ESP_OK) {
            return ret;
        }
        return finish_coherent(ticket);
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t copy_coherent(std::span<Dst> dst, std::span<Src> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return copy_coherent(dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t copy_coherent_strict(std::span<Dst> dst, std::span<Src> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return copy_coherent_strict(dst.data(), src.data(), src.size_bytes());
    }

    static esp_err_t send_coherent(
        Ticket& ticket,
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        return send_coherent_impl(ticket, dst, src, bytes);
    }

    static esp_err_t send_coherent_strict(
        StrictTicket& ticket,
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        return send_coherent_impl(ticket, dst, src, bytes);
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t send_coherent(
        Ticket& ticket,
        std::span<Dst> dst,
        std::span<Src> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return send_coherent(ticket, dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t send_coherent_strict(
        StrictTicket& ticket,
        std::span<Dst> dst,
        std::span<Src> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return send_coherent_strict(ticket, dst.data(), src.data(), src.size_bytes());
    }

    [[nodiscard]] static bool ready(const Ticket& ticket) noexcept
    {
        return seq_reached(done(), ticket.target);
    }

    [[nodiscard]] static bool ready(const StrictTicket& ticket) noexcept
    {
        return seq_reached(done(), ticket.target);
    }

    static esp_err_t finish_coherent(const Ticket& ticket) noexcept
    {
        return finish_coherent_impl(ticket);
    }

    static esp_err_t finish_coherent(const StrictTicket& ticket) noexcept
    {
        return finish_coherent_impl(ticket);
    }

    [[nodiscard]] static std::uint32_t sent() noexcept
    {
        return __atomic_load_n(&state.sent, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t done() noexcept
    {
        return __atomic_load_n(&state.done, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::size_t bytes() noexcept
    {
        return __atomic_load_n(&state.bytes, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static bool idle() noexcept
    {
        return seq_reached(done(), sent());
    }

    static void wait() noexcept
    {
        wait(sent());
    }

private:
    struct State {
        async_memcpy_handle_t driver{};
        alignas(cache_line) std::uint32_t sent{};
        alignas(cache_line) std::uint32_t done{};
        std::size_t bytes{};
    };

    constinit static inline State state{};

    template <typename Hook>
    IRAM_ATTR static bool on_done(async_memcpy_handle_t, async_memcpy_event_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.done, 1U, __ATOMIC_RELEASE);

        if constexpr (CopyHook<Hook>) {
            return Hook::done();
        } else {
            return false;
        }
    }

    template <typename Hook>
    static esp_err_t send_impl(
        void* const dst,
        const void* const src,
        const std::size_t bytes,
        std::uint32_t* const target) noexcept
    {
        boot();

        if (dst == nullptr || src == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ret = esp_async_memcpy(
            state.driver,
            dst,
            const_cast<void*>(src),
            bytes,
            &on_done<Hook>,
            nullptr);

        if (ret == ESP_OK) {
            const auto seq = __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
            __atomic_add_fetch(&state.bytes, bytes, __ATOMIC_RELEASE);
            if (target != nullptr) {
                *target = seq;
            }
        }

        return ret;
    }

    static void wait(const std::uint32_t target) noexcept
    {
        while (seq_before(done(), target)) {
            __asm__ __volatile__("nop");
        }
    }

    template <bool Strict>
    static esp_err_t send_coherent_impl(
        CopyTicket<Strict>& ticket,
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        if (dst == nullptr || src == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto source = [&]() noexcept -> esp_err_t {
            if constexpr (Strict) {
                return Cache::to_device_strict(const_cast<void*>(src), bytes);
            } else {
                return Cache::to_device(const_cast<void*>(src), bytes);
            }
        }();
        if (source != ESP_OK) {
            return source;
        }

        const auto target = [&]() noexcept -> esp_err_t {
            if constexpr (Strict) {
                return Cache::discard_strict(dst, bytes);
            } else {
                return Cache::discard(dst, bytes);
            }
        }();
        if (target != ESP_OK) {
            return target;
        }

        std::uint32_t seq{};
        const auto sent = send_impl<void>(dst, src, bytes, &seq);
        if (sent == ESP_OK) {
            ticket = CopyTicket<Strict>{
                .target = seq,
                .dst = dst,
                .bytes = bytes,
            };
        }

        return sent;
    }

    template <bool Strict>
    static esp_err_t finish_coherent_impl(const CopyTicket<Strict>& ticket) noexcept
    {
        if (ticket.dst == nullptr || ticket.bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        wait(ticket.target);

        if constexpr (Strict) {
            return Cache::from_device_strict(ticket.dst, ticket.bytes);
        } else {
            return Cache::from_device(ticket.dst, ticket.bytes);
        }
    }

    static void install(const async_memcpy_config_t* const config)
    {
        if constexpr (Backend == CopyBackend::auto_dma) {
            ESP_ERROR_CHECK(esp_async_memcpy_install(config, &state.driver));
        } else if constexpr (Backend == CopyBackend::ahb) {
#if SOC_HAS(AHB_GDMA)
            ESP_ERROR_CHECK(esp_async_memcpy_install_gdma_ahb(config, &state.driver));
#else
            ESP_ERROR_CHECK(ESP_ERR_NOT_SUPPORTED);
#endif
        } else if constexpr (Backend == CopyBackend::axi) {
#if SOC_HAS(AXI_GDMA)
            ESP_ERROR_CHECK(esp_async_memcpy_install_gdma_axi(config, &state.driver));
#else
            ESP_ERROR_CHECK(ESP_ERR_NOT_SUPPORTED);
#endif
        } else {
#if SOC_CP_DMA_SUPPORTED
            ESP_ERROR_CHECK(esp_async_memcpy_install_cpdma(config, &state.driver));
#else
            ESP_ERROR_CHECK(ESP_ERR_NOT_SUPPORTED);
#endif
        }
    }
};

}  // namespace arc
