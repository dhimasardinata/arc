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

template <std::uint32_t Backlog = 4,
          std::size_t BurstBytes = 64,
          std::uint32_t Weight = 0,
          CopyBackend Backend = CopyBackend::auto_dma>
struct Copy {
    static_assert(Backlog != 0U, "copy backlog must be non-zero");

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
            __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
            __atomic_add_fetch(&state.bytes, bytes, __ATOMIC_RELEASE);
        }

        return ret;
    }

    template <typename Hook = void, typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t send(std::span<T> dst, std::span<const T> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return send<Hook>(dst.data(), src.data(), src.size_bytes());
    }

    static esp_err_t copy(void* const dst, const void* const src, const std::size_t bytes) noexcept
    {
        const auto ret = send(dst, src, bytes);
        if (ret != ESP_OK) {
            return ret;
        }
        wait(sent());
        return ESP_OK;
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t copy(std::span<T> dst, std::span<const T> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return copy(dst.data(), src.data(), src.size_bytes());
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

    static void wait(const std::uint32_t target) noexcept
    {
        while (seq_before(done(), target)) {
            __asm__ __volatile__("nop");
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
