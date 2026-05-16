#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_cache.h"
#include "esp_err.h"

#include "arc/caps.hpp"
#include "arc/result.hpp"
#include "arc/sdk.hpp"

#ifndef ARC_ENABLE_UNSAFE_CACHE_RAW
#define ARC_ENABLE_UNSAFE_CACHE_RAW 0
#endif

namespace arc {

struct UnsafeCacheRaw {
    explicit constexpr UnsafeCacheRaw() noexcept = default;
};

inline constexpr UnsafeCacheRaw unsafe_raw{};

struct CacheLines {
    void* data{};
    std::size_t bytes{};
};

enum class CacheState : std::uint8_t {
    cpu,
    device,
};

namespace detail {

struct DmaBufAdopt {
    explicit constexpr DmaBufAdopt() noexcept = default;
};

}  // namespace detail

struct Cache {
    static constexpr int invalidate = static_cast<int>(ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    static constexpr int unaligned = static_cast<int>(ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    static constexpr int dir_c2m = static_cast<int>(ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    static constexpr int dir_m2c = static_cast<int>(ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    static constexpr int type_data = static_cast<int>(ESP_CACHE_MSYNC_FLAG_TYPE_DATA);

    [[nodiscard]] static bool aligned(const void* const data) noexcept
    {
        return (reinterpret_cast<std::uintptr_t>(data) & (cache_line - 1U)) == 0U;
    }

    [[nodiscard]] static bool whole_lines(const std::size_t bytes) noexcept
    {
        return (bytes & (cache_line - 1U)) == 0U;
    }

    [[nodiscard]] static Result<CacheLines> lines(void* const data, const std::size_t bytes) noexcept
    {
        if (data == nullptr || bytes == 0U || !aligned(data) || !whole_lines(bytes)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return CacheLines{.data = data, .bytes = bytes};
    }

    template <typename T, std::size_t Extent>
        requires(!std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    [[nodiscard]] static Result<CacheLines> lines(const std::span<T, Extent> data) noexcept
    {
        return lines(data.data(), data.size_bytes());
    }

    static esp_err_t sync(void* const data, const std::size_t bytes, const int flags) noexcept
    {
        if (data == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_cache_msync(data, bytes, flags | type_data);
    }

    static esp_err_t strict(void* const data, const std::size_t bytes, const int flags) noexcept
    {
        if (data == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        if (!aligned(data) || !whole_lines(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_cache_msync(data, bytes, (flags & ~unaligned) | type_data);
    }

    static esp_err_t to_device(void* const data, const std::size_t bytes) noexcept
    {
        return sync(data, bytes, dir_c2m | unaligned);
    }

    static esp_err_t to_aligned(void* const data, const std::size_t bytes) noexcept
    {
        return strict(data, bytes, dir_c2m);
    }

    static esp_err_t to_device(const CacheLines region) noexcept
    {
        return to_aligned(region.data, region.bytes);
    }

    static esp_err_t to_aligned(const CacheLines region) noexcept
    {
        return to_device(region);
    }

    static esp_err_t from_device(void* const data, const std::size_t bytes) noexcept
    {
        return from_aligned(data, bytes);
    }

    static esp_err_t from_aligned(void* const data, const std::size_t bytes) noexcept
    {
        return strict(data, bytes, dir_m2c);
    }

    static esp_err_t from_device(const CacheLines region) noexcept
    {
        return from_aligned(region.data, region.bytes);
    }

    static esp_err_t from_aligned(const CacheLines region) noexcept
    {
        return from_device(region);
    }

#if ARC_ENABLE_UNSAFE_CACHE_RAW
    [[deprecated("unsafe unaligned invalidation can discard unrelated dirty cache-line neighbors; prefer arc::dmabuf and from_aligned")]]
    static esp_err_t from_raw(
        UnsafeCacheRaw,
        void* const data,
        const std::size_t bytes) noexcept
    {
        return sync(data, bytes, dir_m2c | unaligned);
    }
#endif

    static esp_err_t discard(void* const data, const std::size_t bytes) noexcept
    {
        return discard_aligned(data, bytes);
    }

    static esp_err_t discard_aligned(void* const data, const std::size_t bytes) noexcept
    {
        return strict(data, bytes, dir_c2m | invalidate);
    }

    static esp_err_t discard(const CacheLines region) noexcept
    {
        return discard_aligned(region.data, region.bytes);
    }

    static esp_err_t discard_aligned(const CacheLines region) noexcept
    {
        return discard(region);
    }

#if ARC_ENABLE_UNSAFE_CACHE_RAW
    [[deprecated("unsafe unaligned discard can invalidate unrelated dirty cache-line neighbors; prefer arc::dmabuf and discard_aligned")]]
    static esp_err_t discard_raw(
        UnsafeCacheRaw,
        void* const data,
        const std::size_t bytes) noexcept
    {
        return sync(data, bytes, dir_c2m | invalidate | unaligned);
    }
#endif

    [[nodiscard]] static std::size_t line(const void* const data) noexcept
    {
        return esp_cache_get_line_size_by_addr(data);
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_device(const std::span<T, Extent> data) noexcept
    {
        return to_device(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_aligned(const std::span<T, Extent> data) noexcept
    {
        return to_aligned(data.data(), data.size_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_device(const CapsBuf<T>& data) noexcept
    {
        return to_aligned(const_cast<T*>(data.data()), data.storage_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_aligned(const CapsBuf<T>& data) noexcept
    {
        return to_device(data);
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_device(const std::span<T, Extent> data) noexcept
    {
        return from_device(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_aligned(const std::span<T, Extent> data) noexcept
    {
        return from_aligned(data.data(), data.size_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_device(CapsBuf<T>& data) noexcept
    {
        return from_aligned(data.data(), data.storage_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_aligned(CapsBuf<T>& data) noexcept
    {
        return from_device(data);
    }

#if ARC_ENABLE_UNSAFE_CACHE_RAW
    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    [[deprecated("unsafe unaligned invalidation can discard unrelated dirty cache-line neighbors; prefer arc::dmabuf and from_aligned")]]
    static esp_err_t from_raw(
        UnsafeCacheRaw token,
        const std::span<T, Extent> data) noexcept
    {
        return from_raw(token, data.data(), data.size_bytes());
    }
#endif

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard(const std::span<T, Extent> data) noexcept
    {
        return discard(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard_aligned(const std::span<T, Extent> data) noexcept
    {
        return discard_aligned(data.data(), data.size_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard(CapsBuf<T>& data) noexcept
    {
        return discard_aligned(data.data(), data.storage_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard_aligned(CapsBuf<T>& data) noexcept
    {
        return discard(data);
    }

#if ARC_ENABLE_UNSAFE_CACHE_RAW
    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    [[deprecated("unsafe unaligned discard can invalidate unrelated dirty cache-line neighbors; prefer arc::dmabuf and discard_aligned")]]
    static esp_err_t discard_raw(
        UnsafeCacheRaw token,
        const std::span<T, Extent> data) noexcept
    {
        return discard_raw(token, data.data(), data.size_bytes());
    }
#endif
};

template <typename T, CacheState State = CacheState::cpu>
class DmaBuf {
public:
    using value_type = T;
    static constexpr CacheState state = State;

    constexpr DmaBuf() noexcept = default;

    constexpr DmaBuf(CapsBuf<T>&& buffer) noexcept
        requires(State == CacheState::cpu)
        : buffer_(std::move(buffer))
    {
    }

    constexpr DmaBuf(CapsBuf<T>&& buffer, detail::DmaBufAdopt) noexcept
        : buffer_(std::move(buffer))
    {
    }

    DmaBuf(const DmaBuf&) = delete;
    DmaBuf& operator=(const DmaBuf&) = delete;

    constexpr DmaBuf(DmaBuf&&) noexcept = default;
    constexpr DmaBuf& operator=(DmaBuf&&) noexcept = default;

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return static_cast<bool>(buffer_);
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return buffer_.size();
    }

    [[nodiscard]] constexpr std::size_t bytes() const noexcept
    {
        return buffer_.bytes();
    }

    [[nodiscard]] constexpr std::size_t storage_bytes() const noexcept
    {
        return buffer_.storage_bytes();
    }

    [[nodiscard]] constexpr void* addr() noexcept
    {
        return buffer_.data();
    }

    [[nodiscard]] constexpr const void* addr() const noexcept
    {
        return buffer_.data();
    }

    [[nodiscard]] constexpr T* data() noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_.data();
    }

    [[nodiscard]] constexpr const T* data() const noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_.data();
    }

    [[nodiscard]] constexpr T& operator[](const std::size_t index) noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_[index];
    }

    [[nodiscard]] constexpr const T& operator[](const std::size_t index) const noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_[index];
    }

    [[nodiscard]] constexpr std::span<T> view() noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_.view();
    }

    [[nodiscard]] constexpr std::span<const T> view() const noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_.view();
    }

    [[nodiscard]] constexpr std::span<std::byte> storage() noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_.storage();
    }

    [[nodiscard]] constexpr std::span<const std::byte> storage() const noexcept
        requires(State == CacheState::cpu)
    {
        return buffer_.storage();
    }

    [[nodiscard]] Result<DmaBuf<T, CacheState::device>> to_device() && noexcept
        requires(State == CacheState::cpu)
    {
        const auto err = Cache::to_device(buffer_);
        if (err != ESP_OK) {
            return fail(err);
        }
        return ok(DmaBuf<T, CacheState::device>{std::move(buffer_), detail::DmaBufAdopt{}});
    }

    [[nodiscard]] Result<DmaBuf<T, CacheState::device>> discard() && noexcept
        requires(State == CacheState::cpu)
    {
        const auto err = Cache::discard(buffer_);
        if (err != ESP_OK) {
            return fail(err);
        }
        return ok(DmaBuf<T, CacheState::device>{std::move(buffer_), detail::DmaBufAdopt{}});
    }

    [[nodiscard]] Result<DmaBuf<T, CacheState::cpu>> from_device() && noexcept
        requires(State == CacheState::device)
    {
        const auto err = Cache::from_device(buffer_);
        if (err != ESP_OK) {
            return fail(err);
        }
        return ok(DmaBuf<T, CacheState::cpu>{std::move(buffer_), detail::DmaBufAdopt{}});
    }

    [[nodiscard]] constexpr CapsBuf<T> take() && noexcept
        requires(State == CacheState::cpu)
    {
        return std::move(buffer_);
    }

private:
    CapsBuf<T> buffer_{};
};

}  // namespace arc
