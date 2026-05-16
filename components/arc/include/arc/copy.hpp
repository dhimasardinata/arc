#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_async_memcpy.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"

#include "arc/cache.hpp"
#include "arc/init.hpp"
#include "arc/result.hpp"
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
    std::size_t cache_bytes{};
};

template <std::uint32_t Backlog = 4,
          std::size_t BurstBytes = 64,
          std::uint32_t Weight = 0,
          CopyBackend Backend = CopyBackend::auto_dma>
struct Copy {
    static_assert(Backlog != 0U, "copy backlog must be non-zero");

    using Ticket = CopyTicket<false>;
    using StrictTicket = CopyTicket<true>;

    template <bool Strict>
    class LeaseBase {
    public:
        constexpr LeaseBase() noexcept = default;

        explicit constexpr LeaseBase(const CopyTicket<Strict>& ticket) noexcept
            : ticket_(ticket)
            , active_(true)
        {
        }

        LeaseBase(const LeaseBase&) = delete;
        LeaseBase& operator=(const LeaseBase&) = delete;

        constexpr LeaseBase(LeaseBase&& other) noexcept
            : ticket_(other.ticket_)
            , active_(other.active_)
        {
            other.active_ = false;
        }

        LeaseBase& operator=(LeaseBase&& other) noexcept
        {
            if (this != &other) {
                static_cast<void>(finish());
                ticket_ = other.ticket_;
                active_ = other.active_;
                other.active_ = false;
            }
            return *this;
        }

        ~LeaseBase()
        {
            static_cast<void>(finish());
        }

        [[nodiscard]] constexpr bool active() const noexcept
        {
            return active_;
        }

        [[nodiscard]] constexpr const CopyTicket<Strict>& ticket() const noexcept
        {
            return ticket_;
        }

        [[nodiscard]] Status finish() noexcept
        {
            if (!active_) {
                return ok();
            }
            const auto err = finish_impl(ticket_);
            active_ = false;
            return status(err);
        }

    private:
        CopyTicket<Strict> ticket_{};
        bool active_{};
    };

    using Lease = LeaseBase<false>;
    using StrictLease = LeaseBase<true>;

    template <bool Strict, typename Dst, typename Src>
    class OwnedLeaseBase {
    public:
        constexpr OwnedLeaseBase() noexcept = default;

        constexpr OwnedLeaseBase(
            CapsBuf<Dst>&& dst,
            CapsBuf<Src>&& src,
            const CopyTicket<Strict>& ticket) noexcept
            : dst_(std::move(dst))
            , src_(std::move(src))
            , lease_(ticket)
        {
        }

        OwnedLeaseBase(const OwnedLeaseBase&) = delete;
        OwnedLeaseBase& operator=(const OwnedLeaseBase&) = delete;

        constexpr OwnedLeaseBase(OwnedLeaseBase&& other) noexcept
            : dst_(std::move(other.dst_))
            , src_(std::move(other.src_))
            , lease_(std::move(other.lease_))
        {
        }

        OwnedLeaseBase& operator=(OwnedLeaseBase&& other) noexcept
        {
            if (this != &other) {
                static_cast<void>(finish());
                dst_ = std::move(other.dst_);
                src_ = std::move(other.src_);
                lease_ = std::move(other.lease_);
            }
            return *this;
        }

        ~OwnedLeaseBase()
        {
            static_cast<void>(finish());
        }

        [[nodiscard]] constexpr bool active() const noexcept
        {
            return lease_.active();
        }

        [[nodiscard]] constexpr const CopyTicket<Strict>& ticket() const noexcept
        {
            return lease_.ticket();
        }

        [[nodiscard]] constexpr CapsBuf<Dst>& dst() noexcept
        {
            return dst_;
        }

        [[nodiscard]] constexpr const CapsBuf<Dst>& dst() const noexcept
        {
            return dst_;
        }

        [[nodiscard]] constexpr CapsBuf<Src>& src() noexcept
        {
            return src_;
        }

        [[nodiscard]] constexpr const CapsBuf<Src>& src() const noexcept
        {
            return src_;
        }

        [[nodiscard]] Status finish() noexcept
        {
            return lease_.finish();
        }

        [[nodiscard]] Result<CapsBuf<Dst>> take_dst() noexcept
        {
            const auto ready = finish();
            if (!ready) {
                return fail(ready.error());
            }
            return ok(std::move(dst_));
        }

        [[nodiscard]] Result<CapsBuf<Src>> take_src() noexcept
        {
            const auto ready = finish();
            if (!ready) {
                return fail(ready.error());
            }
            return ok(std::move(src_));
        }

    private:
        CapsBuf<Dst> dst_{};
        CapsBuf<Src> src_{};
        LeaseBase<Strict> lease_{};
    };

    template <typename Dst, typename Src>
    using OwnedLease = OwnedLeaseBase<false, Dst, Src>;

    template <typename Dst, typename Src>
    using StrictOwnedLease = OwnedLeaseBase<true, Dst, Src>;

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        async_memcpy_config_t config{};
        config.backlog = Backlog;
        config.weight = Weight;
        config.dma_burst_size = BurstBytes;
        config.flags = 0;

        const auto ready = status(install(&config)).and_then([&]() noexcept -> Status {
            Init::pass(state.init);
            return ok();
        });
        if (!ready) {
            state.driver = nullptr;
            Init::fail(state.init);
        }
        return status_code(ready);
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    template <typename Hook = void>
    static esp_err_t send(void* const dst, const void* const src, const std::size_t bytes) noexcept
    {
        return send_impl<Hook>(dst, src, bytes, nullptr);
    }

    template <typename Hook = void, typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    static esp_err_t send(std::span<Dst, DstExtent> dst, std::span<Src, SrcExtent> src) noexcept
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

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    static esp_err_t copy(std::span<Dst, DstExtent> dst, std::span<Src, SrcExtent> src) noexcept
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

    static esp_err_t copy_strict(
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        StrictTicket ticket{};
        const auto ret = send_strict(ticket, dst, src, bytes);
        if (ret != ESP_OK) {
            return ret;
        }
        return finish_coherent(ticket);
    }

    [[nodiscard]] static Result<Lease> lease_coherent(
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        Ticket ticket{};
        const auto ret = send_coherent(ticket, dst, src, bytes);
        if (ret != ESP_OK) {
            return fail(ret);
        }
        return Lease{ticket};
    }

    [[nodiscard]] static Result<StrictLease> lease_strict(
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        StrictTicket ticket{};
        const auto ret = send_strict(ticket, dst, src, bytes);
        if (ret != ESP_OK) {
            return fail(ret);
        }
        return StrictLease{ticket};
    }

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    static esp_err_t copy_coherent(std::span<Dst, DstExtent> dst, std::span<Src, SrcExtent> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return copy_coherent(dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t copy_coherent(CapsBuf<Dst>& dst, const CapsBuf<Src>& src) noexcept
    {
        Ticket ticket{};
        const auto ret = send_coherent(ticket, dst, src);
        if (ret != ESP_OK) {
            return ret;
        }
        return finish_coherent(ticket);
    }

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    static esp_err_t copy_strict(std::span<Dst, DstExtent> dst, std::span<Src, SrcExtent> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return copy_strict(dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t copy_strict(CapsBuf<Dst>& dst, const CapsBuf<Src>& src) noexcept
    {
        StrictTicket ticket{};
        const auto ret = send_strict(ticket, dst, src);
        if (ret != ESP_OK) {
            return ret;
        }
        return finish_coherent(ticket);
    }

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    [[nodiscard]] static Result<Lease> lease_coherent(
        std::span<Dst, DstExtent> dst,
        std::span<Src, SrcExtent> src) noexcept
    {
        if (dst.size() < src.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        return lease_coherent(dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    [[nodiscard]] static Result<Lease> lease_coherent(CapsBuf<Dst>& dst, const CapsBuf<Src>& src) noexcept
    {
        Ticket ticket{};
        const auto ret = send_coherent(ticket, dst, src);
        if (ret != ESP_OK) {
            return fail(ret);
        }
        return Lease{ticket};
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    [[nodiscard]] static Result<OwnedLease<Dst, Src>> lease_coherent(
        CapsBuf<Dst>&& dst,
        CapsBuf<Src>&& src) noexcept
    {
        Ticket ticket{};
        const auto ret = send_coherent(ticket, dst, src);
        if (ret != ESP_OK) {
            return fail(ret);
        }
        return ok(OwnedLease<Dst, Src>{std::move(dst), std::move(src), ticket});
    }

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    [[nodiscard]] static Result<StrictLease> lease_strict(
        std::span<Dst, DstExtent> dst,
        std::span<Src, SrcExtent> src) noexcept
    {
        if (dst.size() < src.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        return lease_strict(dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    [[nodiscard]] static Result<StrictLease> lease_strict(CapsBuf<Dst>& dst, const CapsBuf<Src>& src) noexcept
    {
        StrictTicket ticket{};
        const auto ret = send_strict(ticket, dst, src);
        if (ret != ESP_OK) {
            return fail(ret);
        }
        return StrictLease{ticket};
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    [[nodiscard]] static Result<StrictOwnedLease<Dst, Src>> lease_strict(
        CapsBuf<Dst>&& dst,
        CapsBuf<Src>&& src) noexcept
    {
        StrictTicket ticket{};
        const auto ret = send_strict(ticket, dst, src);
        if (ret != ESP_OK) {
            return fail(ret);
        }
        return ok(StrictOwnedLease<Dst, Src>{std::move(dst), std::move(src), ticket});
    }

    static esp_err_t send_coherent(
        Ticket& ticket,
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        return send_cache(ticket, dst, src, bytes);
    }

    static esp_err_t send_strict(
        StrictTicket& ticket,
        void* const dst,
        const void* const src,
        const std::size_t bytes) noexcept
    {
        return send_cache(ticket, dst, src, bytes);
    }

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    static esp_err_t send_coherent(
        Ticket& ticket,
        std::span<Dst, DstExtent> dst,
        std::span<Src, SrcExtent> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return send_coherent(ticket, dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t send_coherent(Ticket& ticket, CapsBuf<Dst>& dst, const CapsBuf<Src>& src) noexcept
    {
        return send_cache_buf(ticket, dst, src);
    }

    template <typename Dst, std::size_t DstExtent, typename Src, std::size_t SrcExtent>
        requires CopySpan<Dst, Src>
    static esp_err_t send_strict(
        StrictTicket& ticket,
        std::span<Dst, DstExtent> dst,
        std::span<Src, SrcExtent> src) noexcept
    {
        if (dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        return send_strict(ticket, dst.data(), src.data(), src.size_bytes());
    }

    template <typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t send_strict(StrictTicket& ticket, CapsBuf<Dst>& dst, const CapsBuf<Src>& src) noexcept
    {
        return send_cache_buf(ticket, dst, src);
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
        return finish_impl(ticket);
    }

    static esp_err_t finish_coherent(const StrictTicket& ticket) noexcept
    {
        return finish_impl(ticket);
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

    [[nodiscard]] static bool wait_for(
        const Ticket& ticket,
        const std::uint32_t spin_budget) noexcept
    {
        return wait_for(ticket.target, spin_budget);
    }

    [[nodiscard]] static bool wait_for(
        const StrictTicket& ticket,
        const std::uint32_t spin_budget) noexcept
    {
        return wait_for(ticket.target, spin_budget);
    }

    template <typename YieldPolicy>
    [[nodiscard]] static bool wait_yield(
        const Ticket& ticket,
        const std::uint32_t spin_budget,
        const std::uint32_t max_yields) noexcept
    {
        return wait_yield<YieldPolicy>(ticket.target, spin_budget, max_yields);
    }

    template <typename YieldPolicy>
    [[nodiscard]] static bool wait_yield(
        const StrictTicket& ticket,
        const std::uint32_t spin_budget,
        const std::uint32_t max_yields) noexcept
    {
        return wait_yield<YieldPolicy>(ticket.target, spin_budget, max_yields);
    }

private:
    struct State {
        async_memcpy_handle_t driver{};
        std::uint32_t init{};
        MutexGateState gate{};
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
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        if (dst == nullptr || src == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        MutexGate guard(state.gate);
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

    [[nodiscard]] static bool wait_for(
        const std::uint32_t target,
        const std::uint32_t spin_budget) noexcept
    {
        auto spins = std::uint32_t{};
        while (seq_before(done(), target)) {
            if (spin_budget != 0U && spins++ >= spin_budget) {
                return false;
            }
            __asm__ __volatile__("nop");
        }
        return true;
    }

    template <typename YieldPolicy>
    [[nodiscard]] static bool wait_yield(
        const std::uint32_t target,
        const std::uint32_t spin_budget,
        const std::uint32_t max_yields) noexcept
    {
        auto spins = std::uint32_t{};
        auto yields = std::uint32_t{};
        while (seq_before(done(), target)) {
            if (spin_budget == 0U || spins++ < spin_budget) {
                __asm__ __volatile__("nop");
                continue;
            }
            spins = 0U;
            if (max_yields != 0U && yields++ >= max_yields) {
                return false;
            }
            if constexpr (requires { YieldPolicy::yield(); }) {
                YieldPolicy::yield();
            } else {
                __asm__ __volatile__("nop");
            }
        }
        return true;
    }

    template <bool Strict>
    static esp_err_t send_cache(
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
                return Cache::to_aligned(const_cast<void*>(src), bytes);
            } else {
                return Cache::to_device(const_cast<void*>(src), bytes);
            }
        }();
        if (source != ESP_OK) {
            return source;
        }

        const auto target = [&]() noexcept -> esp_err_t {
            if constexpr (Strict) {
                return Cache::discard_aligned(dst, bytes);
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
                .cache_bytes = bytes,
            };
        }

        return sent;
    }

    template <bool Strict, typename Dst, typename Src>
        requires CopySpan<Dst, Src>
    static esp_err_t send_cache_buf(
        CopyTicket<Strict>& ticket,
        CapsBuf<Dst>& dst,
        const CapsBuf<Src>& src) noexcept
    {
        if (!dst || !src || dst.size() < src.size()) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto source = [&]() noexcept -> esp_err_t {
            if constexpr (Strict) {
                return Cache::to_aligned(src);
            } else {
                return Cache::to_device(src);
            }
        }();
        if (source != ESP_OK) {
            return source;
        }

        const auto target = [&]() noexcept -> esp_err_t {
            if constexpr (Strict) {
                return Cache::discard_aligned(dst);
            } else {
                return Cache::discard(dst);
            }
        }();
        if (target != ESP_OK) {
            return target;
        }

        std::uint32_t seq{};
        const auto sent = send_impl<void>(dst.data(), src.data(), src.bytes(), &seq);
        if (sent == ESP_OK) {
            ticket = CopyTicket<Strict>{
                .target = seq,
                .dst = dst.data(),
                .bytes = src.bytes(),
                .cache_bytes = dst.storage_bytes(),
            };
        }

        return sent;
    }

    template <bool Strict>
    static esp_err_t finish_impl(const CopyTicket<Strict>& ticket) noexcept
    {
        if (ticket.dst == nullptr || ticket.bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        wait(ticket.target);
        const auto bytes = ticket.cache_bytes == 0U ? ticket.bytes : ticket.cache_bytes;

        if constexpr (Strict) {
            return Cache::from_aligned(ticket.dst, bytes);
        } else {
            return Cache::from_device(ticket.dst, bytes);
        }
    }

    [[nodiscard]] static esp_err_t install(const async_memcpy_config_t* const config) noexcept
    {
        if constexpr (Backend == CopyBackend::auto_dma) {
            return esp_async_memcpy_install(config, &state.driver);
        } else if constexpr (Backend == CopyBackend::ahb) {
#if SOC_HAS(AHB_GDMA)
            return esp_async_memcpy_install_gdma_ahb(config, &state.driver);
#else
            return ESP_ERR_NOT_SUPPORTED;
#endif
        } else if constexpr (Backend == CopyBackend::axi) {
#if SOC_HAS(AXI_GDMA)
            return esp_async_memcpy_install_gdma_axi(config, &state.driver);
#else
            return ESP_ERR_NOT_SUPPORTED;
#endif
        } else {
#if SOC_CP_DMA_SUPPORTED
            return esp_async_memcpy_install_cpdma(config, &state.driver);
#else
            return ESP_ERR_NOT_SUPPORTED;
#endif
        }
    }
};

}  // namespace arc
