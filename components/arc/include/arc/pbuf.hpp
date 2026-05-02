#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "lwip/pbuf.h"

#include "arc/result.hpp"

namespace arc::net {

struct Pbuf {
    constexpr Pbuf() noexcept = default;

    explicit constexpr Pbuf(pbuf* const raw) noexcept
        : raw_(raw)
    {
    }

    Pbuf(const Pbuf&) = delete;
    Pbuf& operator=(const Pbuf&) = delete;

    Pbuf(Pbuf&& other) noexcept
        : raw_(std::exchange(other.raw_, nullptr))
    {
    }

    Pbuf& operator=(Pbuf&& other) noexcept
    {
        if (this != &other) {
            reset();
            raw_ = std::exchange(other.raw_, nullptr);
        }
        return *this;
    }

    ~Pbuf()
    {
        reset();
    }

    [[nodiscard]] static Result<Pbuf> alloc(
        const pbuf_layer layer,
        const std::uint16_t bytes,
        const pbuf_type type = PBUF_RAM) noexcept
    {
        auto* const raw = pbuf_alloc(layer, bytes, type);
        if (raw == nullptr) {
            return fail(ESP_ERR_NO_MEM);
        }
        return Pbuf{raw};
    }

    [[nodiscard]] static Result<Pbuf> transport_ram(const std::uint16_t bytes) noexcept
    {
        return alloc(PBUF_TRANSPORT, bytes, PBUF_RAM);
    }

    [[nodiscard]] std::span<std::byte> payload() noexcept
    {
        if (raw_ == nullptr || raw_->payload == nullptr) {
            return {};
        }
        return {static_cast<std::byte*>(raw_->payload), raw_->len};
    }

    [[nodiscard]] std::span<const std::byte> payload() const noexcept
    {
        if (raw_ == nullptr || raw_->payload == nullptr) {
            return {};
        }
        return {static_cast<const std::byte*>(raw_->payload), raw_->len};
    }

    [[nodiscard]] std::uint16_t len() const noexcept
    {
        return raw_ == nullptr ? 0U : raw_->len;
    }

    [[nodiscard]] std::uint16_t total_len() const noexcept
    {
        return raw_ == nullptr ? 0U : raw_->tot_len;
    }

    [[nodiscard]] pbuf* native() const noexcept
    {
        return raw_;
    }

    [[nodiscard]] pbuf* release() noexcept
    {
        return std::exchange(raw_, nullptr);
    }

    void reset(pbuf* const next = nullptr) noexcept
    {
        if (raw_ != nullptr) {
            static_cast<void>(pbuf_free(raw_));
        }
        raw_ = next;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return raw_ != nullptr;
    }

private:
    pbuf* raw_{};
};

}  // namespace arc::net
