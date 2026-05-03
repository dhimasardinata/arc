#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory_resource>
#include <span>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <utility>
#include <vector>

#include "arc/any.hpp"
#include "arc/bare_core.hpp"
#include "arc/caps.hpp"
#include "arc/claim.hpp"
#include "arc/concepts.hpp"
#include "arc/crypto_dma.hpp"
#include "arc/dsp.hpp"
#include "arc/ecs.hpp"
#include "arc/ethernet.hpp"
#include "arc/fanin.hpp"
#include "arc/file.hpp"
#include "arc/flash_log.hpp"
#include "arc/flash_off.hpp"
#include "arc/foc.hpp"
#include "arc/fsm.hpp"
#include "arc/hil.hpp"
#include "arc/http_server.hpp"
#include "arc/init.hpp"
#include "arc/interrupt_matrix.hpp"
#include "arc/kalman.hpp"
#include "arc/coap.hpp"
#include "arc/log.hpp"
#include "arc/matrix.hpp"
#include "arc/motion.hpp"
#include "arc/mmu_span.hpp"
#include "arc/mqtt.hpp"
#include "arc/mpsc.hpp"
#include "arc/netrpc.hpp"
#include "arc/pack.hpp"
#include "arc/perfetto.hpp"
#include "arc/pms.hpp"
#include "arc/pmr.hpp"
#include "arc/postmortem.hpp"
#include "arc/probe.hpp"
#include "arc/provisioning.hpp"
#include "arc/rcu.hpp"
#include "arc/rpc.hpp"
#include "arc/rtos.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/stream.hpp"
#include "arc/secure_update.hpp"
#include "arc/swarm.hpp"
#include "arc/tdma.hpp"
#include "arc/timesync.hpp"
#include "arc/trace_event.hpp"
#include "arc/trace_stream.hpp"
#include "arc/ulp.hpp"
#include "arc/ws.hpp"
#include "arc/w5500.hpp"

struct ReflectedTelemetry {
    std::uint32_t seq{};
    std::int16_t temp{};
};

ARC_PACK_REFLECT(ReflectedTelemetry, &ReflectedTelemetry::seq, &ReflectedTelemetry::temp);

namespace {

static_assert(sizeof(arc::InitTxn) == sizeof(void*));
static_assert(sizeof(arc::RefInitTxn) == sizeof(void*));
static_assert(sizeof(arc::RefLease) == sizeof(void*));
static_assert(sizeof(arc::TryGate) == sizeof(void*));
static_assert(sizeof(arc::MutexGate) == sizeof(void*));
static_assert(sizeof(arc::TryMutexGate) == sizeof(void*));
static_assert(sizeof(arc::AnyOut) == 6U * sizeof(void*));
static_assert(sizeof(arc::AnyIn) == 3U * sizeof(void*));
static_assert(sizeof(arc::AnyI2c) == 4U * sizeof(void*));
static_assert(sizeof(arc::AnySpi) == 4U * sizeof(void*));
static_assert(sizeof(arc::AnyUart) == 6U * sizeof(void*));
static_assert(arc::rtos::ticks_per_second() == 1000U);
static_assert(arc::rtos::milliseconds(std::chrono::microseconds{1500}) == 2U);
static_assert(arc::rtos::milliseconds(std::chrono::milliseconds{-1}) == 0U);
static_assert(arc::rtos::ticks_ms(2U) == 2U);
static_assert(arc::rtos::ticks(std::chrono::microseconds{1500}) == 2U);
static_assert(sizeof(decltype(arc::claim_token<1, 2, 3>())) == sizeof(std::uint64_t));
static_assert(arc::claim_token<1, 2, 3>() != arc::claim_token<1, 2, 4>());
static_assert(arc::claim_proof<1, 2, 3>() != arc::claim_proof<1, 2, 4>());
static_assert(arc::Result<int>{2}.and_then([](int value) { return arc::Result<int>{value + 1}; }).value() == 3);
static_assert(arc::status_code(arc::ok()) == ESP_OK);

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define ARC_HOST_TSAN 1
#endif
#endif

#if defined(__SANITIZE_THREAD__)
#define ARC_HOST_TSAN 1
#endif

void expect(const bool condition, const char* const message)
{
    if (!condition) {
        std::fprintf(stderr, "host test failed: %s\n", message);
        std::exit(1);
    }
}

std::size_t pms_policy_regions{};
bool flash_policy_active{};
std::size_t w5500_policy_bytes{};
std::size_t bare_core_boots{};
std::size_t crypto_dma_bytes{};
std::size_t interrupt_binds{};

struct MockStaticI2c {
    static inline std::size_t sent{};
    static inline std::size_t received{};
    static inline std::size_t xfer_tx{};
    static inline std::size_t xfer_rx{};
    static inline int timeout{};

    [[nodiscard]] static esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        sent = data == nullptr ? 0U : bytes;
        timeout = timeout_ms;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        received = data == nullptr ? 0U : bytes;
        timeout = timeout_ms;
        std::memset(data, 0x5A, bytes);
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t xfer(
        const void* const tx,
        const std::size_t tx_bytes,
        void* const rx,
        const std::size_t rx_bytes,
        const int timeout_ms) noexcept
    {
        xfer_tx = tx == nullptr ? 0U : tx_bytes;
        xfer_rx = rx == nullptr ? 0U : rx_bytes;
        timeout = timeout_ms;
        const auto* in = static_cast<const std::uint8_t*>(tx);
        auto* out = static_cast<std::uint8_t*>(rx);
        for (std::size_t i = 0; i < tx_bytes && i < rx_bytes; ++i) {
            out[i] = static_cast<std::uint8_t>(in[i] ^ 0xFFU);
        }
        return ESP_OK;
    }
};

struct MockBarePolicy {
    static inline arc::BareCoreConfig config{};
    static inline void (*entry)(void*) noexcept {};
    static inline void* context{};

    [[nodiscard]] static esp_err_t boot(
        const arc::BareCoreConfig& cfg,
        void (*fn)(void*) noexcept,
        void* arg) noexcept
    {
        config = cfg;
        entry = fn;
        context = arg;
        ++bare_core_boots;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t halt() noexcept
    {
        return ESP_OK;
    }
};

struct MockCryptoDmaPolicy {
    static inline arc::CryptoDmaJob last{};
    static inline bool complete{};

    [[nodiscard]] static esp_err_t submit(const arc::CryptoDmaJob& job) noexcept
    {
        last = job;
        crypto_dma_bytes = job.bytes;
        complete = true;
        return ESP_OK;
    }

    [[nodiscard]] static bool done() noexcept
    {
        return complete;
    }
};

struct MockMmuPolicy {
    static inline std::array<std::uint32_t, 4> flash{0x10U, 0x20U, 0x30U, 0x40U};
    static inline bool unmapped{};

    [[nodiscard]] static arc::Result<arc::MmuRegion> map(const arc::MmuMapRequest& request) noexcept
    {
        if (request.bytes > flash.size() * sizeof(std::uint32_t)) {
            return arc::fail(ESP_ERR_INVALID_ARG);
        }
        return arc::ok(arc::MmuRegion{
            .address = flash.data(),
            .bytes = request.bytes,
            .handle = 7U,
        });
    }

    static void unmap(const arc::MmuRegion& region) noexcept
    {
        unmapped = region.handle == 7U;
    }
};

struct MockInterruptPolicy {
    static inline arc::InterruptBinding last{};
    static inline void (*handler)(void*) noexcept {};
    static inline void* arg{};

    [[nodiscard]] static esp_err_t bind(
        const arc::InterruptBinding& binding,
        void (*fn)(void*) noexcept,
        void* context) noexcept
    {
        last = binding;
        handler = fn;
        arg = context;
        ++interrupt_binds;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t unbind(const arc::InterruptBinding&) noexcept
    {
        return ESP_OK;
    }
};

static_assert(arc::I2cDevice<MockStaticI2c>);

struct MockStaticOut {
    static inline bool level{};
    static inline std::uint32_t configured{};
    static inline std::uint32_t writes{};

    static void out() noexcept
    {
        ++configured;
    }

    static void hi() noexcept
    {
        level = true;
        ++writes;
    }

    static void lo() noexcept
    {
        level = false;
        ++writes;
    }

    static void toggle() noexcept
    {
        level = !level;
        ++writes;
    }

    [[nodiscard]] static bool high() noexcept
    {
        return level;
    }
};

static_assert(arc::DigitalOut<MockStaticOut>);

struct MockInput {
    bool level{};
    std::uint32_t configured{};

    arc::Status in() noexcept
    {
        ++configured;
        return arc::ok();
    }

    [[nodiscard]] bool high() const noexcept
    {
        return level;
    }
};

struct MockStaticInput {
    static inline bool level{};

    static esp_err_t in() noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static bool high() noexcept
    {
        return level;
    }
};

static_assert(arc::DigitalIn<MockStaticInput>);

struct MockSpi {
    std::size_t sent{};
    std::size_t received{};
    std::size_t xfer_bytes{};
    std::uint32_t hz{};
    std::uint32_t flags{};

    [[nodiscard]] esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const std::uint32_t speed,
        const std::uint32_t mode_flags) noexcept
    {
        sent = data == nullptr ? 0U : bytes;
        hz = speed;
        flags = mode_flags;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t speed,
        const std::uint32_t mode_flags) noexcept
    {
        received = data == nullptr ? 0U : bytes;
        hz = speed;
        flags = mode_flags;
        std::memset(data, 0xC3, bytes);
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t xfer(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t speed,
        const std::uint32_t mode_flags) noexcept
    {
        xfer_bytes = tx == nullptr || rx == nullptr ? 0U : bytes;
        hz = speed;
        flags = mode_flags;
        std::memcpy(rx, tx, bytes);
        return ESP_OK;
    }
};

struct MockStaticSpi {
    [[nodiscard]] static esp_err_t send(
        const void*,
        std::size_t,
        std::uint32_t,
        std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t recv(
        void*,
        std::size_t,
        std::uint32_t,
        std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t xfer(
        const void*,
        void*,
        std::size_t,
        std::uint32_t,
        std::uint32_t) noexcept
    {
        return ESP_OK;
    }
};

static_assert(arc::SpiDevice<MockStaticSpi>);

struct MockUart {
    std::array<std::uint8_t, 8> rx{0xA0U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U, 0xA6U, 0xA7U};
    std::size_t wrote{};
    std::size_t read_pos{};
    std::uint32_t timeout{};
    bool flushed{};

    [[nodiscard]] arc::Result<std::size_t> write(
        const void* const data,
        const std::size_t bytes) noexcept
    {
        wrote = data == nullptr ? 0U : bytes;
        return wrote;
    }

    [[nodiscard]] arc::Result<std::size_t> read(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms) noexcept
    {
        timeout = timeout_ms;
        auto* out = static_cast<std::uint8_t*>(data);
        const auto available = rx.size() - read_pos;
        const auto count = bytes < available ? bytes : available;
        std::memcpy(out, rx.data() + read_pos, count);
        read_pos += count;
        return count;
    }

    [[nodiscard]] esp_err_t wait(const std::uint32_t timeout_ms) noexcept
    {
        timeout = timeout_ms;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t flush() noexcept
    {
        flushed = true;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t available(std::size_t& bytes) noexcept
    {
        bytes = rx.size() - read_pos;
        return ESP_OK;
    }
};

struct MockStaticUart {
    [[nodiscard]] static arc::Result<std::size_t> write(const void*, const std::size_t bytes) noexcept
    {
        return bytes;
    }

    [[nodiscard]] static arc::Result<std::size_t> read(
        void*,
        const std::size_t bytes,
        std::uint32_t) noexcept
    {
        return bytes;
    }

    [[nodiscard]] static esp_err_t wait(std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t flush() noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t available(std::size_t& bytes) noexcept
    {
        bytes = 0U;
        return ESP_OK;
    }
};

static_assert(arc::UartDevice<MockStaticUart>);

void test_any_io()
{
    const arc::AnyOut empty_out;
    expect(!empty_out, "empty AnyOut is false");
    expect(!empty_out.hi().has_value(), "empty AnyOut rejects hi");
    expect(!empty_out.high().has_value(), "empty AnyOut rejects readback");

    MockStaticOut::level = false;
    MockStaticOut::configured = 0U;
    MockStaticOut::writes = 0U;
    const auto static_out = arc::AnyOut::bind<MockStaticOut>();
    expect(static_cast<bool>(static_out), "static AnyOut binds");
    expect(static_out.out().has_value(), "static AnyOut configures");
    expect(static_out.write(true).has_value(), "static AnyOut writes high");
    expect(static_out.high().has_value() && *static_out.high(), "static AnyOut reads high");
    expect(static_out.toggle().has_value(), "static AnyOut toggles");
    expect(static_out.low().has_value() && *static_out.low(), "static AnyOut reads low");
    expect(MockStaticOut::configured == 1U && MockStaticOut::writes == 2U, "static AnyOut thunks");

    MockInput input{.level = true};
    const auto any_in = arc::AnyIn::bind(input);
    expect(static_cast<bool>(any_in), "object AnyIn binds");
    expect(any_in.in().has_value(), "object AnyIn configures");
    expect(any_in.high().has_value() && *any_in.high(), "object AnyIn reads high");
    input.level = false;
    expect(any_in.low().has_value() && *any_in.low(), "object AnyIn reads low");
    expect(input.configured == 1U, "object AnyIn thunks");

    const arc::AnyI2c empty_i2c;
    expect(!empty_i2c, "empty AnyI2c is false");
    expect(empty_i2c.send(nullptr, 1U) == ESP_ERR_INVALID_ARG, "empty AnyI2c rejects send");

    const auto i2c = arc::AnyI2c::bind<MockStaticI2c>();
    expect(static_cast<bool>(i2c), "static AnyI2c binds");
    const std::array<std::uint8_t, 3> tx{0x10U, 0x20U, 0x30U};
    std::array<std::uint8_t, 3> rx{};
    expect(i2c.send(std::span(tx), 77) == ESP_OK, "AnyI2c span send");
    expect(MockStaticI2c::sent == tx.size() && MockStaticI2c::timeout == 77, "AnyI2c send thunks");
    expect(i2c.recv(std::span(rx), 78) == ESP_OK, "AnyI2c span recv");
    expect(rx[0] == 0x5A && rx[2] == 0x5A && MockStaticI2c::received == rx.size(), "AnyI2c recv thunks");
    expect(i2c.xfer(std::span(tx), std::span(rx), 79) == ESP_OK, "AnyI2c span xfer");
    expect(rx[0] == 0xEFU && rx[2] == 0xCFU, "AnyI2c xfer transforms");
    expect(MockStaticI2c::xfer_tx == tx.size() && MockStaticI2c::xfer_rx == rx.size(), "AnyI2c xfer lengths");

    MockSpi mock;
    const auto spi = arc::AnySpi::bind(mock);
    expect(static_cast<bool>(spi), "object AnySpi binds");
    expect(spi.send(std::span(tx), 1'000'000U, 3U) == ESP_OK, "AnySpi span send");
    expect(mock.sent == tx.size() && mock.hz == 1'000'000U && mock.flags == 3U, "AnySpi send thunks");
    rx = {};
    expect(spi.xfer(std::span(tx), std::span(rx), 2'000'000U, 4U) == ESP_OK, "AnySpi span xfer");
    expect(rx == tx && mock.xfer_bytes == tx.size() && mock.hz == 2'000'000U, "AnySpi xfer copies");
    expect(spi.xfer(std::span(tx), std::span(rx).first(2U)) == ESP_ERR_INVALID_ARG, "AnySpi rejects mismatched spans");

    MockUart uart_mock;
    const auto uart = arc::AnyUart::bind(uart_mock);
    expect(static_cast<bool>(uart), "object AnyUart binds");
    const auto wrote = uart.write(std::span(tx));
    expect(wrote.has_value() && *wrote == tx.size() && uart_mock.wrote == tx.size(), "AnyUart writes");
    std::array<std::uint8_t, 4> serial_rx{};
    const auto got = uart.read(std::span(serial_rx), 42U);
    expect(got.has_value() && *got == serial_rx.size(), "AnyUart reads");
    expect(serial_rx[0] == 0xA0U && serial_rx[3] == 0xA3U && uart_mock.timeout == 42U, "AnyUart read thunks");
    const auto got_timed = uart.read(std::span(serial_rx).first(1U), std::chrono::microseconds{1500});
    expect(got_timed.has_value() && *got_timed == 1U && uart_mock.timeout == 2U, "AnyUart chrono read");
    std::size_t available{};
    expect(uart.available(available) == ESP_OK && available == 3U, "AnyUart available");
    expect(uart.wait(43U) == ESP_OK && uart_mock.timeout == 43U, "AnyUart wait");
    expect(uart.wait(std::chrono::milliseconds{44}) == ESP_OK && uart_mock.timeout == 44U, "AnyUart chrono wait");
    expect(uart.flush() == ESP_OK && uart_mock.flushed, "AnyUart flush");
}

template <typename Fn>
void expect_abort(Fn&& fn, const char* const message)
{
    const auto pid = fork();
    expect(pid >= 0, "fork");

    if (pid == 0) {
        fn();
        _exit(0);
    }

    int status = 0;
    expect(waitpid(pid, &status, 0) == pid, "waitpid");
    expect(WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0), message);
}

void test_spsc()
{
    arc::Spsc<int, 4> queue;
    expect(queue.cap() == 3U, "SPSC usable capacity");
    expect(queue.bytes() == sizeof(queue), "SPSC bytes");
    expect(queue.empty(), "SPSC starts empty");
    expect(queue.try_push(1), "SPSC push 1");
    expect(queue.try_push(2), "SPSC push 2");
    expect(queue.try_push(3), "SPSC push 3");
    expect(!queue.try_push(4), "SPSC full rejects");

    int value{};
    expect(queue.try_pop(value) && value == 1, "SPSC pop 1");
    expect(queue.try_pop(value) && value == 2, "SPSC pop 2");
    expect(queue.try_push(4), "SPSC wrap push");
    expect(queue.try_pop(value) && value == 3, "SPSC pop 3");
    expect(queue.try_pop(value) && value == 4, "SPSC pop 4");
    expect(!queue.try_pop(value), "SPSC empty rejects");

    const std::array more{5, 6, 7, 8};
    expect(queue.push(std::span(more)) == 3U, "SPSC batch push caps at space");
    expect(queue.size() == 3U, "SPSC batch size");
    expect(queue.space() == 0U, "SPSC batch full space");

    std::array<int, 4> out{};
    expect(queue.pop(std::span(out)) == 3U, "SPSC batch pop available");
    expect(out[0] == 5 && out[1] == 6 && out[2] == 7, "SPSC batch order");
    expect(queue.empty(), "SPSC batch drains empty");

    expect(queue.try_push(9), "SPSC wrap seed");
    int wrap{};
    expect(queue.try_pop(wrap) && wrap == 9, "SPSC wrap seed pop");
    expect(queue.push(std::span(more).subspan(1U, 3U)) == 3U, "SPSC batch wrap push");
    out = {};
    expect(queue.pop(std::span(out).first(2U)) == 2U, "SPSC batch partial pop");
    expect(out[0] == 6 && out[1] == 7, "SPSC batch partial order");
    expect(queue.pop(std::span(out).first(2U)) == 1U && out[0] == 8, "SPSC batch remainder");
}

void test_checked_spsc()
{
    arc::Audit<arc::Spsc<int, 4>> queue;
    expect(queue.cap() == 3U, "Audit SPSC usable capacity");
    expect(queue.try_push(1), "Audit SPSC push 1");

    int value{};
    expect(queue.try_pop(value) && value == 1, "Audit SPSC pop 1");

    expect_abort(
        []() {
            arc::Audit<arc::Spsc<int, 4>> lane;
            expect(lane.try_push(1), "Audit SPSC child first push");
            std::thread other([&lane]() {
                static_cast<void>(lane.try_push(2));
            });
            other.join();
        },
        "Audit SPSC rejects a second producer");
}

void test_mpsc_single()
{
    arc::Mpsc<int, 4> queue;
    expect(queue.cap() == 4U, "MPSC capacity");
    expect(queue.cell_align() == arc::cache_line, "MPSC cache-line alignment");
    expect(queue.bytes() == sizeof(queue), "MPSC bytes");
    expect(queue.try_push(7), "MPSC push 7");
    expect(queue.try_push(8), "MPSC push 8");
    expect(queue.try_push(9), "MPSC push 9");
    expect(queue.try_push(10), "MPSC push 10");
    expect(!queue.try_push(11), "MPSC full rejects");

    int value{};
    expect(queue.try_pop(value) && value == 7, "MPSC pop 7");
    expect(queue.try_pop(value) && value == 8, "MPSC pop 8");
    expect(queue.try_push(11), "MPSC wrap push");
    expect(queue.try_pop(value) && value == 9, "MPSC pop 9");
    expect(queue.try_pop(value) && value == 10, "MPSC pop 10");
    expect(queue.try_pop(value) && value == 11, "MPSC pop 11");
    expect(!queue.try_pop(value), "MPSC empty rejects");
}

void test_compact_mpsc()
{
    arc::DenseMpsc<std::uint32_t, 1024> queue;
    expect(queue.cap() == 1024U, "Dense MPSC capacity");
    expect(queue.cell_align() == alignof(std::uint32_t), "Dense MPSC alignment");
    expect(queue.cell_bytes() < arc::Mpsc<std::uint32_t, 1024>::cell_bytes(), "Dense MPSC cell shrinks");
    expect(queue.bytes() < arc::Mpsc<std::uint32_t, 1024>::bytes(), "Dense MPSC bytes shrink");
    expect(queue.try_push(7U), "Dense MPSC push");

    std::uint32_t value{};
    expect(queue.try_pop(value) && value == 7U, "Dense MPSC pop");
}

void test_checked_mpsc()
{
    arc::Audit<arc::Mpsc<int, 4>> queue;
    expect(queue.try_push(1), "Audit MPSC push 1");

    int value{};
    expect(queue.try_pop(value) && value == 1, "Audit MPSC pop 1");

    expect_abort(
        []() {
            arc::Audit<arc::Mpsc<int, 4>> lane;
            expect(lane.try_push(1), "Audit MPSC child push");
            int first{};
            expect(lane.try_pop(first) && first == 1, "Audit MPSC child first pop");
            expect(lane.try_push(2), "Audit MPSC child second push");
            std::thread other([&lane]() {
                int second{};
                static_cast<void>(lane.try_pop(second));
            });
            other.join();
        },
        "Audit MPSC rejects a second consumer");
}

void test_mpsc_threads()
{
    constexpr std::uint32_t producers = 4U;
    constexpr std::uint32_t per_producer = 2048U;
    constexpr std::uint32_t total = producers * per_producer;

    arc::Mpsc<std::uint32_t, 1024> queue;
    std::array<std::uint32_t, producers> counts{};
    std::array<std::thread, producers> threads;
    std::atomic<std::uint32_t> ready{};

    for (std::uint32_t producer = 0; producer < producers; ++producer) {
        threads[producer] = std::thread([producer, &queue, &ready]() {
            ready.fetch_add(1U, std::memory_order_release);
            while (ready.load(std::memory_order_acquire) != producers) {
                std::this_thread::yield();
            }

            for (std::uint32_t i = 0; i < per_producer; ++i) {
                const auto value = (producer << 24U) | i;
                while (!queue.try_push(value)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::uint32_t seen{};
    while (seen < total) {
        std::uint32_t value{};
        if (queue.try_pop(value)) {
            const auto producer = value >> 24U;
            expect(producer < producers, "MPSC producer tag");
            ++counts[producer];
            ++seen;
        } else {
            std::this_thread::yield();
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto count : counts) {
        expect(count == per_producer, "MPSC threaded producer count");
    }
    expect(queue.empty(), "MPSC threaded drain empty");
}

void test_fanin()
{
    arc::Fanin<int, 4, 3> fan;
    expect(fan.producers() == 3U, "Fanin producer count");
    expect(fan.cap() == 3U, "Fanin lane capacity");
    expect(fan.bytes() == sizeof(fan), "Fanin bytes");
    expect(fan.try_push<0>(10), "Fanin push lane 0");
    expect(fan.try_push<1>(20), "Fanin push lane 1");
    expect(fan.try_push<2>(30), "Fanin push lane 2");
    expect(fan.try_push<0>(11), "Fanin second lane 0");

    std::size_t producer{};
    int value{};
    expect(fan.try_pop(producer, value) && producer == 0U && value == 10, "Fanin pop lane 0");
    expect(fan.try_pop(producer, value) && producer == 1U && value == 20, "Fanin pop lane 1");
    expect(fan.try_pop(producer, value) && producer == 2U && value == 30, "Fanin pop lane 2");
    expect(fan.try_pop(producer, value) && producer == 0U && value == 11, "Fanin round-robin resume");
    expect(!fan.try_pop(producer, value), "Fanin empty rejects");

    expect(fan.try_push<0>(1), "Fanin batch lane 0 first");
    expect(fan.try_push<0>(2), "Fanin batch lane 0 second");
    expect(fan.try_push<1>(10), "Fanin batch lane 1 first");
    expect(fan.try_push<2>(20), "Fanin batch lane 2 first");
    expect(fan.try_push<2>(21), "Fanin batch lane 2 second");
    std::array<int, 6> out{};
    expect(fan.pop(std::span(out)) == 5U, "Fanin batch pop count");
    expect(out[0] == 10 && out[1] == 20 && out[2] == 21 && out[3] == 1 && out[4] == 2, "Fanin batch lane order");
    expect(fan.empty(), "Fanin batch drains empty");

    const std::array lane{100, 101, 102, 103};
    expect(fan.push<1>(std::span(lane)) == 3U, "Fanin batch push caps lane");
    expect(fan.size<1>() == 3U && fan.space<1>() == 0U, "Fanin lane size and space");
    out = {};
    expect(fan.pop(std::span(out)) == 3U, "Fanin batch pushed pop count");
    expect(out[0] == 100 && out[1] == 101 && out[2] == 102, "Fanin batch pushed order");
}

void test_rpc_lane()
{
    enum class Op : std::uint8_t { set = 1U };
    struct Request {
        std::uint32_t value{};
    };
    struct Reply {
        std::uint32_t applied{};
    };

    arc::RpcLane<Op, Request, Reply, 4> rpc;
    expect(rpc.call(Op::set, Request{.value = 42U}, 7U), "RPC call queues request");

    decltype(rpc)::Request request{};
    expect(rpc.recv(request), "RPC recv request");
    expect(request.serial == 7U && request.op == Op::set && request.payload.value == 42U, "RPC request fields");
    expect(rpc.reply(request.serial, ESP_OK, Reply{.applied = request.payload.value + 1U}), "RPC queues reply");

    decltype(rpc)::Reply reply{};
    expect(rpc.poll_match(7U, reply), "RPC match reply");
    expect(reply.status == ESP_OK && reply.payload.applied == 43U, "RPC reply fields");

    expect(rpc.call(Op::set, Request{.value = 1U}, 8U), "RPC second call");
    expect(rpc.recv(request), "RPC second recv");
    expect(rpc.reply(9U, ESP_OK, Reply{.applied = 9U}), "RPC out-of-order reply");
    expect(!rpc.poll_match(8U, reply), "RPC defers unmatched reply");
    expect(rpc.poll_deferred(reply) && reply.serial == 9U, "RPC deferred reply preserved");
}

void test_checked_fanin()
{
    arc::Audit<arc::Fanin<int, 4, 2>> fan;
    expect(fan.producers() == 2U, "Audit Fanin producer count");
    expect(fan.try_push<0>(10), "Audit Fanin push lane 0");
    expect(fan.try_push<1>(20), "Audit Fanin push lane 1");

    std::size_t producer{};
    int value{};
    expect(fan.try_pop(producer, value) && producer == 0U && value == 10, "Audit Fanin pop lane 0");
    expect(fan.try_pop(producer, value) && producer == 1U && value == 20, "Audit Fanin pop lane 1");

    expect(fan.try_push<0>(30), "Audit Fanin batch lane 0");
    expect(fan.try_push<1>(40), "Audit Fanin batch lane 1");
    std::array<int, 2> out{};
    expect(fan.pop(std::span(out)) == 2U && out[0] == 30 && out[1] == 40, "Audit Fanin batch pop");

    const std::array lane{50, 51, 52};
    expect(fan.push<0>(std::span(lane)) == 3U, "Audit Fanin batch push");
    expect(fan.size<0>() == 3U && fan.space<0>() == 0U, "Audit Fanin lane size and space");
    expect(fan.pop(std::span(out).first(2U)) == 2U && out[0] == 50 && out[1] == 51, "Audit Fanin partial batch pop");
}

void test_mqtt()
{
    std::array<std::uint8_t, 256> buffer{};

    const auto connect = arc::net::Mqtt::connect(
        buffer,
        {
            .client = "arc-host",
            .user = "arc",
            .pass = "secret",
            .keep_alive = 30U,
            .clean = true,
        });
    expect(connect.has_value(), "MQTT connect encodes");
    expect((*connect)[0] == 0x10U, "MQTT connect type");
    expect((*connect)[2] == 0x00U && (*connect)[3] == 0x04U, "MQTT protocol length");
    expect((*connect)[4] == 'M' && (*connect)[7] == 'T', "MQTT protocol name");

    const std::array<std::uint8_t, 5> payload{1U, 2U, 3U, 4U, 5U};
    const auto publish = arc::net::Mqtt::publish(
        buffer,
        "arc/topic",
        std::span(payload),
        7U,
        arc::net::MqttQos::at_least_once,
        false,
        true);
    expect(publish.has_value(), "MQTT publish encodes");

    const auto packet = arc::net::Mqtt::parse(*publish);
    expect(packet.has_value(), "MQTT publish parses");
    expect(packet->type == arc::net::MqttType::publish, "MQTT publish type");
    expect(packet->retain(), "MQTT publish retain");
    expect(packet->qos() == arc::net::MqttQos::at_least_once, "MQTT publish qos");

    const auto view = arc::net::Mqtt::view(*packet);
    expect(view.has_value(), "MQTT publish view");
    expect(view->packet == 7U, "MQTT publish packet id");
    expect(view->topic.size() == 9U, "MQTT publish topic bytes");
    expect(std::memcmp(view->topic.data(), "arc/topic", view->topic.size()) == 0, "MQTT publish topic");
    expect(view->payload.size() == payload.size(), "MQTT publish payload size");
    expect(std::memcmp(view->payload.data(), payload.data(), payload.size()) == 0, "MQTT publish payload");

    const std::array subscriptions{
        arc::net::MqttSubscription{.filter = "arc/+/status", .qos = arc::net::MqttQos::at_least_once},
        arc::net::MqttSubscription{.filter = "arc/#", .qos = arc::net::MqttQos::at_most_once},
    };
    const auto subscribe = arc::net::Mqtt::subscribe(buffer, 9U, std::span(subscriptions));
    expect(subscribe.has_value(), "MQTT subscribe encodes");
    expect((*subscribe)[0] == 0x82U, "MQTT subscribe flags");
    expect((*subscribe)[2] == 0x00U && (*subscribe)[3] == 0x09U, "MQTT subscribe packet id");

    const std::array<std::uint8_t, 4> connack_raw{0x20U, 0x02U, 0x01U, 0x00U};
    const auto connack_packet = arc::net::Mqtt::parse(connack_raw);
    expect(connack_packet.has_value(), "MQTT connack parses");
    const auto connack = arc::net::Mqtt::connack(*connack_packet);
    expect(connack.has_value(), "MQTT connack view");
    expect(connack->session, "MQTT connack session");
    expect(connack->code == 0U, "MQTT connack code");

    const std::array<std::uint8_t, 5> suback_raw{0x90U, 0x03U, 0x00U, 0x09U, 0x01U};
    const auto suback_packet = arc::net::Mqtt::parse(suback_raw);
    expect(suback_packet.has_value(), "MQTT suback parses");
    const auto suback = arc::net::Mqtt::suback(*suback_packet);
    expect(suback.has_value(), "MQTT suback view");
    expect(suback->packet == 9U, "MQTT suback packet id");
    expect(suback->codes.size() == 1U && suback->codes[0] == 0x01U, "MQTT suback code");

    arc::net::MqttSession session;
    session.connect_started(1000U, 2U);
    expect(session.state() == arc::net::MqttSessionState::connecting, "MQTT session connecting");
    expect(session.observe(*connack_packet, 1010U) == ESP_OK, "MQTT session connack");
    expect(session.online(), "MQTT session online");
    expect(session.next_packet() == 1U && session.next_packet() == 2U, "MQTT session packet ids");
    expect(!session.ping_due(2500U), "MQTT session ping not due");
    expect(session.ping_due(3010U), "MQTT session ping due");
    const auto ping = session.ping(buffer, 3010U);
    expect(ping.has_value() && (*ping)[0] == 0xC0U, "MQTT session ping frame");
    expect(session.awaiting_ping(), "MQTT session awaits pingresp");
    const std::array<std::uint8_t, 2> pingresp_raw{0xD0U, 0x00U};
    const auto pingresp = arc::net::Mqtt::parse(pingresp_raw);
    expect(pingresp.has_value(), "MQTT pingresp parses");
    expect(session.observe(*pingresp, 3050U) == ESP_OK, "MQTT session pingresp");
    expect(!session.awaiting_ping(), "MQTT session pingresp clears");
    expect(!session.expired(6000U), "MQTT session not expired");
    expect(session.expired(6060U), "MQTT session expired");

    expect(arc::net::Mqtt::match("arc/+/status", "arc/node/status"), "MQTT single wildcard");
    expect(arc::net::Mqtt::match("arc/#", "arc/node/status"), "MQTT multi wildcard");
    expect(arc::net::Mqtt::match("arc/#", "arc"), "MQTT parent wildcard root");
    expect(!arc::net::Mqtt::match("arc#", "arc"), "MQTT invalid hash placement");
    expect(!arc::net::Mqtt::match("arc+status", "arc/status"), "MQTT invalid plus placement");
    expect(!arc::net::Mqtt::match("arc/+/status", "arc/node/state"), "MQTT mismatch");
    expect(!arc::net::Mqtt::match("arc/#", "$SYS/broker"), "MQTT system topic isolation");
}

void test_ws()
{
    std::array<char, 32> accept_buf{};
    const auto accept = arc::net::Ws::accept(accept_buf, "dGhlIHNhbXBsZSBub25jZQ==");
    expect(accept.has_value(), "WS accept encodes");
    expect(
        std::memcmp(accept->data(), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", accept->size()) == 0,
        "WS accept value");

    const std::array<std::uint8_t, 16> nonce{
        0x01U,
        0x23U,
        0x45U,
        0x67U,
        0x89U,
        0xabU,
        0xcdU,
        0xefU,
        0x10U,
        0x32U,
        0x54U,
        0x76U,
        0x98U,
        0xbaU,
        0xdcU,
        0xfeU,
    };
    std::array<char, 32> key_buf{};
    const auto key = arc::net::Ws::key(key_buf, nonce);
    expect(key.has_value() && key->size() == 24U, "WS key bytes");

    std::array<std::uint8_t, 128> frame{};
    const auto text = arc::net::Ws::text(frame, "arc", true, 0x11223344U);
    expect(text.has_value(), "WS text encodes");

    std::array<std::uint8_t, 32> scratch{};
    const auto parsed = arc::net::Ws::parse(*text, scratch);
    expect(parsed.has_value(), "WS text parses");
    expect(parsed->opcode == arc::net::WsOpcode::text, "WS text opcode");
    expect(parsed->masked, "WS text masked");
    expect(parsed->fin, "WS text fin");
    expect(parsed->payload.size() == 3U, "WS text payload size");
    expect(std::memcmp(parsed->payload.data(), "arc", parsed->payload.size()) == 0, "WS text payload");

    const std::array<std::uint8_t, 3> ping_data{1U, 2U, 3U};
    const auto ping = arc::net::Ws::ping(frame, std::span(ping_data));
    expect(ping.has_value(), "WS ping encodes");
    const auto ping_frame = arc::net::Ws::parse(*ping);
    expect(ping_frame.has_value(), "WS ping parses");
    expect(ping_frame->opcode == arc::net::WsOpcode::ping, "WS ping opcode");

    const std::array<std::uint8_t, 3> reason{'b', 'y', 'e'};
    const auto close = arc::net::Ws::close(frame, 1000U, std::span(reason));
    expect(close.has_value(), "WS close encodes");
    const auto close_frame = arc::net::Ws::parse(*close);
    expect(close_frame.has_value(), "WS close parses");
    const auto close_view = arc::net::Ws::close_view(*close_frame);
    expect(close_view.has_value(), "WS close view");
    expect(close_view->code == 1000U, "WS close code");
    expect(close_view->reason.size() == reason.size(), "WS close reason size");
    expect(std::memcmp(close_view->reason.data(), reason.data(), reason.size()) == 0, "WS close reason");
}

void test_coap()
{
    std::array<std::uint8_t, 256> buffer{};
    const std::array<std::uint8_t, 2> token{0xdeU, 0xadU};
    const std::array options{
        arc::net::Coap::text(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
            "sensors"),
        arc::net::Coap::text(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
            "temp"),
        arc::net::Coap::text(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_query),
            "units=c"),
    };

    const auto request = arc::net::Coap::message(
        buffer,
        arc::net::CoapType::confirmable,
        static_cast<std::uint8_t>(arc::net::CoapCode::get),
        0x1234U,
        std::span(token),
        std::span(options));
    expect(request.has_value(), "CoAP request encodes");

    const auto parsed = arc::net::Coap::parse(*request);
    expect(parsed.has_value(), "CoAP request parses");
    expect(parsed->type == arc::net::CoapType::confirmable, "CoAP request type");
    expect(parsed->code == static_cast<std::uint8_t>(arc::net::CoapCode::get), "CoAP request code");
    expect(parsed->id == 0x1234U, "CoAP request id");
    expect(parsed->token.size() == token.size(), "CoAP token size");
    expect(std::memcmp(parsed->token.data(), token.data(), token.size()) == 0, "CoAP token");

    std::size_t offset = 0U;
    std::uint16_t number = 0U;
    const auto first = arc::net::Coap::next(parsed->options, offset, number);
    expect(first.has_value(), "CoAP option 1");
    expect(first->number == static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path), "CoAP option 1 number");
    expect(std::memcmp(first->value.data(), "sensors", first->value.size()) == 0, "CoAP option 1 value");

    const auto second = arc::net::Coap::next(parsed->options, offset, number);
    expect(second.has_value(), "CoAP option 2");
    expect(second->number == static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path), "CoAP option 2 number");
    expect(std::memcmp(second->value.data(), "temp", second->value.size()) == 0, "CoAP option 2 value");

    const auto third = arc::net::Coap::next(parsed->options, offset, number);
    expect(third.has_value(), "CoAP option 3");
    expect(third->number == static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_query), "CoAP option 3 number");
    expect(std::memcmp(third->value.data(), "units=c", third->value.size()) == 0, "CoAP option 3 value");

    const auto none = arc::net::Coap::next(parsed->options, offset, number);
    expect(!none.has_value() && none.error() == ESP_ERR_NOT_FOUND, "CoAP option end");

    const std::array<std::uint8_t, 1> format{50U};
    const std::array response_options{
        arc::net::Coap::option(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::content_format),
            std::span(format)),
    };
    const std::array<std::uint8_t, 4> body{'4', '2', '.', '0'};
    const auto response = arc::net::Coap::message(
        buffer,
        arc::net::CoapType::acknowledgement,
        static_cast<std::uint8_t>(arc::net::CoapCode::content),
        0x1234U,
        std::span(token),
        std::span(response_options),
        std::span(body));
    expect(response.has_value(), "CoAP response encodes");

    const auto parsed_response = arc::net::Coap::parse(*response);
    expect(parsed_response.has_value(), "CoAP response parses");
    expect(parsed_response->type == arc::net::CoapType::acknowledgement, "CoAP response type");
    expect(parsed_response->code == static_cast<std::uint8_t>(arc::net::CoapCode::content), "CoAP response code");
    expect(parsed_response->payload.size() == body.size(), "CoAP payload size");
    expect(std::memcmp(parsed_response->payload.data(), body.data(), body.size()) == 0, "CoAP payload");

    const auto ping = arc::net::Coap::ping(buffer, 0x40U);
    expect(ping.has_value(), "CoAP ping encodes");
    const auto ping_msg = arc::net::Coap::parse(*ping);
    expect(ping_msg.has_value(), "CoAP ping parses");
    expect(ping_msg->type == arc::net::CoapType::confirmable, "CoAP ping type");
    expect(ping_msg->code == static_cast<std::uint8_t>(arc::net::CoapCode::empty), "CoAP ping code");

    const auto reset = arc::net::Coap::reset(buffer, 0x40U);
    expect(reset.has_value(), "CoAP reset encodes");

    const std::array<std::uint8_t, 4> overflow_option{0xe0U, 0xffU, 0xf2U, 0x00U};
    std::size_t overflow_offset{};
    std::uint16_t overflow_number = 65530U;
    const auto overflow = arc::net::Coap::next(overflow_option, overflow_offset, overflow_number);
    expect(!overflow.has_value(), "CoAP option number overflow rejects");
}

void test_invalid_codecs()
{
    const std::array<std::uint8_t, 2> reserved_ws_opcode{0x83U, 0x00U};
    expect(!arc::net::Ws::parse(reserved_ws_opcode), "WS reserved opcode rejects");

    std::array<std::uint8_t, 8> ws_buffer{};
    expect(!arc::net::Ws::frame(ws_buffer, static_cast<arc::net::WsOpcode>(0x3U), nullptr, 0U),
           "WS reserved opcode encode rejects");

    const std::array<std::uint8_t, 7> mqtt_reserved_qos{0x36U, 0x05U, 0x00U, 0x01U, 't', 0x00U, 0x01U};
    expect(!arc::net::Mqtt::parse(mqtt_reserved_qos), "MQTT reserved publish QoS rejects");

    const std::array<std::uint8_t, 2> mqtt_reserved_type{0xf0U, 0x00U};
    expect(!arc::net::Mqtt::parse(mqtt_reserved_type), "MQTT reserved type rejects");

    const std::array<std::uint8_t, 2> mqtt_bad_subscribe_flags{0x80U, 0x00U};
    expect(!arc::net::Mqtt::parse(mqtt_bad_subscribe_flags), "MQTT fixed flags reject");
}

void test_http_server()
{
    using ConfigGet = arc::net::HttpRoute<arc::net::HttpMethod::get, "/config">;
    using MetricsGet = arc::net::HttpRoute<arc::net::HttpMethod::get, "/metrics">;
    static_assert(ConfigGet::id() != MetricsGet::id());

    constexpr char raw[] =
        "POST /config HTTP/1.1\r\n"
        "Host: arc.local\r\n"
        "Content-Length: 7\r\n"
        "X-Mode: tight \r\n"
        "\r\n"
        "enabledextra";
    std::array<arc::net::HttpHeaderView, 4> headers{};
    const auto req = arc::net::HttpServer::parse(
        std::span<const char>{raw, sizeof(raw) - 1U},
        std::span(headers));
    expect(req.has_value(), "HTTP server request parses");
    expect(req->method == arc::net::HttpMethod::post, "HTTP method parses");
    expect(std::memcmp(req->target.data(), "/config", req->target.size()) == 0, "HTTP target parses");
    expect(req->headers.size() == 3U, "HTTP header count");
    expect(req->body.size() == 7U, "HTTP body trims to content length");
    expect(std::memcmp(req->body.data(), "enabled", req->body.size()) == 0, "HTTP body view");

    const auto* const host = arc::net::HttpServer::find_header(*req, "host");
    expect(host != nullptr && std::memcmp(host->value.data(), "arc.local", host->value.size()) == 0, "HTTP host header");

    const auto* const mode = arc::net::HttpServer::find_header(*req, "x-mode");
    expect(mode != nullptr && mode->value.size() == 5U, "HTTP header trim");

    bool routed{};
    const auto post_hit = arc::net::HttpRouter<ConfigGet, MetricsGet>::dispatch(*req, [&](auto route) {
        routed = route.id() == ConfigGet::id();
    });
    expect(!post_hit && !routed, "HTTP router rejects method mismatch");

    constexpr char get_raw[] = "GET /config HTTP/1.1\r\nHost: arc.local\r\n\r\n";
    const auto get_req = arc::net::HttpServer::parse(
        std::span<const char>{get_raw, sizeof(get_raw) - 1U},
        std::span(headers));
    expect(get_req.has_value(), "HTTP GET parses");
    const auto get_hit = arc::net::HttpRouter<ConfigGet, MetricsGet>::dispatch(*get_req, [&](auto route) {
        routed = route.id() == ConfigGet::id();
    });
    expect(get_hit && routed, "HTTP router matches compile-time route");

    std::array<char, 128> response{};
    constexpr char body[] = "ok";
    const auto encoded = arc::net::HttpServer::text_response(
        std::span(response),
        200,
        "OK",
        std::span<const char>{body, sizeof(body) - 1U});
    expect(encoded.has_value(), "HTTP response encodes");
    expect(std::memcmp(encoded->data(), "HTTP/1.1 200 OK\r\n", 17U) == 0, "HTTP response status");

    std::array<arc::net::HttpHeaderView, 1> too_few{};
    expect(
        !arc::net::HttpServer::parse(std::span<const char>{raw, sizeof(raw) - 1U}, std::span(too_few)),
        "HTTP header capacity rejects");

    constexpr char short_body[] = "POST /config HTTP/1.1\r\nContent-Length: 8\r\n\r\nenabled";
    expect(
        !arc::net::HttpServer::parse(std::span<const char>{short_body, sizeof(short_body) - 1U}, std::span(headers)),
        "HTTP short body rejects");

    constexpr char bad_header[] = "GET /config HTTP/1.1\r\nBroken\r\n\r\n";
    expect(
        !arc::net::HttpServer::parse(std::span<const char>{bad_header, sizeof(bad_header) - 1U}, std::span(headers)),
        "HTTP malformed header rejects");

    std::array<char, 8> small_response{};
    expect(
        !arc::net::HttpServer::text_response(
            std::span(small_response),
            200,
            "OK",
            std::span<const char>{body, sizeof(body) - 1U}),
        "HTTP response buffer cap rejects");
}

void test_caps()
{
    struct alignas(64) OverAligned {
        std::uint32_t value{};
    };

    auto ptr = arc::caps<OverAligned, MALLOC_CAP_INTERNAL>();
    expect(static_cast<bool>(ptr), "caps over-aligned allocation succeeds");
    expect((reinterpret_cast<std::uintptr_t>(ptr.get()) & (alignof(OverAligned) - 1U)) == 0U,
           "caps allocation preserves over-alignment");

    heap_caps_last_calloc_bytes = 0U;
    auto plain = arc::inbuf<std::uint8_t>(65U);
    expect(static_cast<bool>(plain), "plain cap buffer allocation succeeds");
    expect(plain.size() == 65U && plain.bytes() == 65U, "plain cap buffer preserves logical size");
    expect(heap_caps_last_calloc_bytes == 65U, "plain cap buffer allocation stays logical");

    heap_caps_last_calloc_bytes = 0U;
    auto dma = arc::dmabuf<std::uint8_t>(65U);
    const auto padded = (65U + arc::cache_line - 1U) & ~(arc::cache_line - 1U);
    expect(static_cast<bool>(dma), "DMA cap buffer allocation succeeds");
    expect(dma.size() == 65U && dma.bytes() == 65U, "DMA cap buffer preserves logical size");
    expect((reinterpret_cast<std::uintptr_t>(dma.data()) & (arc::cache_line - 1U)) == 0U,
           "DMA cap buffer preserves cache-line alignment");
    expect(heap_caps_last_calloc_bytes == padded, "DMA cap buffer allocates whole cache lines");

    arc::PmrCapsResource<MALLOC_CAP_SPIRAM> psram{};
    std::pmr::vector<std::uint8_t> vec{&psram};
    vec.resize(4U);
    vec[3] = 9U;
    expect(vec.size() == 4U && vec[3] == 9U, "PMR caps vector allocates");
}

void test_dsp()
{
    const std::array<int, 4> lhs{1, 2, 3, 4};
    const std::array<int, 4> rhs{5, 6, 7, 8};
    expect(arc::dsp::dot(lhs.data(), rhs.data(), lhs.size()) == 70, "DSP dot");

    std::array<int, 4> out{};
    arc::dsp::scale(out.data(), lhs.data(), 3, lhs.size());
    expect((out == std::array<int, 4>{3, 6, 9, 12}), "DSP scale");
    arc::dsp::mix(out.data(), lhs.data(), rhs.data(), lhs.size());
    expect((out == std::array<int, 4>{6, 8, 10, 12}), "DSP mix");
    arc::dsp::mac(out.data(), lhs.data(), 2, lhs.size());
    expect((out == std::array<int, 4>{8, 12, 16, 20}), "DSP mac");

    const std::array<int, 5> wave{-2, 4, -7, 1, 3};
    expect(arc::dsp::peak(wave.data(), wave.size()) == 7, "DSP peak");

    using Filter = arc::dsp::Fir<int, 3>;
    Filter::State state{};
    const Filter::Coeffs taps{1, 2, 3};
    const std::array<int, 4> input{1, 2, 3, 4};
    std::array<int, 4> filtered{};
    Filter::run(filtered.data(), state, taps, input.data(), input.size());
    expect((filtered == std::array<int, 4>{1, 4, 10, 16}), "DSP FIR");
    Filter::clear(state);
    expect(Filter::step(state, taps, 5) == 5, "DSP FIR clear");

    using Pid = arc::dsp::Pid<float>;
    Pid::State pid{};
    const auto effort = Pid::step(
        pid,
        {.kp = 2.0F, .ki = 1.0F, .kd = 0.5F},
        {.out_min = -10.0F, .out_max = 10.0F, .i_min = -2.0F, .i_max = 2.0F},
        3.0F,
        1.0F,
        0.5F);
    expect(effort == 5.0F, "DSP PID step");
    expect(pid.integral == 1.0F, "DSP PID integral");

    using Bq = arc::dsp::Biquad<int>;
    Bq::State bq{};
    const Bq::Coeffs pass{.b0 = 1, .b1 = 0, .b2 = 0, .a1 = 0, .a2 = 0};
    expect(Bq::step(bq, pass, 7) == 7, "DSP Biquad passthrough");
    Bq::clear(bq);
    const Bq::Coeffs fir2{.b0 = 1, .b1 = 1, .b2 = 0, .a1 = 0, .a2 = 0};
    expect(Bq::step(bq, fir2, 2) == 2, "DSP Biquad first sample");
    expect(Bq::step(bq, fir2, 3) == 5, "DSP Biquad history");

    using Cascade = arc::dsp::Sos<int, 2>;
    Cascade::State cascade{};
    const Cascade::Coeffs sections{
        Bq::Coeffs{.b0 = 1, .b1 = 1, .b2 = 0, .a1 = 0, .a2 = 0},
        Bq::Coeffs{.b0 = 2, .b1 = 0, .b2 = 0, .a1 = 0, .a2 = 0},
    };
    expect(Cascade::step(cascade, sections, 4) == 8, "DSP SOS first sample");
    expect(Cascade::step(cascade, sections, 5) == 18, "DSP SOS cascade history");
    Cascade::clear(cascade);
    expect(Cascade::step(cascade, sections, 1) == 2, "DSP SOS clear");

    using Plant = arc::dsp::StateSpace<int, 2, 1, 1>;
    Plant::State plant{};
    const Plant::Model model{
        .a = {{{1, 1}, {0, 1}}},
        .b = {{{1}, {1}}},
        .c = {{{1, 0}}},
        .d = {{{0}}},
    };
    expect(Plant::step(plant, model, Plant::InputVec{2})[0] == 0, "DSP StateSpace first output");
    expect((plant.x == Plant::StateVec{2, 2}), "DSP StateSpace state update");
    expect(Plant::step(plant, model, Plant::InputVec{3})[0] == 2, "DSP StateSpace second output");
    expect((plant.x == Plant::StateVec{7, 5}), "DSP StateSpace second update");

    const auto clarke = arc::dsp::clarke(arc::dsp::Phase3<float>{.a = 1.0F, .b = -0.5F, .c = -0.5F});
    expect(clarke.alpha == 1.0F && clarke.beta == 0.0F, "DSP Clarke transform");
    const auto park = arc::dsp::park(clarke, 0.0F, 1.0F);
    expect(park.d == 1.0F && park.q == 0.0F, "DSP Park transform");
    const auto phase = arc::dsp::inverse_clarke(arc::dsp::AlphaBeta<float>{.alpha = 1.0F, .beta = 0.0F});
    expect(phase.a == 1.0F && phase.b == -0.5F && phase.c == -0.5F, "DSP inverse Clarke");

    const std::array accel_lhs{1.0F, 2.0F};
    const std::array accel_rhs{3.0F, 4.0F};
    const auto accel = arc::dsp::DspAccel<>::dot_f32(std::span<const float>{accel_lhs}, std::span<const float>{accel_rhs});
    expect(accel == 11.0F, "DSP accel dot fallback");
}

void test_foc_motion_tdma()
{
    struct Bridge {};
    struct Scope {};
    struct Encoder {};

    arc::Foc<Bridge, Scope, Encoder, float> foc{};
    foc.config = {
        .d_gains = {.kp = 1.0F, .ki = 0.0F, .kd = 0.0F},
        .q_gains = {.kp = 2.0F, .ki = 0.0F, .kd = 0.0F},
        .limits = {.out_min = -1.0F, .out_max = 1.0F, .i_min = -1.0F, .i_max = 1.0F},
    };
    foc.target.write({.d = 0.0F, .q = 0.5F, .bus = 2.0F});
    const auto out = foc.step(
        {.current = {.a = 0.0F, .b = 0.0F, .c = 0.0F}, .sin_theta = 0.0F, .cos_theta = 1.0F},
        0.00005F);
    expect(out.current.d == 0.0F && out.current.q == 0.0F, "FOC current transform");
    expect(out.duty.a == 0.5F && out.duty.b > 0.7F && out.duty.c < 0.3F, "FOC centered duty");

    std::array<arc::MotionStep<3>, 8> steps{};
    const auto plan = arc::MotionPlan<3>::line(
        std::span(steps),
        {.delta = {4, 2, 1}, .ticks_per_step = 10U});
    expect(plan.has_value() && plan->size() == 4U, "Motion line size");
    expect(steps[0].mask == 0x1U && steps[1].mask == 0x3U && steps[3].mask == 0x7U, "Motion Bresenham masks");

    const auto active = arc::net::Tdma::window(1'225U, {.period_us = 4'000U, .slot_us = 1'000U, .guard_us = 50U, .slot = 1U});
    expect(active.active && active.start_us == 1050U && active.end_us == 1950U, "TDMA active window");
    const auto idle = arc::net::Tdma::window(2'225U, {.period_us = 4'000U, .slot_us = 1'000U, .guard_us = 50U, .slot = 1U});
    expect(!idle.active, "TDMA inactive outside slot");

    const auto s = arc::SCurve<float>::sample(100.0F, 2.0F, 1.0F);
    expect(s.position == 50.0F && s.velocity > 90.0F, "S-curve midpoint");

    arc::dsp::PllObserver<float> pll{};
    pll.step(1.0F, 10.0F, 1.0F, 0.01F);
    expect(pll.angle > 0.0F && pll.velocity > 0.0F, "FOC PLL observer");

    arc::dsp::SlidingModeObserver<float> smo{};
    smo.step({.alpha = 1.0F, .beta = -1.0F}, {.alpha = 0.5F, .beta = -0.5F}, 2.0F, 0.1F);
    expect(smo.estimated_alpha > 0.0F && smo.estimated_beta < 0.0F, "FOC sliding observer");
}

void test_netrpc_pms_secure_update()
{
    enum class Op : std::uint8_t { set = 2U };
    struct WireCall {
        std::uint32_t serial{};
        Op op{};
        std::int16_t value{};
    };
    using Wire = arc::pack::StructOf<WireCall, &WireCall::serial, &WireCall::op, &WireCall::value>;

    std::array<std::uint8_t, Wire::bytes> frame{};
    const auto encoded = arc::net::NetRpc<Wire>::encode(frame, WireCall{.serial = 9U, .op = Op::set, .value = -3});
    expect(encoded.has_value(), "NetRPC encode");
    WireCall decoded{};
    expect(arc::net::NetRpc<Wire>::decode(*encoded, decoded).has_value(), "NetRPC decode");
    expect(decoded.serial == 9U && decoded.op == Op::set && decoded.value == -3, "NetRPC roundtrip");

    struct PmsPolicy {
        static esp_err_t lock(const std::span<const arc::PmsRegion> in) noexcept
        {
            pms_policy_regions = in.size();
            return ESP_OK;
        }
        static esp_err_t unlock() noexcept
        {
            pms_policy_regions = 0U;
            return ESP_OK;
        }
    };

    const std::array pms{
        arc::PmsRegion{.base = frame.data(), .bytes = frame.size(), .core0 = arc::PmsAccess::read_write, .core1 = arc::PmsAccess::read},
    };
    expect(arc::Pms<PmsPolicy>::lock(std::span(pms)) == ESP_OK && pms_policy_regions == 1U, "PMS policy lock");
    expect(arc::Pms<PmsPolicy>::unlock() == ESP_OK && pms_policy_regions == 0U, "PMS policy unlock");

    struct Crypto {
        bool verified{};
        esp_err_t open(
            std::span<const std::uint8_t> in,
            std::span<std::uint8_t> out,
            std::span<const std::uint8_t>,
            std::span<const std::uint8_t>,
            std::span<const std::uint8_t>) noexcept
        {
            std::memcpy(out.data(), in.data(), in.size());
            return ESP_OK;
        }
        esp_err_t verify(std::span<const std::uint8_t>) noexcept
        {
            verified = true;
            return ESP_OK;
        }
    };
    struct Writer {
        std::size_t bytes{};
        bool done{};
        esp_err_t begin(std::size_t) noexcept
        {
            return ESP_OK;
        }
        esp_err_t write(const void*, std::size_t size) noexcept
        {
            bytes += size;
            return ESP_OK;
        }
        esp_err_t finish() noexcept
        {
            done = true;
            return ESP_OK;
        }
    };

    arc::SecureUpdate<Crypto, Writer> update{};
    std::array<std::uint8_t, 4> cipher{1, 2, 3, 4};
    std::array<std::uint8_t, 4> plain{};
    expect(update.begin(4U) == ESP_OK, "SecureUpdate begin");
    expect(update.write({.ciphertext = cipher, .nonce = cipher, .aad = {}, .tag = cipher}, plain) == ESP_OK,
           "SecureUpdate write");
    expect(update.finish(cipher) == ESP_OK && update.crypto.verified && update.writer.done, "SecureUpdate finish");
    expect(update.writer.bytes == 4U && plain[3] == 4U, "SecureUpdate plaintext write");
}

void test_matrix_kalman_storage_swarm()
{
    arc::dsp::Matrix<int, 2, 2> a{{1, 2, 3, 4}};
    arc::dsp::Matrix<int, 2, 1> x{{5, 6}};
    const auto y = arc::dsp::mul_vec(a, x);
    expect(y(0, 0) == 17 && y(1, 0) == 39, "Matrix vector multiply");
    const auto at = arc::dsp::transpose(a);
    expect(at(0, 1) == 3 && at(1, 0) == 2, "Matrix transpose");

    arc::dsp::Kalman<float, 1, 1> filter{};
    filter.x(0, 0) = 0.0F;
    filter.p(0, 0) = 1.0F;
    filter.predict(arc::dsp::Matrix<float, 1, 1>{{1.0F}}, arc::dsp::Matrix<float, 1, 1>{{0.0F}});
    filter.correct_diagonal(
        arc::dsp::Matrix<float, 1, 1>{{1.0F}},
        arc::dsp::Matrix<float, 1, 1>{{1.0F}},
        arc::dsp::Matrix<float, 1, 1>{{10.0F}});
    expect(filter.x(0, 0) == 5.0F, "Kalman diagonal correction");

    struct Sink {
        std::size_t bytes{};
        esp_err_t write(std::span<const std::uint8_t> data) noexcept
        {
            bytes += data.size();
            return ESP_OK;
        }
    };
    struct Record {
        std::uint32_t seq{};
        std::uint32_t value{};
    };
    arc::FlashLog<Record, 4, Sink> log{};
    expect(log.push({.seq = 1U, .value = 2U}), "FlashLog push");
    expect(log.flush() == ESP_OK && log.sink.bytes == sizeof(Record), "FlashLog flush");

    const arc::net::SwarmSchedule<3> schedule{
        .node_ids = {10U, 20U, 30U},
        .tdma = {.period_us = 3'000U, .slot_us = 1'000U, .guard_us = 20U, .slot = 0U},
    };
    expect(schedule.slot_for(20U) == 1U, "Swarm slot lookup");
    expect(schedule.window(20U, 1'100U).active, "Swarm TDMA window");
}

void test_trace_provision_ethernet_flashoff_hil_ecs()
{
    struct TraceSink {
        std::array<char, 384> bytes{};
        std::size_t pos{};
        esp_err_t write(std::span<const char> data) noexcept
        {
            if (bytes.size() - pos < data.size()) {
                return ESP_ERR_NO_MEM;
            }
            std::memcpy(bytes.data() + pos, data.data(), data.size());
            pos += data.size();
            return ESP_OK;
        }
    };
    arc::LogLane<4> lane{};
    expect(lane.push(arc::log_id("trace"), 7U, 1U), "TraceStream event push");
    arc::TraceStream<192, TraceSink> stream{};
    expect(stream.drain(lane, "core1") == ESP_OK, "TraceStream drain");
    expect(stream.finish() == ESP_OK, "TraceStream finish");
    expect(std::strstr(stream.sink.bytes.data(), "\"name\":\"core1\"") != nullptr, "TraceStream output");

    std::array<std::uint8_t, 32> pf{};
    const auto perfetto = arc::PerfettoWriter::event(pf, {.tick = 300U, .id = 2U, .payload = 4U, .aux = 1U});
    expect(perfetto.has_value() && perfetto->size() > 0U && pf[0] == 0x08U, "Perfetto binary event");

    arc::Provisioning<32, 64, 128> provisioning{};
    const std::array<std::uint8_t, 4> nonce{1, 2, 3, 4};
    const std::array<std::uint8_t, 3> key{5, 6, 7};
    const std::array<std::uint8_t, 4> ssid{'a', 'r', 'c', 0};
    expect(provisioning.begin(nonce) == ESP_OK, "Provisioning begin");
    expect(provisioning.keys(key) == ESP_OK, "Provisioning keys");
    expect(provisioning.credentials(ssid, key) == ESP_OK && provisioning.ready(), "Provisioning credentials");

    arc::net::EthernetRing<128, 4> eth{};
    const std::array<std::uint8_t, 6> frame{0, 1, 2, 3, 4, 5};
    expect(eth.push(frame), "Ethernet frame push");
    arc::net::EthernetRing<128, 4>::Frame popped{};
    expect(eth.pop(popped) && popped.size == frame.size() && popped.bytes[5] == 5U, "Ethernet frame pop");

    struct SpiPolicy {
        static esp_err_t writev(const std::array<std::uint8_t, 3>& header, std::span<const std::uint8_t> frame) noexcept
        {
            w5500_policy_bytes = header.size() + frame.size();
            return header[2] == arc::net::W5500MacRaw::write ? ESP_OK : ESP_FAIL;
        }
    };
    arc::net::W5500Raw<SpiPolicy, 128, 4> w5500{};
    expect(w5500.send(frame) == ESP_OK && w5500_policy_bytes == 9U, "W5500 raw send");
    expect(w5500.ingest(frame) == ESP_OK, "W5500 raw ingest");

    struct FlashPolicy {
        static esp_err_t enter() noexcept
        {
            flash_policy_active = true;
            return ESP_OK;
        }
        static esp_err_t leave() noexcept
        {
            flash_policy_active = false;
            return ESP_OK;
        }
    };
    arc::FlashOff<FlashPolicy> flash{};
    expect(flash.enter() == ESP_OK && flash.on() && flash_policy_active, "FlashOff enter");
    expect(flash.leave() == ESP_OK && !flash.on() && !flash_policy_active, "FlashOff leave");

    expect(arc::Hil::within({.expected_tick = 100U, .observed_tick = 103U, .tolerance_ticks = 3U}), "HIL within deadline");
    expect(!arc::Hil::within({.expected_tick = 100U, .observed_tick = 104U, .tolerance_ticks = 3U}), "HIL rejects late");

    arc::SwarmSoa<4> swarm{};
    expect(swarm.push(1U, 1.0F, 2.0F, 0.1F, 0.2F), "ECS swarm push");
    expect(swarm.id.span()[0] == 1U && swarm.vx.span()[0] == 0.1F, "ECS SoA spans");
}

void test_log_lane()
{
    static_assert(arc::log_id("core1.overrun") != arc::log_id("core1.watchdog"));
    arc::LogLane<4> lane;
    constexpr auto event = arc::log_id("core1.overrun");

    expect(lane.push(event, 7U, 9U), "LogLane push first");
    expect(lane.push(event, 8U, 10U), "LogLane push second");
    expect(lane.push(event, 9U, 11U), "LogLane push third");
    expect(!lane.push(event, 10U, 12U), "LogLane reports full");
    expect(lane.dropped() == 1U, "LogLane dropped count");

    std::uint32_t payload_sum{};
    const auto drained = lane.drain([&](const arc::LogEvent& item) {
        expect(item.id == event, "LogLane id");
        payload_sum += item.payload;
    });
    expect(drained == 3U, "LogLane drain count");
    expect(payload_sum == 24U, "LogLane payloads");
}

void test_trace_event()
{
    std::array<char, 256> out{};
    const auto begin = arc::TraceEventWriter::json_begin(std::span(out));
    expect(begin.has_value(), "TraceEvent JSON begin");
    expect(std::memcmp(begin->data(), "{\"traceEvents\":[", begin->size()) == 0, "TraceEvent begin text");

    const arc::LogEvent event{.tick = 1234U, .id = 55U, .payload = 77U, .aux = 1U};
    const auto encoded = arc::TraceEventWriter::json_event(std::span(out), event, true, "core1.loop");
    expect(encoded.has_value(), "TraceEvent JSON event");
    expect(std::strstr(out.data(), "\"name\":\"core1.loop\"") != nullptr, "TraceEvent name");
    expect(std::strstr(out.data(), "\"ts\":1234") != nullptr, "TraceEvent timestamp");
    expect(std::strstr(out.data(), "\"payload\":77") != nullptr, "TraceEvent payload");

    std::array<char, 8> small{};
    expect(!arc::TraceEventWriter::json_event(std::span(small), event), "TraceEvent small buffer rejects");
}

void test_postmortem()
{
    using Dump = arc::Postmortem<4>;
    Dump::clear();
    Dump::boot();
    expect(Dump::valid(), "Postmortem valid");
    expect(Dump::header().boots == 1U, "Postmortem boot count");

    Dump::append(arc::LogEvent{.tick = 1U, .id = 10U, .payload = 20U, .aux = 30U});
    Dump::append(arc::LogEvent{.tick = 2U, .id = 11U, .payload = 21U, .aux = 31U});
    Dump::append(arc::LogEvent{.tick = 3U, .id = 12U, .payload = 22U, .aux = 32U});
    Dump::append(arc::LogEvent{.tick = 4U, .id = 13U, .payload = 23U, .aux = 33U});
    Dump::append(arc::LogEvent{.tick = 5U, .id = 14U, .payload = 24U, .aux = 34U});
    expect(Dump::size() == 4U, "Postmortem ring size");
    expect(Dump::event(0).tick == 2U && Dump::event(3).tick == 5U, "Postmortem keeps newest tail");

    arc::LogLane<4> lane;
    expect(lane.push(99U, 1U, 2U), "Postmortem lane push");
    expect(Dump::capture(lane) == 1U, "Postmortem capture count");
    expect(Dump::event(3).id == 99U, "Postmortem capture event");
}

void test_timesync()
{
    arc::TimeSync sync{};
    const auto first = sync.discipline(
        arc::TimeSyncSample{
            .local_send_us = 1'000,
            .remote_recv_us = 1'120,
            .remote_send_us = 1'140,
            .local_recv_us = 1'040,
        },
        arc::TimeSyncConfig{.kp_shift = 0, .ki_shift = 8, .max_step_us = 1'000, .max_integral_us = 1'000});
    expect(first.raw_offset_us == 110, "TimeSync raw offset");
    expect(first.filtered_offset_us == 110, "TimeSync correction");
    expect(first.delay_us == 20, "TimeSync delay");
    expect(sync.local_to_remote(1'000) == 1'110, "TimeSync local to remote");
    expect(sync.remote_to_local(1'110) == 1'000, "TimeSync remote to local");

    const auto second = sync.discipline(
        arc::TimeSyncSample{
            .local_send_us = 2'000,
            .remote_recv_us = 2'120,
            .remote_send_us = 2'120,
            .local_recv_us = 2'000,
        },
        arc::TimeSyncConfig{.kp_shift = 1, .ki_shift = 8, .max_step_us = 5, .max_integral_us = 1'000});
    expect(second.correction_us == 5, "TimeSync correction clamp");
    expect(second.filtered_offset_us == 115, "TimeSync filtered offset");

    const auto hw = sync.discipline_hw(
        {
            .local_send_hw = 4000,
            .remote_recv_hw = 8000,
            .remote_send_hw = 8400,
            .local_recv_hw = 4800,
            .local_hw_to_us_shift = 2,
            .remote_hw_to_us_shift = 3,
        },
        arc::TimeSyncConfig{.kp_shift = 1, .ki_shift = 4, .max_step_us = 1000, .max_integral_us = 1000});
    expect(hw.samples == 3U, "TimeSync hardware timestamp sample count");
}

void test_pack()
{
    enum class Kind : std::uint8_t {
        sample = 0x41U,
    };

    using Telemetry = arc::pack::Schema<Kind, std::uint16_t, std::int32_t>;
    static_assert(Telemetry::bytes == 7U);

    std::array<std::uint8_t, Telemetry::bytes> frame{};
    const auto encoded = Telemetry::write(frame, Kind::sample, 0x1234U, -2);
    expect(encoded.has_value(), "Pack encode");
    expect((frame == std::array<std::uint8_t, 7>{0x41U, 0x12U, 0x34U, 0xffU, 0xffU, 0xffU, 0xfeU}), "Pack big endian");

    Kind kind{};
    std::uint16_t id{};
    std::int32_t value{};
    expect(Telemetry::read(*encoded, kind, id, value).has_value(), "Pack decode");
    expect(kind == Kind::sample && id == 0x1234U && value == -2, "Pack roundtrip");

    std::array<std::uint8_t, Telemetry::bytes> little{};
    expect(Telemetry::write<arc::pack::Endian::little>(little, Kind::sample, 0x1234U, -2).has_value(),
           "Pack little encode");
    expect((little == std::array<std::uint8_t, 7>{0x41U, 0x34U, 0x12U, 0xfeU, 0xffU, 0xffU, 0xffU}),
           "Pack little endian");

    struct PlainTelemetry {
        std::uint32_t seq{};
        std::int16_t temp{};
    };
    using PlainWire = arc::pack::StructOf<PlainTelemetry, &PlainTelemetry::seq, &PlainTelemetry::temp>;
    static_assert(PlainWire::bytes == 6U);

    const PlainTelemetry plain{.seq = 0x01020304U, .temp = -20};
    std::array<std::uint8_t, PlainWire::bytes> plain_frame{};
    const auto plain_encoded = arc::pack::serialize<arc::pack::Endian::big, PlainWire>(plain_frame, plain);
    expect(plain_encoded.has_value(), "Pack struct encode");
    expect((plain_frame == std::array<std::uint8_t, 6>{0x01U, 0x02U, 0x03U, 0x04U, 0xffU, 0xecU}),
           "Pack struct big endian");

    PlainTelemetry decoded{};
    expect(arc::pack::deserialize<arc::pack::Endian::big, PlainWire>(*plain_encoded, decoded).has_value(),
           "Pack struct decode");
    expect(decoded.seq == plain.seq && decoded.temp == plain.temp, "Pack struct roundtrip");

    using ReflectedWire = arc::pack::Reflect<ReflectedTelemetry>;
    static_assert(ReflectedWire::bytes == 6U);
    ReflectedTelemetry reflected{.seq = 7U, .temp = -8};
    std::array<std::uint8_t, ReflectedWire::bytes> reflected_frame{};
    expect(arc::pack::serialize<arc::pack::Endian::big, ReflectedWire>(reflected_frame, reflected).has_value(),
           "Pack reflected encode");
    ReflectedTelemetry reflected_out{};
    expect(arc::pack::deserialize<arc::pack::Endian::big, ReflectedWire>(reflected_frame, reflected_out).has_value(),
           "Pack reflected decode");
    expect(reflected_out.seq == reflected.seq && reflected_out.temp == reflected.temp, "Pack reflected roundtrip");
}

void test_static_silicon_facades()
{
    struct BareProgram {
        static void loop() noexcept {}
    };

    std::array<std::byte, 256> stack{};
    auto token = std::uint32_t{0xA5U};
    expect((arc::BareCore<BareProgram, MockBarePolicy>::boot(stack, &token).has_value()), "BareCore boot");
    expect(bare_core_boots == 1U && MockBarePolicy::config.stack.size() == stack.size(), "BareCore policy handoff");
    expect(MockBarePolicy::context == &token, "BareCore context");

    arc::DmaChain<1> input{};
    arc::DmaChain<1> output{};
    std::array<std::uint8_t, 16> in_bytes{};
    std::array<std::uint8_t, 16> out_bytes{};
    input.bind(0, in_bytes);
    output.bind(0, out_bytes, true);
    expect((arc::crypto_dma_submit<1, 1, MockCryptoDmaPolicy>(
                input,
                output,
                in_bytes.size(),
                arc::CryptoDmaMode::gcm_encrypt)
                .has_value()),
           "CryptoDma submit");
    expect(crypto_dma_bytes == in_bytes.size() && arc::CryptoDma<MockCryptoDmaPolicy>::done(), "CryptoDma policy");

    auto mapped = arc::MmuSpan<std::uint32_t>::map<MockMmuPolicy>(
        {.source = 0U, .offset = 0U, .bytes = 3U * sizeof(std::uint32_t), .memory = 0U});
    expect(mapped.has_value(), "MmuSpan map");
    expect(mapped->size() == 3U && (*mapped)[1] == 0x20U, "MmuSpan view");
    mapped->unmap<MockMmuPolicy>();
    expect(MockMmuPolicy::unmapped, "MmuSpan unmap");

    static auto handled = false;
    constexpr auto handler = [](void*) noexcept { handled = true; };
    using Irq = arc::InterruptMatrix<12, 5, 3, handler, MockInterruptPolicy>;
    expect(Irq::bind(&token).has_value(), "InterruptMatrix bind");
    expect(interrupt_binds == 1U && MockInterruptPolicy::last.source == 12, "InterruptMatrix route");
    MockInterruptPolicy::handler(MockInterruptPolicy::arg);
    expect(handled, "InterruptMatrix handler");

    enum class State : std::uint8_t {
        idle,
        armed,
        fault,
    };
    enum class Event : std::uint8_t {
        arm,
        trip,
        reset,
    };
    using Machine = arc::fsm::Automaton<
        State::idle,
        arc::fsm::Terminals<State::fault>,
        arc::fsm::Transition<State::idle, Event::arm, State::armed>,
        arc::fsm::Transition<State::armed, Event::trip, State::fault>,
        arc::fsm::Transition<State::armed, Event::reset, State::idle>>;
    static_assert(Machine::valid());
    expect(Machine::dispatch(State::idle, Event::arm) == State::armed, "FSM transition");
    expect(Machine::dispatch(State::fault, Event::reset) == State::fault, "FSM terminal hold");
}

void test_rcu()
{
    struct Config {
        std::uint32_t seq{};
        std::array<std::int16_t, 4> taps{};
    };

    arc::Rcu<Config> config{};
    config.write({.seq = 1U, .taps = {1, 2, 3, 4}});
    expect(config.read().seq == 1U && config.read().taps[3] == 4, "RCU first publish");

    auto& staging = config.staging();
    staging = {.seq = 2U, .taps = {5, 6, 7, 8}};
    expect(config.read().seq == 1U, "RCU staging invisible");
    config.publish();
    expect(config.read().seq == 2U && config.read().taps[0] == 5, "RCU publish flips buffer");
}

void test_ulp_shared()
{
    struct Payload {
        std::uint32_t seq{};
        std::uint32_t flags{};
    };

    static_assert(alignof(arc::Ulp::Shared<Payload>) == 8U);
    arc::Ulp::Shared<Payload> shared{};
    shared.write({.seq = 7U, .flags = 0x55U});

    Payload out{};
    expect(shared.read(out), "ULP shared reads stable snapshot");
    expect(out.seq == 7U && out.flags == 0x55U, "ULP shared payload");
}

void test_probe_stats()
{
    arc::CycleStats cycles{};
    cycles.add(10U);
    cycles.add(30U);
    expect(cycles.min == 10U && cycles.max == 30U && cycles.avg() == 20U, "CycleStats aggregate");
    cycles.clear();
    expect(cycles.samples == 0U && cycles.max == 0U, "CycleStats clear");

    arc::JitterStats jitter{};
    jitter.add(-5);
    jitter.add(7);
    jitter.add(1);
    expect(jitter.min == -5 && jitter.max == 7, "JitterStats min/max");
    expect(jitter.avg_abs() == 4U && jitter.max_abs() == 7U, "JitterStats abs");
    jitter.clear();
    expect(jitter.samples == 0U && jitter.avg_abs() == 0U, "JitterStats clear");

    arc::DeadlineStats deadline{};
    deadline.add(80U, 100U);
    deadline.add(130U, 100U);
    expect(deadline.min_slack == -30 && deadline.max_slack == 20, "DeadlineStats slack");
    expect(deadline.avg_slack() == -5 && deadline.overruns == 1U && deadline.max_overrun == 30, "DeadlineStats overrun");
    deadline.clear();
    expect(deadline.samples == 0U && deadline.overruns == 0U, "DeadlineStats clear");

    expect(arc::Clock::ns(240U, 240U) == 1000U, "Clock cycle ns");
    expect(arc::Clock::signed_ns(-240, 240U) == -1000, "Clock signed cycle ns");
}

void test_seqreg()
{
    struct Snapshot {
        std::uint32_t seq;
        std::uint32_t inv;
        std::uint64_t total;
    };

    arc::SeqReg<Snapshot> reg;
    reg.write_unmasked(Snapshot{
        .seq = 0U,
        .inv = ~0U,
        .total = 0U,
    });
    std::atomic<bool> running{true};

    std::thread writer([&]() {
        for (std::uint32_t seq = 1; seq <= 20000U; ++seq) {
            reg.write_unmasked(Snapshot{
                .seq = seq,
                .inv = ~seq,
                .total = static_cast<std::uint64_t>(seq) * 3U,
            });
        }
        running.store(false, std::memory_order_release);
    });

    std::uint32_t last{};
    Snapshot snap{};
    while (running.load(std::memory_order_acquire)) {
        if (!reg.try_read(snap)) {
            std::this_thread::yield();
            continue;
        }

        expect(snap.inv == ~snap.seq, "SeqReg snapshot inverse");
        expect(snap.total == static_cast<std::uint64_t>(snap.seq) * 3U, "SeqReg snapshot total");
        expect(snap.seq >= last, "SeqReg monotonic snapshot");
        last = snap.seq;
    }

    writer.join();

    const auto final = reg.read();
    expect(final.seq == 20000U, "SeqReg final sequence");
    expect(final.inv == ~20000U, "SeqReg final inverse");
    expect(final.total == static_cast<std::uint64_t>(20000U) * 3U, "SeqReg final total");
}

void test_claim()
{
    using Owner = arc::ClaimFor<arc::ClaimKind::uart, 91, 1, 2, 3>;
    using Same = arc::ClaimFor<arc::ClaimKind::uart, 91, 1, 2, 3>;
    using Other = arc::ClaimFor<arc::ClaimKind::uart, 91, 1, 2, 4>;
    using TokenCollision = arc::Claim<arc::ClaimKind::uart, 91, arc::claim_token<1, 2, 3>(), arc::claim_proof<9, 9, 9>()>;

    Owner::drop();
    Other::drop();
    expect(Owner::take() == ESP_OK, "Claim first owner succeeds");
    expect(Same::take() == ESP_OK, "Claim identical owner shares");
    expect(Other::take() == ESP_ERR_INVALID_STATE, "Claim different token rejects");
    expect(TokenCollision::take() == ESP_ERR_INVALID_STATE, "Claim token collision rejects");
    expect(Owner::held(), "Claim owner held");
    Owner::drop();
    expect(!Owner::held(), "Claim owner drops");
    expect(Other::take() == ESP_OK, "Claim other owner succeeds after drop");
    Other::drop();
}

void test_file()
{
    char path[] = "/tmp/arc-host-file-XXXXXX";
    const auto fd = mkstemp(path);
    expect(fd >= 0, "File temp path");
    expect(close(fd) == 0, "File temp close");

    {
        auto file = arc::File::open(path, "wb");
        expect(file.has_value(), "File open write");

        auto handle = std::move(file).value();
        const std::array<std::uint8_t, 4> payload{1U, 3U, 3U, 7U};
        const auto wrote = handle.write(payload.data(), payload.size());
        expect(wrote.has_value() && wrote.value() == payload.size(), "File write bytes");
        expect(handle.flush() == ESP_OK, "File flush");
        expect(handle.close() == ESP_OK, "File close write");
    }

    {
        auto file = arc::File::open(path, "rb");
        expect(file.has_value(), "File open read");

        auto handle = std::move(file).value();
        std::array<std::uint8_t, 4> payload{};
        const auto read = handle.read(payload.data(), payload.size());
        expect(read.has_value() && read.value() == payload.size(), "File read bytes");
        expect((payload == std::array<std::uint8_t, 4>{1U, 3U, 3U, 7U}), "File payload");
        expect(handle.close() == ESP_OK, "File close read");
    }

    {
        auto missing = arc::File::open("/tmp/arc-host-file-missing", "rb");
        expect(!missing.has_value(), "File missing path");
        expect(missing.error() == ESP_ERR_NOT_FOUND, "File missing error");
    }

    expect(std::remove(path) == 0, "File cleanup");
}

void test_stream()
{
    struct Mock {
        std::array<std::uint8_t, 32> tx{};
        std::array<std::uint8_t, 32> rx{};
        std::size_t tx_pos{};
        std::size_t rx_pos{};
        std::size_t rx_len{};

        arc::Status send_all(const void* const data, const std::size_t bytes) noexcept
        {
            if (tx_pos + bytes > tx.size()) {
                return arc::Status{arc::fail(ESP_ERR_NO_MEM)};
            }
            std::memcpy(tx.data() + tx_pos, data, bytes);
            tx_pos += bytes;
            return arc::ok();
        }

        arc::Result<std::size_t> recv(void* const data, const std::size_t bytes) noexcept
        {
            if (rx_pos >= rx_len) {
                return std::size_t{};
            }

            const auto n = bytes < 2U ? bytes : 2U;
            const auto left = rx_len - rx_pos;
            const auto got = n < left ? n : left;
            std::memcpy(data, rx.data() + rx_pos, got);
            rx_pos += got;
            return got;
        }
    };

    static_assert(arc::net::ByteStream<Mock>);

    Mock io{};
    const std::array<std::uint8_t, 3> payload{1U, 2U, 3U};
    expect(arc::net::Stream::write_frame16(io, std::span(payload)).has_value(), "Stream frame write");
    expect(io.tx_pos == 5U, "Stream frame bytes");
    expect(io.tx[0] == 0U && io.tx[1] == 3U && io.tx[4] == 3U, "Stream frame layout");
    expect(arc::net::Stream::write_frame16(io, nullptr, 0U).has_value(), "Stream empty frame write");
    expect(io.tx_pos == 7U && io.tx[5] == 0U && io.tx[6] == 0U, "Stream empty frame layout");

    std::array<std::uint8_t, 8> encoded{};
    const auto coded = arc::net::Stream::frame16(std::span(encoded), std::span(payload));
    expect(coded.has_value() && coded->size() == 5U, "Stream frame encode");
    expect((*coded)[0] == 0U && (*coded)[1] == 3U && (*coded)[4] == 3U, "Stream frame encode layout");

    io.rx = {0U, 3U, 7U, 8U, 9U};
    io.rx_len = 5U;
    std::array<std::uint8_t, 4> out{};
    const auto frame = arc::net::Stream::read_frame16(io, std::span(out));
    expect(frame.has_value() && *frame == 3U, "Stream frame read");
    expect((out == std::array<std::uint8_t, 4>{7U, 8U, 9U, 0U}), "Stream frame payload");

    io.rx = {0U, 5U, 1U, 2U, 3U, 4U, 5U};
    io.rx_pos = 0U;
    io.rx_len = 7U;
    expect(!arc::net::Stream::read_frame16(io, std::span(out)), "Stream oversized frame rejects");
}

void test_refinit()
{
    std::uint32_t state{};
    expect(arc::Init::is_empty(state), "Init starts empty");
    expect(!arc::Init::is_busy(state), "Init not busy initially");
    expect(!arc::Init::is_ready(state), "Init not ready initially");

    {
        arc::TryGate first(state);
        expect(static_cast<bool>(first), "TryGate takes open gate");

        arc::TryGate second(state);
        expect(!static_cast<bool>(second), "TryGate rejects shut gate");

        auto moved = std::move(first);
        expect(!static_cast<bool>(first), "TryGate move clears source");
        expect(static_cast<bool>(moved), "TryGate move keeps destination");
        moved.reset();
    }
    expect(arc::Gate::try_take(state), "TryGate reset opens gate");
    arc::Gate::drop(state);

    arc::MutexGateState mutex{};
    {
        arc::TryMutexGate first(mutex);
        expect(static_cast<bool>(first), "TryMutexGate takes mutex");

        auto moved_mutex = std::move(first);
        expect(!static_cast<bool>(first), "TryMutexGate move clears source");
        expect(static_cast<bool>(moved_mutex), "TryMutexGate move keeps destination");
        moved_mutex.reset();
    }
    {
        arc::MutexGate guard(mutex);
        static_cast<void>(guard);
    }

    {
        auto txn = arc::InitTxn::begin(state);
        expect(static_cast<bool>(txn), "InitTxn first begin initializes");
        expect(arc::Init::is_busy(state), "InitTxn begin marks busy");
    }
    expect(arc::Init::is_empty(state), "InitTxn destructor restores empty");
    expect(arc::InitTxn::begin(state).pass(), "InitTxn destructor rolls back busy state");
    expect(arc::Init::is_ready(state), "InitTxn pass marks ready");
    expect(!arc::InitTxn::begin(state), "InitTxn ready state does not initialize");
    expect(arc::Init::take(state), "InitTxn pass leaves ready state");
    expect(arc::Init::is_busy(state), "Init take marks busy");
    arc::Init::fail(state);
    expect(arc::Init::is_empty(state), "Init fail restores empty");

    auto txn = arc::InitTxn::begin(state);
    expect(static_cast<bool>(txn), "InitTxn move source active");
    auto moved = std::move(txn);
    expect(!static_cast<bool>(txn), "InitTxn move clears source");
    expect(static_cast<bool>(moved), "InitTxn move keeps destination");
    expect(moved.fail(), "InitTxn explicit fail");

    {
        auto ref_txn = arc::RefInitTxn::begin(state);
        expect(static_cast<bool>(ref_txn), "RefInitTxn first begin initializes");
        expect(arc::RefInit::is_busy(state), "RefInitTxn begin marks busy");
    }
    expect(arc::RefInit::is_empty(state), "RefInitTxn destructor restores empty");
    expect(arc::RefInitTxn::begin(state).pass(), "RefInitTxn destructor rolls back busy state");
    expect(arc::RefInit::is_ready(state), "RefInitTxn pass marks ready");
    expect(arc::RefInit::refs(state) == 1U, "RefInitTxn pass owns first ref");
    expect(!arc::RefInitTxn::begin(state), "RefInitTxn ready state shares without init");
    expect(arc::RefInit::refs(state) == 2U, "RefInitTxn ready begin increments refs");
    expect(!arc::RefInit::drop(state), "RefInitTxn shared drop");
    expect(arc::RefInit::drop(state), "RefInitTxn final drop");
    arc::RefInit::fail(state);

    auto ref_txn = arc::RefInitTxn::begin(state);
    expect(static_cast<bool>(ref_txn), "RefInitTxn move source active");
    auto moved_ref = std::move(ref_txn);
    expect(!static_cast<bool>(ref_txn), "RefInitTxn move clears source");
    expect(static_cast<bool>(moved_ref), "RefInitTxn move keeps destination");
    expect(moved_ref.fail(), "RefInitTxn explicit fail");

    expect(arc::RefInit::begin(state), "RefInit first begin initializes");
    arc::RefInit::pass(state);
    expect(arc::RefInit::refs(state) == 1U, "RefInit initializer owns first ref");
    expect(!arc::RefInit::begin(state), "RefInit second begin shares ready resource");
    expect(arc::RefInit::refs(state) == 2U, "RefInit shared begin increments refs");
    expect(arc::RefInit::take(state), "RefInit take existing resource");
    expect(arc::RefInit::refs(state) == 3U, "RefInit take increments refs");
    expect(!arc::RefInit::drop(state), "RefInit non-last drop keeps resource live");
    expect(!arc::RefInit::drop(state), "RefInit second non-last drop keeps resource live");
    expect(arc::RefInit::drop(state), "RefInit last drop owns teardown");
    arc::RefInit::fail(state);
    expect(arc::RefInit::refs(state) == 0U, "RefInit teardown returns empty");
    expect(!arc::RefInit::take(state), "RefInit take empty resource fails");

    expect(arc::RefInit::begin(state), "RefLease init begin");
    arc::RefInit::pass(state);
    {
        auto lease = arc::RefLease::take(state);
        expect(static_cast<bool>(lease), "RefLease takes ready resource");
        expect(arc::RefInit::refs(state) == 2U, "RefLease increments refs");

        auto moved = std::move(lease);
        expect(!static_cast<bool>(lease), "RefLease move clears source");
        expect(static_cast<bool>(moved), "RefLease move keeps destination");
    }
    expect(arc::RefInit::refs(state) == 1U, "RefLease destructor drops non-final ref");

    auto lease = arc::RefLease::take(state);
    expect(!lease.release(), "RefLease explicit non-final release");
    expect(arc::RefInit::refs(state) == 1U, "RefLease release leaves owner ref");

    auto owner = arc::RefLease::adopt(state);
    expect(owner.release(), "RefLease explicit final release owns teardown");
    arc::RefInit::fail(state);
    expect(arc::RefInit::refs(state) == 0U, "RefLease final teardown returns empty");

    expect_abort(
        []() {
            std::uint32_t local{};
            expect(arc::RefInit::begin(local), "RefLease abort begin");
            arc::RefInit::pass(local);
            auto final = arc::RefLease::adopt(local);
            static_cast<void>(final);
        },
        "RefLease final destructor aborts");
}

}  // namespace

int main()
{
    test_any_io();
    test_spsc();
    test_checked_spsc();
    test_mpsc_single();
    test_compact_mpsc();
    test_checked_mpsc();
    test_mpsc_threads();
    test_fanin();
    test_checked_fanin();
    test_rpc_lane();
    test_mqtt();
    test_ws();
    test_coap();
    test_invalid_codecs();
    test_http_server();
    test_caps();
    test_dsp();
    test_foc_motion_tdma();
    test_netrpc_pms_secure_update();
    test_matrix_kalman_storage_swarm();
    test_trace_provision_ethernet_flashoff_hil_ecs();
    test_log_lane();
    test_trace_event();
    test_postmortem();
    test_timesync();
    test_pack();
    test_static_silicon_facades();
    test_rcu();
    test_ulp_shared();
    test_probe_stats();
    test_seqreg();
    test_claim();
    test_file();
    test_stream();
    test_refinit();
    std::puts("arc host tests: OK");
}
