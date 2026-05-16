#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory_resource>
#include <span>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "arc/any.hpp"
#include "arc/acoustic_slam.hpp"
#include "arc/bare_core.hpp"
#include "arc/ble_mesh.hpp"
#include "arc/blackbox.hpp"
#include "arc/caps.hpp"
#include "arc/cert_bundle.hpp"
#include "arc/chaos.hpp"
#include "arc/cli.hpp"
#include "arc/claim.hpp"
#include "arc/concepts.hpp"
#include "arc/covert.hpp"
#include "arc/csi.hpp"
#include "arc/crypto_dma.hpp"
#include "arc/dsp.hpp"
#include "arc/digital_twin.hpp"
#include "arc/distributed_mmu.hpp"
#include "arc/ecs.hpp"
#include "arc/ethernet.hpp"
#include "arc/fanin.hpp"
#include "arc/file.hpp"
#include "arc/flash_log.hpp"
#include "arc/flash_off.hpp"
#include "arc/flexroute.hpp"
#include "arc/foc.hpp"
#include "arc/fsm.hpp"
#include "arc/hil.hpp"
#include "arc/hotpatch.hpp"
#include "arc/hypervisor.hpp"
#include "arc/http.hpp"
#include "arc/http_server.hpp"
#include "arc/init.hpp"
#include "arc/interrupt_matrix.hpp"
#include "arc/ipc.hpp"
#include "arc/intermittent.hpp"
#include "arc/isp.hpp"
#include "arc/kalman.hpp"
#include "arc/kyber.hpp"
#include "arc/jit.hpp"
#include "arc/coap.hpp"
#include "arc/cloak.hpp"
#include "arc/lifi.hpp"
#include "arc/log.hpp"
#include "arc/copy.hpp"
#include "arc/cnc.hpp"
#include "arc/fabric.hpp"
#include "arc/hls.hpp"
#include "arc/hyper_matrix.hpp"
#include "arc/matrix.hpp"
#include "arc/migrator.hpp"
#include "arc/maglev.hpp"
#include "arc/motion.hpp"
#include "arc/mmu_span.hpp"
#include "arc/ml.hpp"
#include "arc/mqtt.hpp"
#include "arc/mpsc.hpp"
#include "arc/nav.hpp"
#include "arc/netrpc.hpp"
#include "arc/nvs_crypto.hpp"
#include "arc/paillier.hpp"
#include "arc/pack.hpp"
#include "arc/perfetto.hpp"
#include "arc/pipeline.hpp"
#include "arc/pms.hpp"
#include "arc/pmr.hpp"
#include "arc/postmortem.hpp"
#include "arc/power_profiler.hpp"
#include "arc/power_governor.hpp"
#include "arc/probe.hpp"
#include "arc/provisioning.hpp"
#include "arc/pru.hpp"
#include "arc/puf.hpp"
#include "arc/rcu.hpp"
#include "arc/rdma.hpp"
#include "arc/rpc.hpp"
#include "arc/rtos.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/stream.hpp"
#include "arc/secure_update.hpp"
#include "arc/sdr.hpp"
#include "arc/simd.hpp"
#include "arc/sdio_slave.hpp"
#include "arc/snn.hpp"
#include "arc/secure_boot.hpp"
#include "arc/star_tracker.hpp"
#include "arc/swarm.hpp"
#include "arc/tdma.hpp"
#include "arc/tee.hpp"
#include "arc/thread.hpp"
#include "arc/timesync.hpp"
#include "arc/tcp.hpp"
#include "arc/text.hpp"
#include "arc/tls.hpp"
#include "arc/trace_event.hpp"
#include "arc/trace_live.hpp"
#include "arc/trace_stream.hpp"
#include "arc/ulp.hpp"
#include "arc/ulp_cxx.hpp"
#include "arc/ulp_ml.hpp"
#include "arc/uri.hpp"
#include "arc/usb_device.hpp"
#include "arc/usb_host.hpp"
#include "arc/vm.hpp"
#include "arc/wasm_aot.hpp"
#include "arc/vision.hpp"
#include "arc/vslam.hpp"
#include "arc/ftm.hpp"
#include "arc/wavefront.hpp"
#include "arc/ws.hpp"
#include "arc/w5500.hpp"

struct ReflectedTelemetry {
    std::uint32_t seq{};
    std::int16_t temp{};
};

ARC_PACK_REFLECT(ReflectedTelemetry, &ReflectedTelemetry::seq, &ReflectedTelemetry::temp);

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);

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
static_assert(arc::rtos::tick_hz() == 1000U);
static_assert(arc::rtos::milliseconds(std::chrono::microseconds{1500}) == 2U);
static_assert(arc::rtos::milliseconds(std::chrono::milliseconds{-1}) == 0U);
static_assert(arc::rtos::ticks_ms(2U) == 2U);
static_assert(arc::rtos::ticks(std::chrono::microseconds{1500}) == 2U);
static_assert(sizeof(decltype(arc::claim_token<1, 2, 3>())) == sizeof(std::uint64_t));
static_assert(arc::claim_token<1, 2, 3>() != arc::claim_token<1, 2, 4>());
static_assert(arc::claim_proof<1, 2, 3>() != arc::claim_proof<1, 2, 4>());
static_assert(arc::Result<int>{2}.and_then([](int value) { return arc::Result<int>{value + 1}; }).value() == 3);
static_assert(arc::status_code(arc::ok()) == ESP_OK);

[[nodiscard]] bool near(const float lhs, const float rhs, const float epsilon = 0.0001F) noexcept
{
    return std::fabs(lhs - rhs) <= epsilon;
}

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
bool ulp_policy_gpio{};
bool ulp_policy_woke{};
std::size_t bare_core_boots{};
std::size_t crypto_dma_bytes{};
std::size_t interrupt_binds{};
std::size_t tee_regions{};
std::size_t tee_trusted_peripherals{};
std::size_t tee_untrusted_peripherals{};
bool csi_policy_started{};
arc::net::CsiCallback csi_policy_callback{};
void* csi_policy_user{};
std::size_t hotpatch_locks{};
std::size_t hotpatch_stores{};
std::size_t hotpatch_syncs{};
std::size_t hotpatch_unlocks{};
std::uint32_t covert_hz{};
std::uint32_t covert_duty{};
int covert_density{};
bool semantic_wake{};
std::size_t sdr_words{};
std::size_t hypervisor_regions{};
std::size_t hypervisor_traps{};
std::size_t chaos_flips{};
std::size_t chaos_stalls{};
std::size_t chaos_drops{};
std::size_t chaos_overruns{};
std::uint16_t intermittent_bod_mv{};
bool intermittent_restored{};
std::uint32_t flexroute_gpio{};
std::uint32_t flexroute_detached{};
std::size_t wavefront_writes{};
std::uint32_t copy_yields{};
std::uint32_t cloak_heavy{};
std::uint32_t cloak_light{};
std::uint32_t fabric_next_hop{};
std::size_t fabric_bytes{};
std::size_t hil_actions{};
std::size_t usb_host_started{};
std::size_t usb_host_submitted{};
std::uintptr_t rdma_store_addr{};
std::size_t rdma_store_bytes{};
std::size_t lifi_sent_symbols{};
std::size_t lifi_armed{};
std::size_t jit_finalized_words{};
std::uint32_t blackbox_hash_acc{};
std::size_t blackbox_write_bytes{};
float digital_twin_last_output{};
int cli_drive_left{};
int cli_drive_right{};
float cli_kp{};
std::size_t sdio_queued_bytes{};
std::uint32_t sdio_irq_bits{};
std::uint8_t usb_device_address{};
std::uint8_t usb_configuration{};
std::size_t usb_ep0_bytes{};
std::uint16_t thread_peer_rloc{};
std::uint16_t ble_mesh_group{};
arc::IpcCore ipc_core{};
bool ipc_called{};
std::size_t cert_bundle_bytes{};
std::string_view nvs_crypto_partition{};
std::uint8_t boot_revoked{};
std::size_t distributed_fetch_bytes{};
std::uintptr_t distributed_remap_line{};
std::uint8_t distributed_resume_core{};
std::size_t live_trace_bytes{};
std::size_t live_trace_sink_bytes{};
std::uint32_t live_trace_swaps{};
std::uint32_t governor_boosts{};
std::uint32_t governor_releases{};
std::size_t wasm_finalized_words{};
std::size_t wasm_guard_regions{};
std::uint8_t ftm_channel{};
std::uint8_t ftm_frames{};
std::uint16_t migration_peer{};
std::size_t migration_bytes{};
std::uint32_t migration_process{};
std::uint32_t migration_ip{};
std::uint32_t migration_sp{};
std::uint16_t profiler_current{};
std::uint32_t profiler_pc{};
std::uint32_t hypermatrix_node{};
std::size_t hypermatrix_bytes{};
std::array<std::uint8_t, 64> live_trace_buffer{};

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
    using SpscInt = arc::Spsc<int, 4>;
    static_assert(!std::is_copy_constructible_v<SpscInt::Producer>);
    static_assert(!std::is_copy_constructible_v<SpscInt::Consumer>);
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

    auto endpoints = queue.split();
    expect(static_cast<bool>(endpoints.producer) && static_cast<bool>(endpoints.consumer),
           "SPSC role endpoints split");
    expect(endpoints.producer.try_push(10), "SPSC producer endpoint push");
    expect(endpoints.consumer.try_pop(value) && value == 10, "SPSC consumer endpoint pop");
    const std::array endpoint_values{11, 12, 13};
    expect(endpoints.producer.push(std::span(endpoint_values)) == 3U, "SPSC producer endpoint batch push");
    out = {};
    expect(endpoints.consumer.pop(std::span(out)) == 3U && out[2] == 13, "SPSC consumer endpoint batch pop");
    auto moved_producer = std::move(endpoints.producer);
    expect(!static_cast<bool>(endpoints.producer) && static_cast<bool>(moved_producer),
           "SPSC producer endpoint is move-only");
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
    using MpscInt = arc::Mpsc<int, 4>;
    static_assert(std::is_copy_constructible_v<MpscInt::Producer>);
    static_assert(!std::is_copy_constructible_v<MpscInt::Consumer>);
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

    auto endpoints = queue.split();
    auto second_producer = endpoints.producer;
    expect(static_cast<bool>(endpoints.producer) && static_cast<bool>(endpoints.consumer),
           "MPSC role endpoints split");
    expect(endpoints.producer.try_push(12) && second_producer.try_push(13), "MPSC producer endpoints push");
    expect(endpoints.consumer.try_pop(value) && value == 12, "MPSC consumer endpoint pop 12");
    auto moved_consumer = std::move(endpoints.consumer);
    expect(!static_cast<bool>(endpoints.consumer) && static_cast<bool>(moved_consumer),
           "MPSC consumer endpoint is move-only");
    expect(moved_consumer.try_pop(value) && value == 13, "MPSC moved consumer endpoint pop 13");
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
        arc::net::MqttQos::at_least,
        false,
        true);
    expect(publish.has_value(), "MQTT publish encodes");

    const auto packet = arc::net::Mqtt::parse(*publish);
    expect(packet.has_value(), "MQTT publish parses");
    expect(packet->type == arc::net::MqttType::publish, "MQTT publish type");
    expect(packet->retain(), "MQTT publish retain");
    expect(packet->qos() == arc::net::MqttQos::at_least, "MQTT publish qos");

    const auto view = arc::net::Mqtt::view(*packet);
    expect(view.has_value(), "MQTT publish view");
    expect(view->packet == 7U, "MQTT publish packet id");
    expect(view->topic.size() == 9U, "MQTT publish topic bytes");
    expect(std::memcmp(view->topic.data(), "arc/topic", view->topic.size()) == 0, "MQTT publish topic");
    expect(view->payload.size() == payload.size(), "MQTT publish payload size");
    expect(std::memcmp(view->payload.data(), payload.data(), payload.size()) == 0, "MQTT publish payload");

    const std::array subscriptions{
        arc::net::MqttSubscription{.filter = "arc/+/status", .qos = arc::net::MqttQos::at_least},
        arc::net::MqttSubscription{.filter = "arc/#", .qos = arc::net::MqttQos::at_most},
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

void test_uri()
{
    constexpr char full[] =
        "https://node:secret@[2001:db8::1]:8443/api/v1/sample%201?mode=fast&flag&empty=#frag";
    const auto uri = arc::net::Uri::parse(std::span<const char>{full, sizeof(full) - 1U});
    expect(uri.has_value(), "URI full parses");
    expect(uri->absolute() && uri->has_authority(), "URI absolute authority");
    expect(std::memcmp(uri->scheme.data(), "https", uri->scheme.size()) == 0, "URI scheme");
    expect(std::memcmp(uri->userinfo.data(), "node:secret", uri->userinfo.size()) == 0, "URI userinfo");
    expect(std::memcmp(uri->host.data(), "2001:db8::1", uri->host.size()) == 0, "URI bracket host");
    expect(std::memcmp(uri->port.data(), "8443", uri->port.size()) == 0, "URI port");
    expect(std::memcmp(uri->path.data(), "/api/v1/sample%201", uri->path.size()) == 0, "URI path");
    expect(std::memcmp(uri->fragment.data(), "frag", uri->fragment.size()) == 0, "URI fragment");
    expect(arc::net::Uri::scheme_is(*uri, "HTTPS"), "URI scheme compares case-insensitively");
    expect(!arc::net::Uri::scheme_is(*uri, nullptr), "URI scheme null compare rejects");
    expect(!arc::net::Uri::scheme_is(*uri, "http"), "URI scheme mismatch rejects");

    const auto port = arc::net::Uri::port(*uri, 443U);
    expect(port.has_value() && *port == 8443U, "URI port value");
    const auto endpoint = arc::net::Uri::endpoint(*uri, 443U);
    expect(endpoint.has_value() && endpoint->port == 8443U && endpoint->host.size() == uri->host.size(),
           "URI endpoint explicit port");

    std::array<char, 16> host{};
    const auto host_text = arc::net::Uri::copy_host(std::span(host), *uri);
    expect(host_text.has_value(), "URI host copies");
    expect(host[host_text->size()] == '\0' &&
               std::memcmp(host_text->data(), "2001:db8::1", host_text->size()) == 0,
           "URI host copy is nul-terminated");

    std::array<char, 80> target{};
    const auto target_text = arc::net::Uri::path_query(std::span(target), *uri);
    expect(target_text.has_value(), "URI path query writes");
    expect(std::memcmp(target_text->data(), "/api/v1/sample%201?mode=fast&flag&empty=", target_text->size()) == 0,
           "URI path query includes query");

    std::size_t offset{};
    const auto mode = arc::net::Uri::next(uri->query, offset);
    expect(mode.has_value(), "URI query mode");
    expect(
        std::memcmp(mode->key.data(), "mode", mode->key.size()) == 0 &&
            std::memcmp(mode->value.data(), "fast", mode->value.size()) == 0 && mode->has_value,
        "URI query mode value");

    const auto flag = arc::net::Uri::next(uri->query, offset);
    expect(flag.has_value() && !flag->has_value && std::memcmp(flag->key.data(), "flag", flag->key.size()) == 0,
           "URI query flag");

    const auto empty = arc::net::Uri::next(uri->query, offset);
    expect(empty.has_value() && empty->has_value && empty->value.empty(), "URI query empty value");

    const auto done = arc::net::Uri::next(uri->query, offset);
    expect(!done.has_value() && done.error() == ESP_ERR_NOT_FOUND, "URI query end");
    offset = uri->query.size() + 1U;
    const auto bad_offset = arc::net::Uri::next(uri->query, offset);
    expect(!bad_offset.has_value() && bad_offset.error() == ESP_ERR_INVALID_ARG,
           "URI query invalid offset rejects");

    constexpr char sparse_q[] = "&&first=1&&second";
    std::size_t sparse_offset{};
    const auto sparse_first = arc::net::Uri::next(
        std::span<const char>{sparse_q, sizeof(sparse_q) - 1U},
        sparse_offset);
    expect(sparse_first.has_value() && sparse_first->has_value &&
               std::memcmp(sparse_first->key.data(), "first", sparse_first->key.size()) == 0,
           "URI query skips leading empty separators");
    const auto sparse_second = arc::net::Uri::next(
        std::span<const char>{sparse_q, sizeof(sparse_q) - 1U},
        sparse_offset);
    expect(sparse_second.has_value() && !sparse_second->has_value &&
               std::memcmp(sparse_second->key.data(), "second", sparse_second->key.size()) == 0,
           "URI query skips repeated separators");

    constexpr char rel[] = "/config?unit=ph";
    const auto relative = arc::net::Uri::parse(std::span<const char>{rel, sizeof(rel) - 1U});
    expect(relative.has_value() && !relative->absolute() && relative->path.size() == 7U, "URI relative parses");
    expect(!arc::net::Uri::endpoint(*relative, 80U), "URI relative endpoint rejects");

    const auto defaulted = arc::net::Uri::parse("HTTPS://node/api?x=1");
    expect(defaulted.has_value(), "URI default endpoint parses");
    const auto default_endpoint = arc::net::Uri::endpoint(*defaulted, 443U);
    expect(default_endpoint.has_value() && default_endpoint->port == 443U, "URI endpoint default port");
    expect(!arc::net::Uri::port(*defaulted), "URI missing port rejects without fallback");

    const auto encoded_host = arc::net::Uri::parse("http://node%2D1.local/path");
    expect(encoded_host.has_value() && std::memcmp(encoded_host->host.data(), "node%2D1.local", encoded_host->host.size()) == 0,
           "URI percent-escaped reg-name host parses");
    std::array<char, 16> decoded_host{};
    const auto decoded_host_text = arc::net::Uri::copy_host(std::span(decoded_host), *encoded_host);
    expect(decoded_host_text.has_value() &&
               std::memcmp(decoded_host_text->data(), "node-1.local", decoded_host_text->size()) == 0 &&
               decoded_host[decoded_host_text->size()] == '\0',
           "URI host copy decodes percent escapes");

    const auto userinfo = arc::net::Uri::parse("http://ops:pa%24%24@node.local/path");
    expect(userinfo.has_value() &&
               std::memcmp(userinfo->userinfo.data(), "ops:pa%24%24", userinfo->userinfo.size()) == 0,
           "URI userinfo permits pct-encoded subdelims");

    const auto empty_query = arc::net::Uri::parse("http://node/path?");
    expect(empty_query.has_value() && empty_query->has_query && empty_query->query.empty(), "URI empty query flag");
    std::array<char, 16> empty_target{};
    const auto empty_target_text = arc::net::Uri::path_query(std::span(empty_target), *empty_query);
    expect(empty_target_text.has_value() &&
               std::memcmp(empty_target_text->data(), "/path?", empty_target_text->size()) == 0,
           "URI path query preserves empty query");

    const auto query_only = arc::net::Uri::parse("https://node?unit=ph");
    expect(query_only.has_value() && query_only->path.empty() && query_only->has_query,
           "URI authority with query and empty path parses");
    std::array<char, 16> query_only_target{};
    const auto query_only_text = arc::net::Uri::path_query(std::span(query_only_target), *query_only);
    expect(query_only_text.has_value() &&
               std::memcmp(query_only_text->data(), "/?unit=ph", query_only_text->size()) == 0,
           "URI path query defaults empty authority path before query");

    constexpr char manual_bad_path_text[] = "/bad path";
    const arc::net::UriView manual_bad_path{
        .path = std::span<const char>{manual_bad_path_text, sizeof(manual_bad_path_text) - 1U},
    };
    expect(!arc::net::Uri::path_query(std::span(target), manual_bad_path),
           "URI path query rejects manually constructed path space");

    constexpr char manual_split_path_text[] = "/bad?query";
    const arc::net::UriView manual_split_path{
        .path = std::span<const char>{manual_split_path_text, sizeof(manual_split_path_text) - 1U},
    };
    expect(!arc::net::Uri::path_query(std::span(target), manual_split_path),
           "URI path query rejects manually constructed path delimiter");

    constexpr char manual_bad_raw_path_text[] = "/bad[raw]";
    const arc::net::UriView manual_bad_raw_path{
        .path = std::span<const char>{manual_bad_raw_path_text, sizeof(manual_bad_raw_path_text) - 1U},
    };
    expect(!arc::net::Uri::path_query(std::span(target), manual_bad_raw_path),
           "URI path query rejects manually constructed raw path char");

    constexpr char manual_bad_query_path[] = "/path";
    constexpr char manual_bad_query_text[] = "q=bad#frag";
    const arc::net::UriView manual_bad_query{
        .path = std::span<const char>{manual_bad_query_path, sizeof(manual_bad_query_path) - 1U},
        .query = std::span<const char>{manual_bad_query_text, sizeof(manual_bad_query_text) - 1U},
        .has_query = true,
    };
    expect(!arc::net::Uri::path_query(std::span(target), manual_bad_query),
           "URI path query rejects manually constructed query fragment delimiter");

    constexpr char manual_bad_escape_path[] = "/bad%escape";
    const arc::net::UriView manual_bad_escape{
        .path = std::span<const char>{manual_bad_escape_path, sizeof(manual_bad_escape_path) - 1U},
    };
    expect(!arc::net::Uri::path_query(std::span(target), manual_bad_escape),
           "URI path query rejects manually constructed bad escape");

    const auto future = arc::net::Uri::parse("http://[v1.arc-node]/path");
    expect(future.has_value() && future->host.size() == 11U &&
               std::memcmp(future->host.data(), "v1.arc-node", future->host.size()) == 0,
           "URI IPvFuture literal parses");

    const auto zone = arc::net::Uri::parse("http://[fe80::1%25wlan0]/path");
    expect(zone.has_value(), "URI IPv6 zone literal parses");
    const auto zone_endpoint = arc::net::Uri::endpoint(*zone, 80U);
    expect(zone_endpoint.has_value() && zone_endpoint->port == 80U, "URI IPv6 zone endpoint parses");
    std::array<char, 24> zone_host{};
    const auto zone_host_text = arc::net::Uri::copy_host(std::span(zone_host), *zone);
    expect(zone_host_text.has_value() &&
               std::memcmp(zone_host_text->data(), "fe80::1%wlan0", zone_host_text->size()) == 0 &&
               zone_host[zone_host_text->size()] == '\0',
           "URI host copy decodes IPv6 zone escape");

    const auto zone_alias = arc::net::Uri::parse("http://[fe80::1%25eth0%3A1]/path");
    expect(zone_alias.has_value(), "URI IPv6 zone pct-encoded colon parses");
    std::array<char, 24> zone_alias_host{};
    const auto zone_alias_endpoint = arc::net::Uri::endpoint(*zone_alias, 80U);
    const auto zone_alias_text = arc::net::Uri::copy_host(std::span(zone_alias_host), *zone_alias);
    expect(zone_alias_endpoint.has_value() && zone_alias_text.has_value() &&
               std::memcmp(zone_alias_text->data(), "fe80::1%eth0:1", zone_alias_text->size()) == 0 &&
               zone_alias_host[zone_alias_text->size()] == '\0',
           "URI host copy decodes IPv6 zone pct-encoded colon");

    const auto zone_slash = arc::net::Uri::parse("http://[fe80::1%25eth0%2F1]/path");
    expect(zone_slash.has_value() && !arc::net::Uri::endpoint(*zone_slash, 80U) &&
               !arc::net::Uri::copy_host(std::span(zone_alias_host), *zone_slash),
           "URI endpoint rejects IPv6 zone pct-encoded delimiter");

    const auto mapped = arc::net::Uri::parse("http://[::ffff:192.0.2.1]/path");
    expect(mapped.has_value(), "URI IPv4-mapped IPv6 literal parses");

    std::array<char, 32> decoded{};
    const auto text = arc::net::Uri::decode(std::span(decoded), "sample%201+ok", true);
    expect(text.has_value(), "URI decode succeeds");
    expect(std::memcmp(text->data(), "sample 1 ok", text->size()) == 0, "URI percent decode");

    std::array<char, 32> encoded{};
    const auto wire = arc::net::Uri::encode(std::span(encoded), "sample 1/ok?", true);
    expect(wire.has_value(), "URI encode succeeds");
    expect(std::memcmp(wire->data(), "sample+1%2Fok%3F", wire->size()) == 0, "URI percent encode");

    std::array<char, 2> small{};
    expect(!arc::net::Uri::decode(std::span(small), "abcd"), "URI decode capacity rejects");
    expect(!arc::net::Uri::copy_host(std::span(small), *uri), "URI copy host capacity rejects");
    expect(!arc::net::Uri::path_query(std::span(small), *uri), "URI path query capacity rejects");
    std::array<char, 4> exact_host{};
    expect(!arc::net::Uri::copy_host(std::span(exact_host), *defaulted),
           "URI copy host reserves space for nul terminator");
    expect(!arc::net::Uri::decode(std::span(decoded), "%xz"), "URI invalid escape rejects");
    expect(!arc::net::Uri::encode(std::span(small), "abc"), "URI encode capacity rejects");
    expect(!arc::net::Uri::parse("http://2001:db8::1/path"), "URI unbracketed IPv6 rejects");
    expect(!arc::net::Uri::parse("http://[node]/path"), "URI bracketed reg-name rejects");
    expect(!arc::net::Uri::parse("http://[node:bad]/path"), "URI invalid bracket literal rejects");
    expect(!arc::net::Uri::parse("http://[2001:db8::zz]/path"), "URI invalid IPv6 literal chars reject");
    expect(!arc::net::Uri::parse("http://[::::]/path"), "URI malformed IPv6 compression rejects");
    expect(!arc::net::Uri::parse("http://[fe80::1%25]/path"), "URI empty IPv6 zone rejects");
    expect(!arc::net::Uri::parse("http://[::ffff:999.0.2.1]/path"), "URI invalid IPv4 tail rejects");
    expect(!arc::net::Uri::parse("http://[1:2:3:4:5:6::192.0.2.1]/path"),
           "URI zero-width IPv6 compression with IPv4 tail rejects");
    expect(!arc::net::Uri::parse("http://[1:2:3:4:5:6:7:192.0.2.1]/path"),
           "URI oversized IPv6 with IPv4 tail rejects");
    expect(!arc::net::Uri::parse("http://node:/path"), "URI empty port rejects");
    const auto zero_port = arc::net::Uri::parse("http://node:0/path");
    expect(zero_port.has_value() && !arc::net::Uri::endpoint(*zero_port),
           "URI endpoint rejects explicit zero port");
    expect(!arc::net::Uri::parse("http://no[de/path"), "URI raw open bracket rejects");
    expect(!arc::net::Uri::parse("http://node]/path"), "URI raw close bracket rejects");
    expect(!arc::net::Uri::parse("http://no\\de/path"), "URI backslash host rejects");
    expect(!arc::net::Uri::parse("http://no|de/path"), "URI pipe host rejects");
    expect(!arc::net::Uri::parse("http://<node>/path"), "URI angle host rejects");
    expect(!arc::net::Uri::parse("http://node^bad/path"), "URI caret host rejects");
    expect(!arc::net::Uri::parse("http://bad[user@node/path"), "URI raw bracket userinfo rejects");
    expect(!arc::net::Uri::parse("http://bad\\user@node/path"), "URI raw backslash userinfo rejects");
    expect(!arc::net::Uri::parse("http://bad|user@node/path"), "URI raw pipe userinfo rejects");
    expect(!arc::net::Uri::parse("http://bad%00user@node/path"), "URI decoded nul userinfo rejects");
    expect(!arc::net::Uri::parse("http://bad%2Fuser@node/path"), "URI decoded slash userinfo rejects");
    expect(!arc::net::Uri::parse("http://bad%40user@node/path"), "URI decoded at userinfo rejects");
    const auto nul_host = arc::net::Uri::parse("http://node%00.local/path");
    expect(nul_host.has_value() && !arc::net::Uri::endpoint(*nul_host, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), *nul_host),
           "URI endpoint host rejects decoded nul");
    const auto control_host = arc::net::Uri::parse("http://node%0A.local/path");
    expect(control_host.has_value() && !arc::net::Uri::endpoint(*control_host, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), *control_host),
           "URI endpoint host rejects decoded control");
    const auto slash_host = arc::net::Uri::parse("http://node%2Fadmin.local/path");
    expect(slash_host.has_value() && !arc::net::Uri::endpoint(*slash_host, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), *slash_host),
           "URI endpoint host rejects decoded delimiter");
    const auto colon_host = arc::net::Uri::parse("http://node%3A443/path");
    expect(colon_host.has_value() && !arc::net::Uri::endpoint(*colon_host, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), *colon_host),
           "URI endpoint host rejects decoded colon in reg-name");
    const auto percent_host = arc::net::Uri::parse("http://node%25.local/path");
    expect(percent_host.has_value() && !arc::net::Uri::endpoint(*percent_host, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), *percent_host),
           "URI endpoint host rejects decoded percent in reg-name");

    constexpr char manual_authority[] = "node:443";
    constexpr char manual_host[] = "node:443";
    const arc::net::UriView manual_bad_host{
        .authority = std::span<const char>{manual_authority, sizeof(manual_authority) - 1U},
        .host = std::span<const char>{manual_host, sizeof(manual_host) - 1U},
    };
    expect(!arc::net::Uri::endpoint(manual_bad_host, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), manual_bad_host),
           "URI endpoint helpers reject manually constructed host delimiter");

    constexpr char manual_raw_authority[] = "node^bad";
    constexpr char manual_raw_host[] = "node^bad";
    const arc::net::UriView manual_raw_host_view{
        .authority = std::span<const char>{manual_raw_authority, sizeof(manual_raw_authority) - 1U},
        .host = std::span<const char>{manual_raw_host, sizeof(manual_raw_host) - 1U},
    };
    expect(!arc::net::Uri::endpoint(manual_raw_host_view, 80U) &&
               !arc::net::Uri::copy_host(std::span(decoded_host), manual_raw_host_view),
           "URI endpoint helpers reject manually constructed raw host char");

    expect(!arc::net::Uri::parse("http://node:70000/path"), "URI port range rejects");
    expect(!arc::net::Uri::parse("http://node:4295032831/path"), "URI wrapped port rejects");
    expect(!arc::net::Uri::parse("http://node/a b"), "URI literal space rejects");
    expect(!arc::net::Uri::parse("http://node/%xz"), "URI invalid path escape rejects");
    expect(!arc::net::Uri::parse("http://node/?q=%"), "URI short query escape rejects");
    expect(!arc::net::Uri::parse("http://node/#bad%q"), "URI invalid fragment escape rejects");
    expect(!arc::net::Uri::parse("http://node/[bad]"), "URI raw path brackets reject");
    expect(!arc::net::Uri::parse("http://node/\\bad"), "URI raw path backslash rejects");
    expect(!arc::net::Uri::parse("http://node/?q=<bad>"), "URI raw query angle brackets reject");
    expect(!arc::net::Uri::parse("http://node/#frag[bad]"), "URI raw fragment brackets reject");
    constexpr char embedded_nul[] = {'h', 't', 't', 'p', ':', '/', '/', 'n', 'o', 'd', 'e', '\0', 'x'};
    expect(!arc::net::Uri::parse(std::span<const char>{embedded_nul, sizeof(embedded_nul)}),
           "URI embedded nul rejects");
    expect(!arc::net::Uri::parse("1bad://node/path"), "URI invalid scheme rejects");
}

void test_http_client()
{
    expect(!arc::net::Http::https(nullptr), "HTTP HTTPS rejects null URL");
    expect(!arc::net::Http::https("http://node/api"), "HTTP HTTPS rejects http URL");
    expect(!arc::net::Http::https("/api"), "HTTP HTTPS rejects relative URL");
    expect(!arc::net::Http::https("https:/node/api"), "HTTP HTTPS rejects missing authority");
    expect(!arc::net::Http::https("https://node:0/api"), "HTTP HTTPS rejects zero port URL");
    expect(!arc::net::Http::https("https://node/%xz"), "HTTP HTTPS rejects invalid escaped URL");
    expect(!arc::net::Http::https("https://node%00.local/api"), "HTTP HTTPS rejects decoded nul host");
    expect(!arc::net::Http::https("https://node%0A.local/api"), "HTTP HTTPS rejects decoded control host");
    expect(!arc::net::Http::https("https://node%2Fadmin.local/api"),
           "HTTP HTTPS rejects decoded delimiter host");
    expect(!arc::net::Http::https("https://bad[user@node/api"), "HTTP HTTPS rejects invalid userinfo");
    expect(!arc::net::Http::https("https://bad%00user@node/api"), "HTTP HTTPS rejects decoded userinfo control");
    expect(!arc::net::Http::https("https://bad%2Fuser@node/api"),
           "HTTP HTTPS rejects decoded userinfo delimiter");

    auto client = arc::net::Http::https("HTTPS://node/api");
    expect(client.has_value() && client->native() != nullptr, "HTTP HTTPS accepts uppercase scheme");
    expect(client->method(HTTP_METHOD_POST) == ESP_OK, "HTTP method forwards");
    expect(client->url("http://node/plain") == ESP_ERR_INVALID_ARG,
           "HTTP HTTPS client rejects insecure URL reset");
    expect(client->url("https://node%2Fadmin.local/next") == ESP_ERR_INVALID_ARG,
           "HTTP HTTPS client rejects unsafe URL reset");
    expect(client->url("https://node/next") == ESP_OK, "HTTP url forwards");
    const std::array<std::uint8_t, 3> body{1U, 2U, 3U};
    expect(client->body(std::span(body)) == ESP_OK, "HTTP body forwards");
    expect(client->perform() == ESP_OK, "HTTP perform forwards");
    expect(client->open() == ESP_OK, "HTTP open forwards");
    expect(client->fetch().has_value(), "HTTP fetch forwards");
    std::array<std::uint8_t, 4> scratch{};
    expect(client->write(std::span(body)).has_value(), "HTTP write forwards");
    expect(client->read(std::span(scratch)).has_value(), "HTTP read forwards");
    expect(client->status().has_value() && *client->status() == 200, "HTTP status forwards");
    expect(client->length().has_value(), "HTTP length forwards");
    expect(client->close() == ESP_OK, "HTTP close forwards");

    const esp_http_client_config_t config{.url = "https://[2001:db8::1]/status"};
    auto ipv6 = arc::net::Http::https(config);
    expect(ipv6.has_value() && ipv6->native() != nullptr, "HTTP HTTPS accepts bracket IPv6 URL");

    auto moved = arc::net::Http::https("https://node/move");
    expect(moved.has_value(), "HTTP HTTPS movable client builds");
    auto moved_client = std::move(*moved);
    expect(moved_client.url("http://node/plain") == ESP_ERR_INVALID_ARG,
           "HTTP HTTPS move constructor preserves secure URL guard");
    arc::net::Http assigned;
    assigned = std::move(moved_client);
    expect(assigned.url("http://node/plain") == ESP_ERR_INVALID_ARG,
           "HTTP HTTPS move assignment preserves secure URL guard");

    const esp_http_client_config_t clear_config{.url = "http://node/plain"};
    auto clear = arc::net::Http::make(clear_config);
    expect(clear.has_value() && clear->url("http://node/next") == ESP_OK,
           "HTTP generic client keeps cleartext URL setter");
}

void test_uri_dial_helpers()
{
    std::array<char, 16> host{};
    expect(!arc::net::Tcp::dial(nullptr, std::span(host), 80U), "TCP URI dial rejects null URI");
    expect(!arc::net::Tcp::dial("/api", std::span(host), 80U), "TCP URI dial rejects relative URI");
    expect(!arc::net::Tcp::dial("http://node:70000/path", std::span(host), 80U),
           "TCP URI dial rejects invalid port");
    expect(!arc::net::Tcp::dial("http://node%00.local/path", std::span(host), 80U),
           "TCP URI dial rejects decoded nul host");
    expect(!arc::net::Tcp::dial("http://node%0A.local/path", std::span(host), 80U),
           "TCP URI dial rejects decoded control host");
    expect(!arc::net::Tcp::dial("http://node%2Fadmin.local/path", std::span(host), 80U),
           "TCP URI dial rejects decoded delimiter host");

    std::array<char, 2> small{};
    expect(!arc::net::Tcp::dial("http://node/path", std::span(small), 80U),
           "TCP URI dial rejects small host buffer");

    esp_tls_cfg_t cfg{};
    expect(!arc::net::Tls::dial(nullptr, std::span(host), cfg), "TLS URI dial rejects null URI");
    expect(!arc::net::Tls::dial("/api", std::span(host), cfg), "TLS URI dial rejects relative URI");
    expect(!arc::net::Tls::dial("http://node/path", std::span(host), cfg), "TLS URI dial rejects HTTP scheme");
    expect(!arc::net::Tls::dial("mqtt://node/path", std::span(host), cfg), "TLS URI dial rejects MQTT scheme");
    expect(!arc::net::Tls::dial("https://node%00.local/path", std::span(host), cfg),
           "TLS URI dial rejects decoded nul host");
    expect(!arc::net::Tls::dial("https://node%0A.local/path", std::span(host), cfg),
           "TLS URI dial rejects decoded control host");
    expect(!arc::net::Tls::dial("https://node%2Fadmin.local/path", std::span(host), cfg),
           "TLS URI dial rejects decoded delimiter host");
    expect(!arc::net::Tls::dial("https://node/path", std::span(small), cfg),
           "TLS URI dial rejects small host buffer");

    auto schemeless_tls = arc::net::Tls::dial("//node/path", std::span(host), cfg);
    expect(schemeless_tls.has_value() && schemeless_tls->native() != nullptr,
           "TLS URI dial accepts schemeless authority");
    auto wss_tls = arc::net::Tls::dial("wss://node/path", std::span(host), cfg);
    expect(wss_tls.has_value() && wss_tls->native() != nullptr, "TLS URI dial accepts secure stream scheme");
    auto mqtts_tls = arc::net::Tls::dial("mqtts://node/path", std::span(host), cfg);
    expect(mqtts_tls.has_value() && mqtts_tls->native() != nullptr, "TLS URI dial accepts secure MQTT scheme");
    auto raw_tls = arc::net::Tls::dial("tls://node/path", std::span(host), cfg);
    expect(raw_tls.has_value() && raw_tls->native() != nullptr, "TLS URI dial accepts raw TLS scheme");
    auto tls = arc::net::Tls::dial("https://node:8443/path", std::span(host), cfg);
    expect(tls.has_value() && tls->native() != nullptr, "TLS URI dial succeeds through stub");
    const std::array<std::uint8_t, 3> payload{1U, 2U, 3U};
    expect(tls->send(std::span(payload)).has_value(), "TLS send forwards");
    std::array<std::uint8_t, 4> scratch{};
    expect(tls->recv(std::span(scratch)).has_value(), "TLS recv forwards");
    expect(tls->avail().has_value(), "TLS avail forwards");
    expect(tls->sockfd().has_value() && *tls->sockfd() == 7, "TLS sockfd forwards");

    constexpr char pem[] = "-----BEGIN CERTIFICATE-----\narc\n-----END CERTIFICATE-----\n";
    auto with_ca = arc::net::Tls::dial_ca("https://node/path", std::span(host), pem);
    expect(with_ca.has_value() && with_ca->native() != nullptr, "TLS URI dial_ca succeeds through stub");
}

void test_codec_fuzzer_smoke()
{
    expect(LLVMFuzzerTestOneInput(nullptr, 0U) == 0, "codec fuzzer empty seed");

    constexpr std::array<std::uint8_t, 52> http_seed{
        'G',
        'E',
        'T',
        ' ',
        '/',
        'm',
        'e',
        't',
        'r',
        'i',
        'c',
        's',
        '?',
        'u',
        'n',
        'i',
        't',
        '=',
        'p',
        'h',
        ' ',
        'H',
        'T',
        'T',
        'P',
        '/',
        '1',
        '.',
        '1',
        '\r',
        '\n',
        'H',
        'o',
        's',
        't',
        ':',
        ' ',
        'n',
        'o',
        'd',
        'e',
        '\r',
        '\n',
        '\r',
        '\n',
        'h',
        't',
        't',
        'p',
        ':',
        '/',
        '/',
    };
    expect(LLVMFuzzerTestOneInput(http_seed.data(), http_seed.size()) == 0, "codec fuzzer HTTP seed");

    constexpr std::array<std::uint8_t, 33> uri_seed{
        'h',
        't',
        't',
        'p',
        's',
        ':',
        '/',
        '/',
        'n',
        'o',
        'd',
        'e',
        ':',
        '4',
        '4',
        '3',
        '/',
        'a',
        '%',
        '2',
        '0',
        'b',
        '?',
        'f',
        'l',
        'a',
        'g',
        '&',
        'x',
        '=',
        '%',
        '7',
        'E',
    };
    expect(LLVMFuzzerTestOneInput(uri_seed.data(), uri_seed.size()) == 0, "codec fuzzer URI seed");

    constexpr std::array<std::uint8_t, 12> binary_seed{0x10U, 0x0AU, 0x00U, 0x04U, 'M', 'Q',
                                                       'T', 'T', 0xffU, 0x83U, 0x00U, 0x00U};
    expect(LLVMFuzzerTestOneInput(binary_seed.data(), binary_seed.size()) == 0, "codec fuzzer binary seed");
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

    constexpr char query_raw[] = "GET /config?mode=tight&limit=5&flag&empty= HTTP/1.1\r\nHost: arc.local\r\n\r\n";
    const auto query_req = arc::net::HttpServer::parse(
        std::span<const char>{query_raw, sizeof(query_raw) - 1U},
        std::span(headers));
    expect(query_req.has_value(), "HTTP query request parses");
    const auto query_path = arc::net::HttpServer::path(*query_req);
    expect(query_path.size() == 7U && std::memcmp(query_path.data(), "/config", query_path.size()) == 0, "HTTP path strips query");
    expect(arc::net::HttpServer::query(*query_req).size() == 30U, "HTTP query view");
    const auto query_hit = arc::net::HttpRouter<ConfigGet, MetricsGet>::dispatch(*query_req, [&](auto route) {
        routed = route.id() == ConfigGet::id();
    });
    expect(query_hit && routed, "HTTP router ignores query string");
    const auto mode_arg = arc::net::HttpServer::find_query(*query_req, "mode");
    expect(mode_arg.has_value() && std::memcmp(mode_arg->data(), "tight", mode_arg->size()) == 0, "HTTP query param value");
    const auto limit_arg = arc::net::HttpServer::find_query(*query_req, "limit");
    expect(limit_arg.has_value() && std::memcmp(limit_arg->data(), "5", limit_arg->size()) == 0, "HTTP query numeric value");
    const auto flag_arg = arc::net::HttpServer::find_query(*query_req, "flag");
    expect(flag_arg.has_value() && flag_arg->empty(), "HTTP query bare flag");
    const auto empty_arg = arc::net::HttpServer::find_query(*query_req, "empty");
    expect(empty_arg.has_value() && empty_arg->empty(), "HTTP query empty value");
    expect(!arc::net::HttpServer::find_query(*query_req, "missing"), "HTTP query missing value");
    expect(!arc::net::HttpServer::find_query(*query_req, nullptr), "HTTP query null key rejects");

    constexpr char encoded_query_raw[] =
        "GET /config?mode=tight%20loop&label=A%2BB+C&bad=%xz HTTP/1.1\r\nHost: arc.local\r\n\r\n";
    const auto encoded_query_req = arc::net::HttpServer::parse(
        std::span<const char>{encoded_query_raw, sizeof(encoded_query_raw) - 1U},
        std::span(headers));
    expect(encoded_query_req.has_value(), "HTTP encoded query request parses");
    std::array<char, 16> decoded_query{};
    const auto decoded_mode = arc::net::HttpServer::find_query(*encoded_query_req, "mode", std::span(decoded_query));
    expect(decoded_mode.has_value() && decoded_mode->size() == 10U &&
               std::memcmp(decoded_mode->data(), "tight loop", decoded_mode->size()) == 0,
           "HTTP query value decodes percent escapes");
    const auto decoded_label = arc::net::HttpServer::find_query(*encoded_query_req, "label", std::span(decoded_query));
    expect(decoded_label.has_value() && decoded_label->size() == 5U &&
               std::memcmp(decoded_label->data(), "A+B C", decoded_label->size()) == 0,
           "HTTP query value decodes form spaces");
    std::array<char, 4> small_query{};
    expect(!arc::net::HttpServer::find_query(*encoded_query_req, "mode", std::span(small_query)),
           "HTTP query decode capacity rejects");
    expect(!arc::net::HttpServer::find_query(*encoded_query_req, "bad", std::span(decoded_query)),
           "HTTP query decode invalid escape rejects");
    expect(!arc::net::HttpServer::find_query(*encoded_query_req, "missing", std::span(decoded_query)),
           "HTTP decoded query missing value");

    std::array<char, 128> response{};
    constexpr char body[] = "ok";
    const auto encoded = arc::net::HttpServer::text_response(
        std::span(response),
        200,
        "OK",
        std::span<const char>{body, sizeof(body) - 1U});
    expect(encoded.has_value(), "HTTP response encodes");
    expect(std::memcmp(encoded->data(), "HTTP/1.1 200 OK\r\n", 17U) == 0, "HTTP response status");

    const std::array<arc::net::JsonField, 6> json_fields{{
        {.name = "mode", .value = arc::net::Json::str("safe\"loop\n")},
        {.name = "enabled", .value = arc::net::Json::boolean(true)},
        {.name = "count", .value = arc::net::Json::u64(3U)},
        {.name = "delta", .value = arc::net::Json::i64(-2)},
        {.name = "samples", .value = arc::net::Json::raw("[1,2]")},
        {.name = "missing", .value = arc::net::Json::nil()},
    }};
    std::array<char, 256> json_response{};
    const auto json = arc::net::HttpServer::json_response(
        std::span(json_response),
        200,
        "OK",
        std::span(json_fields));
    expect(json.has_value(), "HTTP JSON response encodes");
    expect(std::strstr(json_response.data(), "Content-Type: application/json\r\n") != nullptr, "HTTP JSON content type");
    expect(std::strstr(json_response.data(), "\"mode\":\"safe\\\"loop\\n\"") != nullptr, "HTTP JSON escaped string");
    expect(std::strstr(json_response.data(), "\"enabled\":true") != nullptr, "HTTP JSON bool");
    expect(std::strstr(json_response.data(), "\"delta\":-2") != nullptr, "HTTP JSON signed value");
    expect(std::strstr(json_response.data(), "\"samples\":[1,2]") != nullptr, "HTTP JSON raw value");
    expect(std::strstr(json_response.data(), "\"missing\":null") != nullptr, "HTTP JSON null value");

    std::array<char, 64> json_body{};
    const auto body_only = arc::net::HttpServer::json_body(std::span(json_body), std::span(json_fields).first(2U));
    expect(body_only.has_value(), "HTTP JSON body encodes");
    expect(std::memcmp(body_only->data(), "{\"mode\":\"safe\\\"loop\\n\",\"enabled\":true}", body_only->size()) == 0, "HTTP JSON body text");

    const std::array<arc::net::JsonField, 1> bad_json{{{.name = "bad", .value = arc::net::Json::raw("")}}};
    expect(!arc::net::HttpServer::json_body(std::span(json_body), std::span(bad_json)), "HTTP JSON raw empty rejects");

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
    expect(
        !arc::net::HttpServer::json_response(std::span(small_response), 200, "OK", std::span(json_fields)),
        "HTTP JSON response buffer cap rejects");
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
    expect(dma.storage_bytes() == padded && dma.storage().size() == padded,
           "DMA cap buffer exposes padded storage");
    expect(arc::Cache::from_device(dma) == ESP_OK && esp_cache_last_msync_bytes == padded &&
               (esp_cache_last_msync_flags & arc::Cache::unaligned) == 0,
           "DMA cap buffer cache sync uses padded storage");

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

    using Beam = arc::dsp::Beamform<float, 2>;
    const std::array<float, 5> mic0{1.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    const std::array<float, 5> mic1{0.0F, 1.0F, 0.0F, 0.0F, 0.0F};
    std::array<float, 4> beam_out{};
    const Beam::Inputs beam_inputs{std::span<const float>{mic0}, std::span<const float>{mic1}};
    const auto beamed = Beam::delay_sum(
        beam_inputs,
        {.delay_samples = {0U, 1U}, .gain = {1.0F, 1.0F}, .scale = 0.5F},
        std::span(beam_out));
    expect(beamed.has_value() && *beamed == beam_out.size() && beam_out[0] == 1.0F, "DSP Beamform delay sum");

    const auto lag = Beam::lag_xcorr(std::span<const float>{mic0}, std::span<const float>{mic1}, 2U);
    expect(lag.has_value() && lag->lag_samples == 1, "DSP Beamform xcorr lag");

    std::array<arc::simd::ComplexF32, 4> ref_fft{{
        {.re = 1.0F},
        {},
        {},
        {},
    }};
    std::array<arc::simd::ComplexF32, 4> delayed_fft{{
        {},
        {.re = 1.0F},
        {},
        {},
    }};
    std::array<arc::simd::ComplexF32, 4> gcc{};
    const auto estimate = Beam::gcc_phat(std::span(ref_fft), std::span(delayed_fft), std::span(gcc), 1U, 48'000.0F, 0.05F);
    expect(estimate.has_value() && estimate->confidence > 0.0F, "DSP Beamform GCC-PHAT estimate");

    using Echo = arc::dsp::Aec<float, 2>;
    Echo::State echo{};
    const std::array<float, 4> far{1.0F, 0.0F, 0.0F, 0.0F};
    const std::array<float, 4> mic{0.5F, 0.0F, 0.0F, 0.0F};
    std::array<float, 4> clean{};
    const auto cancelled = Echo::run(std::span(clean), echo, std::span<const float>{far}, std::span<const float>{mic});
    expect(cancelled.has_value() && *cancelled == clean.size(), "DSP AEC run");
    expect(echo.taps[0] > 0.0F, "DSP AEC adapts tap");
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

    arc::DualFoc<arc::Foc<Bridge, Scope, Encoder, float>, arc::Foc<Bridge, Scope, Encoder, float>> dual{};
    dual.a.config = foc.config;
    dual.b.config = foc.config;
    dual.a.target.write({.d = 0.0F, .q = 0.5F, .bus = 2.0F});
    dual.b.target.write({.d = 0.0F, .q = -0.5F, .bus = 2.0F});
    const auto dual_out = dual.step(
        {
            .a = {.current = {.a = 0.0F, .b = 0.0F, .c = 0.0F}, .sin_theta = 0.0F, .cos_theta = 1.0F},
            .b = {.current = {.a = 0.0F, .b = 0.0F, .c = 0.0F}, .sin_theta = 0.0F, .cos_theta = 1.0F},
        },
        0.00005F);
    expect(dual_out.a.duty.b > dual_out.a.duty.c && dual_out.b.duty.b < dual_out.b.duty.c, "Dual FOC outputs");

    struct SyncA {
        [[nodiscard]] static constexpr std::uint32_t frequency() noexcept
        {
            return 20'000U;
        }
    };
    struct SyncB {
        [[nodiscard]] static constexpr std::uint32_t frequency() noexcept
        {
            return 20'000U;
        }
    };
    static_assert(arc::BridgeCarrierSync<SyncA, SyncB>::same_frequency());
    const auto sync = arc::BridgeCarrierSync<SyncA, SyncB>::plan(arc::FocCarrierSample::center, 4U);
    expect(sync.valid() && sync.phase_ticks == 4U, "FOC carrier sync plan");
    struct AdcTask {};
    const arc::FocEtmCurrentTrigger<SyncA, AdcTask> trigger{
        .sample = arc::FocCarrierSample::peak,
        .phase_ticks = 2U,
    };
    expect(trigger.plan().current_sample == arc::FocCarrierSample::peak && trigger.plan().phase_ticks == 2U,
           "FOC ETM trigger plan");

    struct SpiEncoder {};
    struct PcntEncoder {};
    arc::FocEncoderFusion<SpiEncoder, PcntEncoder, arc::dsp::Kalman<float, 2, 1>, float> fusion{};
    const auto fused = fusion.step({
        .absolute_angle = 1.25F,
        .incremental_angle = 1.0F,
        .velocity = 0.5F,
        .absolute_valid = true,
    });
    expect(fused == 1.25F && fusion.filter.x(0, 0) == 1.25F && fusion.filter.x(1, 0) == 0.5F,
           "FOC encoder fusion");

    std::array<arc::MotionStep<3>, 8> steps{};
    const auto plan = arc::MotionPlan<3>::line(
        std::span(steps),
        {.delta = {4, 2, 1}, .ticks_step = 10U});
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

void test_ml_tensor()
{
    arc::ml::Tensor<float, 1, 4> input{};
    input[0] = 1.0F;
    input[1] = -2.0F;
    input[2] = 3.0F;
    input[3] = 4.0F;

    static_assert(decltype(input)::rank == 2U);
    static_assert(decltype(input)::size == 4U);
    static_assert(decltype(input)::shape[1] == 4U);

    const std::array<float, 8> weights{
        1.0F,
        1.0F,
        1.0F,
        1.0F,
        1.0F,
        -1.0F,
        0.0F,
        0.5F,
    };
    const std::array<float, 2> bias{0.5F, -1.0F};
    arc::ml::Tensor<float, 2> output{};
    const arc::ml::Dense<4, 2> dense{
        .weights = std::span<const float, 8>{weights},
        .bias = std::span<const float, 2>{bias},
    };
    expect(dense.eval(std::span<const float, 4>{input.values}, output.span()).has_value(), "ML dense eval");
    expect(output[0] == 6.5F && output[1] == 4.0F, "ML dense output");

    output[1] = -3.0F;
    arc::ml::relu(output.span());
    expect(output[1] == 0.0F && arc::ml::argmax(std::span<const float, 2>{output.values}) == 0U, "ML relu argmax");

    const std::array<std::int8_t, 4> qw{1, 2, 3, 4};
    const std::array<std::int32_t, 1> qb{0};
    const std::array<std::int8_t, 4> qx{1, 1, 1, 1};
    std::array<std::int8_t, 1> qy{};
    const arc::ml::QuantDenseS8<4, 1> qdense{
        .weights = std::span<const std::int8_t, 4>{qw},
        .bias = std::span<const std::int32_t, 1>{qb},
        .multiplier = 1,
        .shift = 0,
    };
    expect(qdense.eval(std::span<const std::int8_t, 4>{qx}, std::span<std::int8_t, 1>{qy}).has_value(),
           "ML quant dense eval");
    expect(qy[0] == 10, "ML quant dense output");
}

void test_simd_ml_pipeline()
{
    const std::array<std::int8_t, 16> lhs{1, 2, 3, 4, 5, 6, 7, 8, -1, -2, -3, -4, -5, -6, -7, -8};
    const std::array<std::int8_t, 16> rhs{1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1};
    expect(arc::simd::dot_s8(std::span(lhs), std::span(rhs)) == 72, "SIMD int8 dot");
    expect(arc::simd::pie::mac_s8(0, arc::simd::load_s8x16(lhs.data()), arc::simd::load_s8x16(rhs.data())) == 72,
           "PIE int8 MAC wrapper");
    expect(std::string_view{arc::simd::pie::MacS8x16::instruction} == "ee.mac.s8", "PIE int8 wrapper name");

    const std::array<std::uint8_t, 4> white_yuv{235, 128, 235, 128};
    std::array<std::uint16_t, 2> rgb{};
    const auto pixels = arc::simd::Rgb565::from_yuv422(std::span(white_yuv), std::span(rgb));
    expect(pixels.has_value() && *pixels == 2U && rgb[0] == 0xffffU && rgb[1] == 0xffffU, "YUV422 RGB565 conversion");

    std::array<arc::simd::ComplexF32, 4> spectrum{
        arc::simd::ComplexF32{.re = 1.0F},
        arc::simd::ComplexF32{},
        arc::simd::ComplexF32{},
        arc::simd::ComplexF32{},
    };
    expect(arc::simd::fft_radix2(std::span(spectrum)).has_value(), "SIMD FFT runs");
    expect(near(spectrum[0].re, 1.0F) && near(spectrum[1].re, 1.0F) && near(spectrum[2].re, 1.0F), "SIMD FFT impulse");

    const std::array<std::int8_t, 9> conv_in{1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::array<std::int8_t, 4> conv_w{1, 1, 1, 1};
    const std::array<std::int32_t, 1> conv_b{0};
    std::array<std::int8_t, arc::ml::Conv2dS8<3, 3, 1, 1, 2, 2>::output_size> conv_out{};
    const arc::ml::Conv2dS8<3, 3, 1, 1, 2, 2> conv{
        .weights = std::span<const std::int8_t, 4>{conv_w},
        .bias = std::span<const std::int32_t, 1>{conv_b},
    };
    const arc::MmuSpan<std::int8_t> mapped_conv{
        .region = {},
        .values = std::span<const std::int8_t>{conv_w},
    };
    const auto mapped_weights = arc::ml::mapped_weights<std::int8_t, 4>(mapped_conv);
    expect(mapped_weights.has_value() && mapped_weights->data() == conv_w.data(), "ML MmuSpan mapped weights");
    expect(conv.eval(std::span<const std::int8_t, 9>{conv_in}, std::span(conv_out)).has_value(), "Conv2D S8 eval");
    expect(conv_out[0] == 12 && conv_out[1] == 16 && conv_out[2] == 24 && conv_out[3] == 28, "Conv2D S8 output");

    const std::array<std::int8_t, 8> depth_in{1, 2, 3, 4, 5, 6, 7, 8};
    const std::array<std::int8_t, 2> depth_w{1, 2};
    const std::array<std::int32_t, 2> depth_b{0, 0};
    std::array<std::int8_t, arc::ml::DepthwiseConv2dS8<2, 2, 2, 1, 1>::output_size> depth_out{};
    const arc::ml::DepthwiseConv2dS8<2, 2, 2, 1, 1> depth{
        .weights = std::span<const std::int8_t, 2>{depth_w},
        .bias = std::span<const std::int32_t, 2>{depth_b},
    };
    expect(depth.eval(std::span<const std::int8_t, 8>{depth_in}, std::span(depth_out)).has_value(),
           "Depthwise Conv2D S8 eval");
    expect(depth_out[0] == 1 && depth_out[1] == 4 && depth_out[7] == 16, "Depthwise Conv2D S8 output");

    const std::array<std::int8_t, 4> pool_in{1, 4, 2, 3};
    std::array<std::int8_t, arc::ml::MaxPool2d<std::int8_t, 2, 2, 1, 2, 2>::output_size> pool_out{};
    expect(arc::ml::MaxPool2d<std::int8_t, 2, 2, 1, 2, 2>::eval(std::span(pool_in), std::span(pool_out)).has_value(),
           "MaxPool eval");
    expect(pool_out[0] == 4, "MaxPool output");

    struct TinyGraph {
        using Context = std::uint32_t;

        static arc::Status run(Context& context) noexcept
        {
            ++context;
            return arc::ok();
        }
    };
    std::uint32_t graph_context{};
    arc::ml::Core1Inference<TinyGraph>::loop(&graph_context);
    expect(graph_context == 1U, "ML Core1 inference program");
}

void test_csi_hotpatch()
{
    struct CsiPolicy {
        static esp_err_t start(
            arc::net::CsiConfig,
            const arc::net::CsiCallback callback,
            void* const user) noexcept
        {
            csi_policy_started = true;
            csi_policy_callback = callback;
            csi_policy_user = user;
            return ESP_OK;
        }

        static esp_err_t stop() noexcept
        {
            csi_policy_started = false;
            csi_policy_callback = nullptr;
            csi_policy_user = nullptr;
            return ESP_OK;
        }
    };

    std::size_t csi_callbacks{};
    const auto csi_callback = [](void* user, const arc::net::CsiRawView& raw) noexcept {
        auto* const count = static_cast<std::size_t*>(user);
        *count += raw.iq.size();
    };
    expect(arc::net::CsiRx<CsiPolicy>::start({}, csi_callback, &csi_callbacks).has_value(), "CSI policy start");

    const arc::net::CsiMeta meta{
        .mac = {1U, 2U, 3U, 4U, 5U, 6U},
        .rssi = -40,
        .rate = 1U,
        .channel = 6U,
        .timestamp_us = 1234U,
    };
    const std::array<std::int8_t, 8> raw_iq{3, 4, 0, 5, -3, 4, -4, 3};
    csi_policy_callback(csi_policy_user, {.meta = meta, .iq = std::span<const std::int8_t>{raw_iq}});
    expect(csi_callbacks == raw_iq.size(), "CSI raw callback hook");

    auto csi_frame = arc::net::Csi::frame<8>(meta, std::span<const std::int8_t>{raw_iq});
    expect(csi_frame.has_value() && csi_frame->used == 4U, "CSI frame parse");
    const auto stats = arc::net::Csi::stats(*csi_frame);
    expect(stats.bins == 4U && near(stats.amplitude_mean, 5.0F), "CSI RF stats");

    arc::Spsc<arc::net::CsiFrame<8>, 4> csi_queue{};
    expect(arc::net::Csi::push(csi_queue, meta, std::span<const std::int8_t>{raw_iq}).has_value(), "CSI SPSC push");
    arc::net::CsiFrame<8> popped{};
    expect(csi_queue.try_pop(popped) && popped.meta.channel == 6U, "CSI SPSC pop");

    std::array<float, 3> features{};
    expect(arc::net::Csi::features(*csi_frame, std::span<float, 3>{features}).has_value(), "CSI feature vector");
    std::array<std::int8_t, 3> quantized{};
    expect(arc::net::Csi::quantize(std::span<const float, 3>{features}, std::span<std::int8_t, 3>{quantized}, 1.0F).has_value(),
           "CSI quantize RF tensor");
    const std::array<std::int8_t, 6> weights{1, 0, 0, -1, 0, 0};
    const std::array<std::int32_t, 2> bias{0, 0};
    std::array<std::int8_t, 2> logits{};
    const arc::ml::QuantDenseS8<3, 2> classifier{
        .weights = std::span<const std::int8_t, 6>{weights},
        .bias = std::span<const std::int32_t, 2>{bias},
        .multiplier = 1,
        .shift = 0,
    };
    expect(classifier.eval(std::span<const std::int8_t, 3>{quantized}, std::span<std::int8_t, 2>{logits}).has_value(),
           "CSI semantic presence classifier");
    expect(logits[0] > logits[1], "CSI classifier output");
    expect(arc::net::CsiRx<CsiPolicy>::stop().has_value() && !csi_policy_started, "CSI policy stop");

    struct PatchHeap {
        static void* allocate(const std::size_t bytes) noexcept
        {
            return std::malloc(bytes);
        }

        static void release(void* const pointer) noexcept
        {
            std::free(pointer);
        }

        static esp_err_t sync_code(void*, std::size_t) noexcept
        {
            ++hotpatch_syncs;
            return ESP_OK;
        }
    };

    const std::array<std::uint8_t, 4> payload{0xaaU, 0xbbU, 0xccU, 0xddU};
    {
        auto image = arc::HotPatch::load<PatchHeap>(std::span<const std::uint8_t>{payload}, 2U);
        expect(image.has_value() && image->entry() == static_cast<std::uint8_t*>(image->block.memory) + 2U,
               "HotPatch executable payload load");
    }

    struct PatchPolicy {
        static esp_err_t lock_code(arc::CacheRegion) noexcept
        {
            ++hotpatch_locks;
            return ESP_OK;
        }

        static esp_err_t store_instruction(void* const target, const std::uint32_t instruction) noexcept
        {
            ++hotpatch_stores;
            __atomic_store_n(static_cast<std::uint32_t*>(target), instruction, __ATOMIC_RELEASE);
            return ESP_OK;
        }

        static esp_err_t sync_code(void*, std::size_t) noexcept
        {
            ++hotpatch_syncs;
            return ESP_OK;
        }

        static esp_err_t unlock_all() noexcept
        {
            ++hotpatch_unlocks;
            return ESP_OK;
        }
    };

    std::uint32_t target_instruction{};
    const std::uint32_t replacement_instruction{0x00abcdefU};
    const auto patched = arc::HotPatch::install<PatchPolicy, arc::Mask<0U>>(
        {.target = &target_instruction, .replacement = reinterpret_cast<const void*>(0x40001000U), .instruction = replacement_instruction});
    expect(patched.has_value() && target_instruction == replacement_instruction, "HotPatch atomic detour install");
    expect(hotpatch_locks == 1U && hotpatch_stores == 1U && hotpatch_unlocks == 1U, "HotPatch policy hooks");
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

    struct TeePolicy {
        static esp_err_t configure(const std::span<const arc::WorldRegion> regions) noexcept
        {
            tee_regions = regions.size();
            return ESP_OK;
        }
        static esp_err_t core_world(std::uint32_t, arc::World) noexcept
        {
            return ESP_OK;
        }
        static esp_err_t peripheral_world(const std::uint32_t, const arc::World world) noexcept
        {
            if (world == arc::World::trusted) {
                ++tee_trusted_peripherals;
            } else {
                ++tee_untrusted_peripherals;
            }
            return ESP_OK;
        }
    };
    const std::array worlds{
        arc::WorldRegion{
            .base = frame.data(),
            .bytes = frame.size(),
            .owner = arc::World::trusted,
            .trusted = arc::PmsAccess::read_write,
            .untrusted = arc::PmsAccess::none,
        },
    };
    const std::array trusted_peripherals{1U, 2U};
    const std::array untrusted_peripherals{3U};
    expect(arc::WorldGuard<TeePolicy>::apply(
               {
                   .regions = std::span<const arc::WorldRegion>{worlds},
                   .trusted_peripherals = std::span<const std::uint32_t>{trusted_peripherals},
                   .untrusted_peripherals = std::span<const std::uint32_t>{untrusted_peripherals},
               })
               .has_value(),
           "TEE plan apply");
    expect(tee_regions == 1U && tee_trusted_peripherals == 2U && tee_untrusted_peripherals == 1U, "TEE world split");

    arc::TrustedObject<arc::TeeAsset::crypto, std::uint32_t> trusted_key{};
    trusted_key.get() = 0xCAFEU;
    expect(trusted_key.get() == 0xCAFEU, "TEE trusted object");

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

    using Fleet = arc::net::DistributedRcu<Record, 3>;
    arc::TimeSync fleet_clock{};
    Fleet master{20U};
    master.write({.seq = 7U, .value = 42U});
    const auto frame = master.transmit(schedule, fleet_clock, 1'100);
    expect(frame.has_value() && frame->node_id == 20U, "DistributedRcu TDMA transmit");

    const auto wire = Fleet::bytes(*frame);
    const auto decoded = Fleet::parse(wire);
    expect(decoded.has_value() && decoded->value.value == 42U, "DistributedRcu wire parse");

    Fleet slave{10U};
    expect(slave.assign(0U, 20U).has_value(), "DistributedRcu assign remote");
    const auto slot = slave.ingest(*decoded, schedule, 1'120);
    expect(slot.has_value() && *slot == 0U, "DistributedRcu ingest slot");
    const auto remote = slave.read_remote(20U);
    expect(remote.has_value() && (*remote)->value == 42U, "DistributedRcu remote RCU read");
    expect(slave.stale(20U, 1'400, 100), "DistributedRcu stale detection");

    arc::dsp::Kalman<float, 1, 1> reckon{};
    reckon.x(0, 0) = 2.0F;
    const auto predicted = arc::net::DeadReckoning::predict_stale(
        reckon,
        1'400,
        1'120,
        100,
        arc::dsp::Matrix<float, 1, 1>{{2.0F}},
        arc::dsp::Matrix<float, 1, 1>{{0.0F}});
    expect(predicted && reckon.x(0, 0) == 4.0F, "DeadReckoning Kalman fallback");
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

void test_text()
{
    std::array<char, 96> out{};
    arc::Text text{std::span(out)};

    expect(text.empty() && text.cap() == out.size(), "Text starts empty");
    expect(text.append("adc="), "Text append literal");
    expect(text.u32(42U), "Text u32");
    expect(text.push(','), "Text push");
    expect(text.i32(-7), "Text i32");
    expect(text.append(",pc=0x"), "Text append second literal");
    expect(text.hex(0x1afU, 4U), "Text hex width");
    expect(text.append(",name=\""), "Text append json prefix");
    expect(text.json("core\"loop\n\x01"), "Text json escape");
    expect(text.push('"'), "Text close quote");

    const auto view = text.view();
    expect(view == "adc=42,-7,pc=0x01af,name=\"core\\\"loop\\n\\u0001\"", "Text formatted output");
    expect(text.done().has_value() && text.done()->size() == view.size(), "Text done span");
    text.clear();
    expect(text.empty(), "Text clear");

    std::array<char, 4> small{};
    arc::Text tiny{std::span(small)};
    expect(!tiny.append("toolong"), "Text rejects oversized append");
    expect(tiny.empty(), "Text oversized append is not partial");
    expect(tiny.append("1234") && tiny.full(), "Text fills exactly");
    expect(!tiny.push('5'), "Text rejects full push");

    arc::Text empty{};
    expect(empty.view().empty() && !empty.push('x'), "Text default storage is inert");

    std::array<char, 2> signed_small{};
    arc::Text negative{std::span(signed_small)};
    expect(!negative.i32(-123) && negative.empty(), "Text failed signed write rolls back");

    std::array<char, 4> json_small{};
    arc::Text escaped{std::span(json_small)};
    expect(!escaped.json("ab\nc") && escaped.empty(), "Text failed json write rolls back");
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

    std::array<char, 256> escaped{};
    const auto escaped_event = arc::TraceEventWriter::json_event(std::span(escaped), event, false, "core\"loop\n");
    expect(escaped_event.has_value(), "TraceEvent escaped event");
    expect(std::strstr(escaped.data(), "\"name\":\"core\\\"loop\\n\"") != nullptr, "TraceEvent escaped name");

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

    arc::PostmortemFaultFrame fault{
        .cause = 29U,
        .pc = 0x1000U,
        .ps = 0x20U,
        .sp = 0x2000U,
        .a0 = 0x3000U,
        .excvaddr = 0x4000U,
        .core = 1U,
        .frames = 2U,
        .trace = {0x1000U, 0x1010U},
    };
    Dump::capture_fault(fault);
    expect(Dump::has_fault(), "Postmortem fault present");
    expect(Dump::fault().cause == 29U && Dump::fault().trace[1] == 0x1010U, "Postmortem fault frame");
    Dump::clear_fault();
    expect(!Dump::has_fault(), "Postmortem fault clear");
}

void test_timesync()
{
    arc::TimeSync sync{};
    const auto first = sync.discipline(
        arc::TimeSyncSample{
            .local_tx = 1'000,
            .remote_rx = 1'120,
            .remote_tx = 1'140,
            .local_rx = 1'040,
        },
        arc::TimeSyncConfig{.kp_shift = 0, .ki_shift = 8, .step_max = 1'000, .integral_max = 1'000});
    expect(first.raw_offset == 110, "TimeSync raw offset");
    expect(first.filt_offset == 110, "TimeSync correction");
    expect(first.delay == 20, "TimeSync delay");
    expect(sync.to_remote(1'000) == 1'110, "TimeSync local to remote");
    expect(sync.to_local(1'110) == 1'000, "TimeSync remote to local");

    const auto second = sync.discipline(
        arc::TimeSyncSample{
            .local_tx = 2'000,
            .remote_rx = 2'120,
            .remote_tx = 2'120,
            .local_rx = 2'000,
        },
        arc::TimeSyncConfig{.kp_shift = 1, .ki_shift = 8, .step_max = 5, .integral_max = 1'000});
    expect(second.correction == 5, "TimeSync correction clamp");
    expect(second.filt_offset == 115, "TimeSync filtered offset");

    const auto hw = sync.discipline_hw(
        {
            .local_tx = 4000,
            .remote_rx = 8000,
            .remote_tx = 8400,
            .local_rx = 4800,
            .local_shift = 2,
            .remote_shift = 3,
        },
        arc::TimeSyncConfig{.kp_shift = 1, .ki_shift = 4, .step_max = 1000, .integral_max = 1000});
    expect(hw.samples == 3U, "TimeSync hardware timestamp sample count");

    arc::PtpClock ptp{};
    const auto ptp_stats = ptp.discipline(
        arc::PtpSample{
            .origin_ns = 1'000'000,
            .ingress_ns = 1'000'120,
            .egress_ns = 1'000'140,
            .receive_ns = 1'000'040,
        },
        arc::PtpConfig{.kp_shift = 0, .ki_shift = 8, .step_max = 1'000, .integral_max = 1'000});
    expect(ptp_stats.raw_offset == 110, "PTP raw offset");
    expect(ptp_stats.filt_offset == 110, "PTP filtered offset");
    expect(ptp.to_master(1'000'000) == 1'000'110, "PTP local to grandmaster");
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
    static std::uint32_t token{0xA5U};
    expect((arc::BareCore<BareProgram, MockBarePolicy>::boot(stack, &token).has_value()), "BareCore boot");
    expect(bare_core_boots == 1U && MockBarePolicy::config.stack.size() == stack.size(), "BareCore policy handoff");
    expect(MockBarePolicy::context == &token, "BareCore context");

    arc::DmaChain<1> input{};
    arc::DmaChain<1> output{};
    std::array<std::uint8_t, 16> in_bytes{};
    std::array<std::uint8_t, 16> out_bytes{};
    input.bind(0, in_bytes);
    output.bind(0, out_bytes, true);
    expect((arc::crypto_submit<1, 1, MockCryptoDmaPolicy>(
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
    static_assert(arc::net::ByteStream<arc::net::AnyStream>);

    Mock io{};
    const std::array<std::uint8_t, 3> payload{1U, 2U, 3U};
    auto erased = arc::net::AnyStream::bind(io);
    expect(static_cast<bool>(erased), "AnyStream binds byte stream");
    expect(arc::net::Stream::write(erased, std::span(payload)).has_value(), "AnyStream write forwards");
    expect(io.tx_pos == 3U && io.tx[2] == 3U, "AnyStream write bytes");
    io.tx_pos = 0U;

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
    const auto frame = arc::net::Stream::read_frame16(erased, std::span(out));
    expect(frame.has_value() && *frame == 3U, "Stream frame read");
    expect((out == std::array<std::uint8_t, 4>{7U, 8U, 9U, 0U}), "Stream frame payload");

    arc::net::AnyStream empty{};
    expect(!empty.send_all(payload.data(), payload.size()), "AnyStream unbound send rejects");

    io.rx = {0U, 5U, 1U, 2U, 3U, 4U, 5U};
    io.rx_pos = 0U;
    io.rx_len = 7U;
    expect(!arc::net::Stream::read_frame16(io, std::span(out)), "Stream oversized frame rejects");
}

void test_pipeline_usb_ulp()
{
    arc::DmaChain<2> camera{};
    arc::DmaChain<2> display{};
    std::array<std::uint8_t, 8> camera_bytes{};
    std::array<std::uint8_t, 8> display_bytes{};
    camera.bind(0, std::span(camera_bytes).first(4));
    camera.bind(1, std::span(camera_bytes).subspan(4), true);
    display.bind(0, std::span(display_bytes).first(4));
    display.bind(1, std::span(display_bytes).subspan(4), true);

    arc::Pipeline<2> pipe{};
    expect(pipe.bind(0, arc::endpoint(camera)).has_value(), "Pipeline source bind");
    expect(pipe.bind(1, arc::endpoint(display)).has_value(), "Pipeline sink bind");
    expect(pipe.link_linear().has_value(), "Pipeline linear link");
    expect(camera.tail()->next == display.head() && display.tail()->next == nullptr, "Pipeline descriptor chain");

    std::array<std::uint8_t, 16> frame{};
    arc::DmaChain<2> rows{};
    const arc::Dma2dWindow crop{.x_bytes = 1U, .y = 1U, .width_bytes = 2U, .height = 2U, .stride_bytes = 4U};
    expect(arc::bind_rows(rows, std::span(frame), crop).has_value(), "Pipeline 2D row bind");
    expect(rows.desc[0].buffer == frame.data() + 5U && rows.desc[1].buffer == frame.data() + 9U, "Pipeline 2D offsets");
    expect(rows.desc[0].length == 2U && rows.desc[1].next == nullptr, "Pipeline 2D lengths");

    const std::array<std::uint8_t, 2> payload{0xaaU, 0x55U};
    std::array<std::uint8_t, 32> packet{};
    const auto rtp = arc::net::Rtp::packet(std::span(packet), std::span(payload), 7U, 0x01020304U, 0xaabbccddU, 97U, true);
    expect(rtp.has_value() && rtp->size() == 14U, "RTP packet size");
    expect(packet[1] == 0xe1U && packet[2] == 0U && packet[3] == 7U && packet[12] == 0xaaU, "RTP packet layout");

    std::array<std::uint8_t, 96> part{};
    const auto mjpeg = arc::net::Mjpeg::part(std::span(part), std::span(payload), "cam");
    expect(mjpeg.has_value() && mjpeg->size() > payload.size(), "MJPEG part");
    expect(part[0] == '-' && part[2] == 'c' && part[mjpeg->size() - 2U] == '\r', "MJPEG boundary layout");

    constexpr auto bulk = arc::usb::Bulk<0x01U, 0x82U>::descriptors();
    static_assert(bulk.size() == 32U);
    static_assert(bulk[1] == static_cast<std::uint8_t>(arc::usb::DescriptorType::configuration));
    static_assert(bulk[2] == 32U);
    static_assert(bulk[27] == 0x82U);
    constexpr auto uvc = arc::usb::Uvc<0x83U>::descriptors();
    static_assert(uvc[14] == static_cast<std::uint8_t>(arc::usb::Class::video));
    constexpr auto uac = arc::usb::Uac<0x84U>::descriptors();
    static_assert(uac[14] == static_cast<std::uint8_t>(arc::usb::Class::audio));

    arc::Spsc<std::uint8_t, 4> fifo{};
    expect(arc::usb::Fifo<decltype(fifo)>::write(fifo, std::span(payload)).has_value(), "USB FIFO write");
    std::array<std::uint8_t, 2> fifo_out{};
    const auto read = arc::usb::Fifo<decltype(fifo)>::read(fifo, std::span(fifo_out));
    expect(read.has_value() && *read == 2U && fifo_out == payload, "USB FIFO read");

    struct UlpPolicy {
        static esp_err_t gpio_output(int pin) noexcept
        {
            return pin == 4 ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static esp_err_t gpio_input(int pin) noexcept
        {
            return pin == 4 ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static void gpio_write(int, const bool high) noexcept
        {
            ulp_policy_gpio = high;
        }

        static bool gpio_read(int) noexcept
        {
            return ulp_policy_gpio;
        }

        static arc::Result<std::uint16_t> adc_read(int channel) noexcept
        {
            return channel == 2 ? arc::Result<std::uint16_t>{123U} : arc::fail(ESP_ERR_INVALID_ARG);
        }

        static esp_err_t i2c_write(int port, std::uint8_t address, std::span<const std::uint8_t> data) noexcept
        {
            return port == 0 && address == 0x68U && data.size() == 2U ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static esp_err_t i2c_read(int port, std::uint8_t address, std::span<std::uint8_t> data) noexcept
        {
            if (port != 0 || address != 0x68U || data.empty()) {
                return ESP_ERR_INVALID_ARG;
            }
            data[0] = 0x42U;
            return ESP_OK;
        }

        static esp_err_t wake_main() noexcept
        {
            ulp_policy_woke = true;
            return ESP_OK;
        }
    };

    expect(arc::ulp::Gpio<4, UlpPolicy>::output().has_value(), "ULP GPIO output");
    arc::ulp::Gpio<4, UlpPolicy>::high();
    expect(arc::ulp::Gpio<4, UlpPolicy>::read(), "ULP GPIO read");
    expect(arc::ulp::Adc<2, UlpPolicy>::read_raw().value() == 123U, "ULP ADC read");
    std::array<std::uint8_t, 2> i2c_out{1U, 2U};
    expect(arc::ulp::I2c<0, UlpPolicy>::write(0x68U, std::span(i2c_out)).has_value(), "ULP I2C write");
    std::array<std::uint8_t, 1> i2c_in{};
    expect(arc::ulp::I2c<0, UlpPolicy>::read(0x68U, std::span(i2c_in)).has_value() && i2c_in[0] == 0x42U,
           "ULP I2C read");

    struct Shared {
        using value_type = std::uint32_t;
        std::uint32_t value{};

        void write(const std::uint32_t next) noexcept
        {
            value = next;
        }
    };
    struct Model {
        using Input = std::uint16_t;

        static std::uint32_t eval(const Input value) noexcept
        {
            return static_cast<std::uint32_t>(value) * 2U;
        }

        static bool wake(const std::uint32_t value) noexcept
        {
            return value > 10U;
        }
    };

    Shared shared{};
    expect(arc::ulp::SleepFsm<Model, Shared, UlpPolicy>::run_once(shared, 6U).has_value(), "ULP sleep FSM run");
    expect(shared.value == 12U && ulp_policy_woke, "ULP sleep FSM wake");
}

void test_pru_isp_vision()
{
    static_assert(arc::PruTiming{.hz = 40'000'000U, .width = 16U}.valid());

    std::array<std::uint16_t, 8> waveform{0x0001U, 0x0002U, 0x0004U, 0x0008U, 0x0010U, 0x0020U, 0x0040U, 0x0080U};
    arc::DmaChain<2> out_chain{};
    expect(arc::PruOut<2>::bind(out_chain, std::span(waveform), 4U).has_value(), "PRU out DMA bind");
    expect(out_chain.desc[0].length == 8U && out_chain.desc[1].next == out_chain.head(), "PRU out circular chain");

    const std::array<std::uint16_t, 2> patch{0xaaaaU, 0x5555U};
    expect(arc::PruOut<2>::mutate_ahead(waveform, {.read = 6U, .guard = 2U}, std::span(patch)).has_value(),
           "PRU out mutate ahead");
    expect(waveform[0] == 0xaaaaU && waveform[1] == 0x5555U, "PRU out mutation wraps");

    std::array<std::uint16_t, 8> capture{};
    arc::DmaChain<2> in_chain{};
    expect(arc::PruIn<2>::bind(in_chain, std::span(capture), 4U).has_value(), "PRU in DMA bind");
    expect(in_chain.desc[0].length == 8U && in_chain.desc[1].next == in_chain.head(), "PRU in circular chain");

    const std::array<std::uint8_t, 16> raw{
        240U,
        64U,
        220U,
        64U,
        80U,
        16U,
        96U,
        24U,
        230U,
        72U,
        210U,
        80U,
        70U,
        18U,
        88U,
        28U,
    };
    std::array<std::uint16_t, 16> rgb{};
    const auto debayered = arc::isp::Debayer::to_rgb565(std::span(raw), 4U, 4U, std::span(rgb), arc::isp::Bayer::rggb);
    expect(debayered.has_value() && *debayered == rgb.size(), "ISP debayer RGB565");
    const auto stats = arc::isp::AecAwb::measure_rgb565(std::span(rgb));
    const auto tuning = arc::isp::AecAwb::tune(stats);
    expect(stats.pixels == rgb.size() && tuning.red_gain_q8 != 0U, "ISP AEC/AWB tuning");

    const std::array<std::uint8_t, 16> gray{
        0U,
        0U,
        255U,
        255U,
        0U,
        0U,
        255U,
        255U,
        0U,
        0U,
        255U,
        255U,
        0U,
        0U,
        255U,
        255U,
    };
    std::array<std::uint8_t, 16> edges{};
    expect(arc::vision::Sobel::edges(std::span(gray), 4U, 4U, std::span(edges), 0U).has_value(),
           "Vision Sobel edges");
    expect(edges[5] != 0U || edges[6] != 0U, "Vision Sobel detects edge");

    const std::array<std::uint8_t, 16> shifted{
        0U,
        0U,
        0U,
        255U,
        0U,
        0U,
        0U,
        255U,
        0U,
        0U,
        0U,
        255U,
        0U,
        0U,
        0U,
        255U,
    };
    const auto flow = arc::vision::OpticalFlow::lucas_kanade(std::span(gray), std::span(shifted), 4U, 4U);
    expect(flow.has_value(), "Vision optical flow");
    const auto target = arc::vision::VisualServo::flow_target({.dx_q8 = 256, .confidence = 1U}, 0.25F, 12.0F);
    expect(target.q == -0.25F && target.bus == 12.0F, "Vision flow to FOC target");
}

void test_vm_bpf()
{
    using Insn = arc::vm::BpfInsn;
    using Op = arc::vm::BpfOp;

    const std::array program{
        Insn::make(Op::mov_reg, 0U, 1U),
        Insn::make(Op::add_imm, 0U, 0U, 0, 5),
        Insn::make(Op::store32, 2U, 0U),
        Insn::make(Op::exit),
    };

    const auto program_bytes = std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(program.data()),
        program.size() * sizeof(Insn),
    };

    std::array<std::uint8_t, 128> ws_frame{};
    const auto framed = arc::net::Ws::binary(std::span(ws_frame), program_bytes);
    expect(framed.has_value(), "BPF program WebSocket frame");
    const auto parsed = arc::net::Ws::parse(*framed);
    expect(parsed.has_value() && parsed->opcode == arc::net::WsOpcode::binary, "BPF program WebSocket parse");

    std::array<Insn, 8> decoded{};
    const auto bytecode = arc::vm::BPF<>::decode(parsed->payload, std::span(decoded));
    expect(bytecode.has_value() && bytecode->size() == program.size(), "BPF decode binary block");

    std::array<std::uint8_t, 8> memory{};
    arc::vm::BPF<> vm{};
    const std::array<std::int64_t, 2> args{7, 0};
    const auto result = vm.run(*bytecode, std::span(memory), std::span<const std::int64_t>{args});
    std::uint32_t stored{};
    std::memcpy(&stored, memory.data(), sizeof(stored));
    expect(result.has_value() && result->value == 12 && stored == 12U, "BPF VM executes and stores");
    expect(!vm.run(*bytecode, std::span(memory), std::span<const std::int64_t>{args}, {.writable_memory = false}),
           "BPF VM rejects store into read-only memory");

    struct BpfPolicy {
        static esp_err_t configure(std::span<const arc::WorldRegion> regions) noexcept
        {
            tee_regions = regions.size();
            return regions.size() == 1U && regions[0].untrusted == arc::PmsAccess::read ? ESP_OK : ESP_FAIL;
        }

        static esp_err_t core_world(std::uint32_t, arc::World) noexcept
        {
            return ESP_OK;
        }

        static esp_err_t peripheral_world(std::uint32_t, arc::World) noexcept
        {
            return ESP_OK;
        }
    };

    tee_regions = 0U;
    expect(arc::vm::BpfSandbox::protect<BpfPolicy>(std::span(memory)).has_value() && tee_regions == 1U,
           "BPF sandbox configures VM memory region");
}

void test_ulp_ml_paillier_covert()
{
    const std::array<std::int8_t, 2> ulp_weights{1, 1};
    const std::array<std::int32_t, 1> ulp_bias{0};
    const arc::ulp::ml::QuantDenseS8<2, 1> ulp_dense{
        .weights = ulp_weights.data(),
        .bias = ulp_bias.data(),
    };
    const std::array<std::int8_t, 2> ulp_input{2, 4};
    std::array<std::int8_t, 1> ulp_logits{};
    expect(ulp_dense.eval(std::span<const std::int8_t, 2>{ulp_input}, std::span<std::int8_t, 1>{ulp_logits}),
           "ULP ML dense eval");
    expect(ulp_logits[0] == 6, "ULP ML dense output");

    struct WakePolicy {
        static esp_err_t i2c_read(int port, std::uint8_t address, std::span<std::uint8_t> data) noexcept
        {
            if (port != 0 || address != 0x1dU || data.size() != 2U) {
                return ESP_ERR_INVALID_ARG;
            }
            data[0] = 130U;
            data[1] = 132U;
            return ESP_OK;
        }

        static esp_err_t i2c_write(int, std::uint8_t, std::span<const std::uint8_t>) noexcept
        {
            return ESP_OK;
        }

        static esp_err_t wake_main() noexcept
        {
            semantic_wake = true;
            return ESP_OK;
        }
    };
    semantic_wake = false;
    ulp_logits = {};
    expect(arc::ulp::ml::SemanticWake<0, 0x1dU, 2, 1, WakePolicy>::run_once(
               ulp_dense,
               std::span<std::int8_t, 1>{ulp_logits},
               5)
               .has_value(),
           "ULP semantic wake workflow");
    expect(semantic_wake && ulp_logits[0] == 6, "ULP semantic wake triggers");

    struct ToyInt {
        std::uint64_t value{};

        ToyInt() = default;
        constexpr ToyInt(const std::uint64_t next)
            : value{next}
        {
        }

        [[nodiscard]] std::expected<ToyInt, int> clone() const noexcept
        {
            return *this;
        }

        [[nodiscard]] std::expected<ToyInt, int> mul_mod(
            const ToyInt& rhs,
            const ToyInt& modulus) const noexcept
        {
            return ToyInt{(value * rhs.value) % modulus.value};
        }

        [[nodiscard]] std::expected<ToyInt, int> exp_mod(
            const ToyInt& exponent,
            const ToyInt& modulus,
            ToyInt* = nullptr) const noexcept
        {
            auto base = value % modulus.value;
            auto exp = exponent.value;
            auto out = std::uint64_t{1U};
            while (exp != 0U) {
                if ((exp & 1U) != 0U) {
                    out = (out * base) % modulus.value;
                }
                base = (base * base) % modulus.value;
                exp >>= 1U;
            }
            return ToyInt{out};
        }
    };

    const arc::crypto::PaillierPublic<ToyInt> key{
        .n = ToyInt{5},
        .n2 = ToyInt{25},
        .g = ToyInt{6},
    };
    const auto c3 = arc::crypto::Paillier<ToyInt>::encrypt(key, ToyInt{3}, ToyInt{2});
    const auto c4 = arc::crypto::Paillier<ToyInt>::encrypt(key, ToyInt{4}, ToyInt{2});
    expect(c3.has_value() && c4.has_value(), "Paillier encrypt toy values");
    const auto sum = arc::crypto::Paillier<ToyInt>::add(key, *c3, *c4);
    expect(sum.has_value() && sum->value == 14U, "Paillier homomorphic add");
    const auto plus_plain = arc::crypto::Paillier<ToyInt>::add_plain(key, *c3, ToyInt{4});
    expect(plus_plain.has_value() && plus_plain->value == 2U, "Paillier add plaintext");
    const std::array encrypted{*c3, *c4};
    const auto aggregate = arc::crypto::Paillier<ToyInt>::aggregate(key, std::span<const ToyInt>{encrypted});
    expect(aggregate.has_value() && aggregate->value == sum->value, "Paillier aggregate ciphertexts");

    struct PwmCarrier {
        static esp_err_t hz(const std::uint32_t value) noexcept
        {
            covert_hz = value;
            return ESP_OK;
        }

        static esp_err_t duty(const std::uint32_t value) noexcept
        {
            covert_duty = value;
            return ESP_OK;
        }
    };
    struct SigmaCarrier {
        static esp_err_t set(const int value) noexcept
        {
            covert_density = value;
            return ESP_OK;
        }
    };

    constexpr arc::covert::FskConfig fsk{
        .mark_hz = 2'400'000U,
        .space_hz = 2'100'000U,
        .symbol_us = 50U,
        .duty_permille = 500U,
        .mark_density = 80,
        .space_density = -80,
    };
    std::array<arc::covert::FskSymbol, 8> symbols{};
    const std::array<std::uint8_t, 1> bits{0b1010'0000U};
    expect(arc::covert::Fsk::plan(std::span<arc::covert::FskSymbol, 8>{symbols}, std::span<const std::uint8_t>{bits}, fsk).has_value(),
           "Covert FSK plan bytes");
    expect(symbols[0].mark && !symbols[1].mark && symbols[0].hz == fsk.mark_hz, "Covert FSK symbol mapping");

    expect(arc::covert::EmTx<PwmCarrier>::bit(true, fsk).has_value(), "Covert EM PWM mark");
    expect(covert_hz == fsk.mark_hz && covert_duty == fsk.duty_permille, "Covert EM PWM retune");
    expect(arc::covert::EmTx<SigmaCarrier>::bit(false, fsk).has_value(), "Covert EM Sigma space");
    expect(covert_density == fsk.space_density, "Covert Sigma density");
    expect(arc::covert::SonicTx<PwmCarrier>::bit(false, fsk).has_value(), "Covert sonic PWM space");
    expect(covert_hz == fsk.space_hz, "Covert sonic frequency");
}

void test_goal_wave_surfaces()
{
    using Kem = arc::crypto::Kyber512;
    Kem::Poly poly{};
    for (std::size_t i = 0U; i < poly.size(); ++i) {
        poly[i] = static_cast<std::int16_t>(i & 0x7fU);
    }
    const auto original = poly;
    expect(Kem::ntt(std::span<std::int16_t, Kem::n>{poly}).has_value(), "Kyber NTT");
    expect(Kem::inverse_ntt(std::span<std::int16_t, Kem::n>{poly}).has_value(), "Kyber inverse NTT");
    expect(poly == original, "Kyber NTT roundtrip");

    Kem::Poly product{};
    expect(Kem::pointwise(
               std::span<std::int16_t, Kem::n>{product},
               std::span<const std::int16_t, Kem::n>{original},
               std::span<const std::int16_t, Kem::n>{original})
               .has_value(),
           "Kyber SIMD pointwise product");
    expect(product[5] == Kem::mul(5, 5), "Kyber pointwise coefficient");

    const std::array<std::uint8_t, 4> seed{1, 2, 3, 4};
    const std::array<std::uint8_t, 4> coins{9, 10, 11, 12};
    std::array<std::uint8_t, Kem::public_key_bytes> public_key{};
    std::array<std::uint8_t, Kem::secret_key_bytes> secret_key{};
    std::array<std::uint8_t, Kem::ciphertext_bytes> ciphertext{};
    std::array<std::uint8_t, Kem::shared_bytes> shared{};
    std::array<std::uint8_t, Kem::shared_bytes> opened{};
    expect(Kem::keypair(seed, public_key, secret_key).has_value(), "Kyber zero-alloc keypair");
    expect(Kem::encapsulate(public_key, coins, ciphertext, shared).has_value(), "Kyber zero-alloc encapsulate");
    expect(Kem::decapsulate(secret_key, ciphertext, opened).has_value() && opened == shared, "Kyber zero-alloc decapsulate");
    expect(ciphertext[0] != ciphertext[1] && shared[0] != 0U, "Kyber KEM fills buffers");

    constexpr arc::sdr::TxConfig sdr_cfg{
        .sample_hz = 1'000'000U,
        .carrier_hz = 100'000U,
        .deviation_hz = 25'000U,
        .gpio = 4U,
        .one_word = 0x10U,
    };
    std::array<std::uint16_t, 16> rf_words{};
    const std::array<std::int16_t, 4> audio{0, 16'000, -16'000, 0};
    const auto fm = arc::sdr::PulseSynth::fm(rf_words, audio, sdr_cfg);
    expect(fm.has_value() && fm->words == audio.size(), "SDR FM pulse synthesis");
    const std::array<std::uint8_t, 1> ook_bits{0b1010'0000U};
    const auto ook = arc::sdr::PulseSynth::ook(rf_words, ook_bits, sdr_cfg);
    expect(ook.has_value() && rf_words[0] == sdr_cfg.one_word && rf_words[1] == 0U, "SDR OOK pulse synthesis");
    arc::DmaChain<1> rf_dma{};
    expect(arc::sdr::bind_dma(rf_dma, std::span<std::uint16_t>{rf_words}).has_value() && rf_dma.head()->buffer == rf_words.data(),
           "SDR DMA bind");
    struct SdrPolicy {
        static esp_err_t cam_start(const arc::sdr::TxConfig& config, const std::span<const std::uint16_t> words) noexcept
        {
            sdr_words = config.gpio == 4U ? words.size() : 0U;
            return ESP_OK;
        }

        static esp_err_t cam_stop() noexcept
        {
            return ESP_OK;
        }
    };
    expect(arc::sdr::Tx<SdrPolicy>::start(sdr_cfg, std::span<const std::uint16_t>{rf_words}).has_value() &&
               sdr_words == rf_words.size(),
           "SDR LCD_CAM policy start");

    std::array<std::int16_t, 64> chirp{};
    expect(arc::swarm::AcousticSlam::fmcw_chirp(chirp).has_value() && chirp[1] != 0, "Acoustic FMCW chirp");
    const std::array<std::int16_t, 6> ref{1, 2, 3, 4, 0, 0};
    const std::array<std::int16_t, 6> delayed{0, 1, 2, 3, 4, 0};
    const auto tdoa = arc::swarm::AcousticSlam::tdoa(ref, delayed, 1'000U, 3U);
    expect(tdoa.has_value() && tdoa->lag_samples == 1 && near(tdoa->delta_mm, 343.0F, 0.1F), "Acoustic TDoA");
    std::array<arc::simd::ComplexF32, 8> fft_ref{};
    std::array<arc::simd::ComplexF32, 8> fft_delayed{};
    std::array<arc::simd::ComplexF32, 8> fft_scratch{};
    fft_ref[1].re = 1.0F;
    fft_delayed[2].re = 1.0F;
    const auto phat = arc::swarm::AcousticSlam::gcc_tdoa(fft_ref, fft_delayed, fft_scratch, 3U, 1'000.0F);
    expect(phat.has_value(), "Acoustic SIMD FFT GCC-PHAT");

    const std::array<arc::swarm::AcousticAnchor, 4> anchors{
        arc::swarm::AcousticAnchor{.node_id = 1U, .position = {}},
        arc::swarm::AcousticAnchor{.node_id = 2U, .position = {.x_mm = 1'000.0F}},
        arc::swarm::AcousticAnchor{.node_id = 3U, .position = {.y_mm = 1'000.0F}},
        arc::swarm::AcousticAnchor{.node_id = 4U, .position = {.z_mm = 1'000.0F}},
    };
    const arc::swarm::Position3 target{.x_mm = 300.0F, .y_mm = 400.0F, .z_mm = 500.0F};
    std::array<float, 4> ranges{};
    for (std::size_t i = 0U; i < anchors.size(); ++i) {
        const auto dx = target.x_mm - anchors[i].position.x_mm;
        const auto dy = target.y_mm - anchors[i].position.y_mm;
        const auto dz = target.z_mm - anchors[i].position.z_mm;
        ranges[i] = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
    }
    const auto solved = arc::swarm::AcousticSlam::solve(std::span<const arc::swarm::AcousticAnchor, 4>{anchors}, std::span<const float, 4>{ranges});
    expect(solved.has_value() && near(solved->x_mm, target.x_mm, 0.5F) && near(solved->z_mm, target.z_mm, 0.5F),
           "Acoustic 3D trilateration");
    arc::dsp::Kalman<float, 6, 3> slam_filter{};
    expect(arc::swarm::AcousticSlam::correct_filter(slam_filter, *solved).has_value(), "Acoustic Kalman correction");

    struct HyperPolicy {
        static esp_err_t configure(const std::span<const arc::WorldRegion> regions) noexcept
        {
            hypervisor_regions = regions.size();
            return regions.size() == 2U ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static esp_err_t core_world(std::uint32_t, arc::World) noexcept
        {
            return ESP_OK;
        }

        static esp_err_t peripheral_world(std::uint32_t, arc::World) noexcept
        {
            return ESP_OK;
        }

        static esp_err_t bind_traps() noexcept
        {
            ++hypervisor_traps;
            return ESP_OK;
        }

        static esp_err_t start_core0(void (*)(void*) noexcept, void*) noexcept
        {
            return ESP_OK;
        }

        static arc::Result<arc::vm::TrapDecision> handle_trap(const arc::vm::TrapFrame& frame) noexcept
        {
            return arc::vm::TrapDecision{
                .action = frame.kind == arc::vm::TrapKind::peripheral ? arc::vm::TrapAction::emulate : arc::vm::TrapAction::terminate,
                .value = 0x55U,
            };
        }
    };
    std::array<std::uint8_t, 16> guest_code{};
    std::array<std::uint8_t, 32> guest_ram{};
    const std::array<std::uint32_t, 1> trusted_peripherals{7U};
    const arc::vm::Partition partition{
        .code = guest_code,
        .ram = guest_ram,
        .trusted_peripherals = trusted_peripherals,
    };
    expect(arc::vm::Hypervisor::apply<HyperPolicy>(partition).has_value() &&
               hypervisor_regions == 2U && hypervisor_traps == 1U,
           "Hypervisor partition apply");
    const auto trap = arc::vm::Hypervisor::trap<HyperPolicy>({
        .kind = arc::vm::TrapKind::peripheral,
        .address = 0x6000'0000U,
    });
    expect(trap.has_value() && trap->action == arc::vm::TrapAction::emulate && trap->value == 0x55U,
           "Hypervisor trap emulation");

    struct ChaosPolicy {
        static esp_err_t flip_sram(const std::span<std::uint8_t> sram, const std::size_t index, const std::uint8_t mask) noexcept
        {
            sram[index] ^= mask;
            ++chaos_flips;
            return ESP_OK;
        }

        static esp_err_t stall_i2c(const std::uint32_t) noexcept
        {
            ++chaos_stalls;
            return ESP_OK;
        }

        static esp_err_t drop_espnow(const bool drop) noexcept
        {
            chaos_drops += drop ? 1U : 0U;
            return ESP_OK;
        }

        static esp_err_t tight_overrun(const std::uint32_t) noexcept
        {
            ++chaos_overruns;
            return ESP_OK;
        }
    };
    using ChaosDump = arc::Postmortem<8>;
    ChaosDump::clear();
    arc::chaos::State chaos{};
    std::array<std::uint8_t, 4> sram{0x10U, 0x20U, 0x30U, 0x40U};
    expect(arc::chaos::Monkey::inject<ChaosPolicy>(chaos, arc::chaos::Fault::bit_flip, {}, sram).has_value() &&
               chaos_flips == 1U,
           "Chaos SRAM bit flip");
    expect(!arc::chaos::Monkey::drop_espnow(chaos) &&
               !arc::chaos::Monkey::drop_espnow(chaos) &&
               arc::chaos::Monkey::drop_espnow(chaos),
           "Chaos exact ESP-NOW 33 percent drop cadence");
    arc::chaos::Monkey::record<ChaosDump>({.fault = arc::chaos::Fault::tight_overrun, .payload = 1U, .aux = 2U});
    expect(ChaosDump::size() == 1U && ChaosDump::event(0).payload == static_cast<std::uint32_t>(arc::chaos::Fault::tight_overrun),
           "Chaos Postmortem record");
    arc::Rcu<std::uint32_t> recovered{};
    arc::chaos::Monkey::recover_rcu(recovered, 99U);
    expect(recovered.read() == 99U, "Chaos RCU recovery");
}

void test_resilient_edge_goal_surfaces()
{
    using Intermittent = arc::power::Intermittent<64>;
    Intermittent::clear();
    intermittent_bod_mv = 0U;
    intermittent_restored = false;

    struct IntermittentPolicy {
        static esp_err_t bod_arm(const std::uint16_t threshold_mv, void (*)(void*) noexcept) noexcept
        {
            intermittent_bod_mv = threshold_mv;
            return ESP_OK;
        }

        static void dying_gasp(void*) noexcept {}

        static esp_err_t restore(
            const std::uintptr_t pc,
            const std::uintptr_t sp,
            const std::span<const std::byte> image) noexcept
        {
            intermittent_restored = pc == 0x1234U && sp == 0x5678U && image.size() == 6U;
            return ESP_OK;
        }
    };

    const auto brownout = arc::power::BrownoutPlan::above_death(3'100U);
    expect(brownout.threshold_mv == 3'200U && Intermittent::arm<IntermittentPolicy>(brownout).has_value() &&
               intermittent_bod_mv == 3'200U,
           "Intermittent brownout threshold");
    const std::array stack{std::byte{0x10}, std::byte{0x11}, std::byte{0x12}, std::byte{0x13}};
    const std::array rcu_state{std::byte{0x20}, std::byte{0x21}};
    expect(Intermittent::save({.pc = 0x1234U, .sp = 0x5678U}, stack, rcu_state).has_value() &&
               Intermittent::ready() &&
               Intermittent::checkpoint.frame.bytes == 6U &&
               Intermittent::checkpoint.image[0] == std::byte{0x10} &&
               Intermittent::checkpoint.image[4] == std::byte{0x20},
           "Intermittent checkpoint image");
    expect(Intermittent::resume<IntermittentPolicy>().has_value() && intermittent_restored,
           "Intermittent restore hook");

    const std::array entropy_sram{std::byte{0xA5}, std::byte{0x3C}, std::byte{0x5A}};
    std::array<std::uint8_t, 4> raw_entropy{};
    expect(arc::crypto::Puf::sample_sram(entropy_sram, raw_entropy).has_value() &&
               raw_entropy[0] == 0xA5U && raw_entropy[2] == 0x5AU,
           "PUF SRAM decay sample");
    const std::array<std::uint8_t, 1> raw_pairs{0b0001'1011U};
    std::array<std::uint8_t, 1> stable_bits{};
    const auto puf_stats = arc::crypto::Puf::von_neumann(raw_pairs, stable_bits);
    expect(puf_stats.raw_bits == 8U && puf_stats.stable_bits == 2U && puf_stats.ones == 1U && stable_bits[0] == 0b10U,
           "PUF von Neumann extractor");
    struct PufAdc {
        static arc::Result<std::uint16_t> read() noexcept
        {
            static std::uint16_t sample{100U};
            sample = static_cast<std::uint16_t>(sample + 7U);
            return sample;
        }
    };
    arc::dsp::Biquad<std::int32_t>::State puf_filter{};
    std::array<std::uint16_t, 3> adc_noise{};
    expect(arc::crypto::Puf::sample_adc<PufAdc>(adc_noise, puf_filter).has_value() &&
               adc_noise[0] != 0U && adc_noise[2] > adc_noise[0],
           "PUF ADC noise filter");
    struct PufHash {
        static arc::Result<std::array<std::uint8_t, 32>> sha256(const std::span<const std::uint8_t> stable) noexcept
        {
            std::array<std::uint8_t, 32> key{};
            key[0] = static_cast<std::uint8_t>(stable.size());
            key[31] = stable.empty() ? 0U : stable[0];
            return key;
        }
    };
    const auto puf_key = arc::crypto::Puf::derive_with<PufHash>(stable_bits);
    expect(puf_key.has_value() && (*puf_key)[0] == stable_bits.size() && (*puf_key)[31] == stable_bits[0],
           "PUF key derivation hook");

    flexroute_gpio = 0U;
    flexroute_detached = 0U;
    struct FlexPolicy {
        static esp_err_t matrix_out(
            const std::uint32_t gpio,
            const std::uint32_t signal,
            const bool,
            const bool) noexcept
        {
            flexroute_gpio = gpio + signal;
            return ESP_OK;
        }

        static esp_err_t matrix_in(const std::uint32_t gpio, const std::uint32_t signal, const bool) noexcept
        {
            flexroute_gpio = gpio + signal;
            return ESP_OK;
        }

        static esp_err_t matrix_detach(const std::uint32_t gpio, const arc::matrix::RouteDir) noexcept
        {
            flexroute_detached = gpio;
            return ESP_OK;
        }
    };
    arc::matrix::FlexRoutePlan<1> route_plan{
        .active = {.signal = 99U, .gpio = 4U},
        .spare_gpio = {18U},
    };
    const auto healed = arc::matrix::FlexRoute::heal<FlexPolicy>(
        route_plan,
        {.expected_hz = 40'000U, .measured_hz = 0U, .tolerance_hz = 10U});
    expect(healed.has_value() && healed->gpio == 18U && route_plan.active.gpio == 18U &&
               flexroute_detached == 4U && flexroute_gpio == 117U,
           "FlexRoute reroutes failed PWM");

    std::array<arc::dsp::Transducer, 16> transducers{};
    for (std::size_t i = 0U; i < transducers.size(); ++i) {
        transducers[i].pos.x_mm = static_cast<float>(i * 5U);
        transducers[i].gain = 1.0F;
    }
    const auto wave_plan = arc::dsp::Wavefront::focus<16>(
        std::span<const arc::dsp::Transducer, 16>{transducers},
        {.x_mm = 35.0F, .z_mm = 100.0F});
    std::array<std::int16_t, 64> wave_samples{};
    expect(wave_plan.has_value() &&
               arc::dsp::Wavefront::synthesize<16>(*wave_plan, wave_samples, 4U).has_value() &&
               wave_samples[1] != wave_samples[17],
           "Wavefront 16-channel synthesis");
    struct WaveTdm {
        static arc::Result<std::size_t> write(const std::span<std::int16_t> samples, const std::uint32_t) noexcept
        {
            wavefront_writes = samples.size();
            return samples.size();
        }
    };
    expect(arc::dsp::Wavefront::stream<WaveTdm>(std::span<std::int16_t, 64>{wave_samples}).has_value() &&
               wavefront_writes == wave_samples.size(),
           "Wavefront I2S TDM stream hook");

    std::array<std::uint8_t, 25> stars_frame{};
    stars_frame[6] = 240U;
    stars_frame[8] = 230U;
    stars_frame[17] = 220U;
    std::array<std::uint8_t, 25> stars_mask{};
    expect(arc::vision::StarTracker::threshold(stars_frame, stars_mask, 200U).value() == 3U &&
               stars_mask[6] == 255U,
           "StarTracker SIMD threshold");
    std::array<arc::vision::StarPoint, 4> stars{};
    const auto star_count = arc::vision::StarTracker::centroids(stars_frame, 5U, 5U, 200U, stars);
    expect(star_count.has_value() && *star_count == 3U && stars[0].x_q4 == 16U && stars[2].y_q4 == 48U,
           "StarTracker sub-pixel centroids");
    auto signature = arc::vision::StarTracker::triangle(stars[0], stars[1], stars[2]);
    signature.pitch_rad = 0.1F;
    signature.yaw_rad = -0.2F;
    signature.roll_rad = 0.3F;
    const std::array catalog{signature};
    const arc::MmuSpan<arc::vision::StarTriangle> mapped_catalog{
        .values = std::span<const arc::vision::StarTriangle>{catalog},
    };
    const auto pose = arc::vision::StarTracker::match(
        std::span<const arc::vision::StarPoint>{stars.data(), *star_count},
        mapped_catalog);
    expect(pose.has_value() && pose->matches == 1U && near(pose->yaw_rad, -0.2F),
           "StarTracker MMU catalog match");
}

void test_current_goal_surfaces()
{
    const arc::Copy<>::Ticket pending_copy{.target = 1U};
    expect(!arc::Copy<>::wait_for(pending_copy, 2U), "Copy bounded wait reports unfinished target");
    struct CopyYield {
        static void yield() noexcept
        {
            ++copy_yields;
        }
    };
    copy_yields = 0U;
    expect(!arc::Copy<>::wait_yield<CopyYield>(pending_copy, 1U, 2U) && copy_yields == 2U,
           "Copy wait_yield calls policy instead of hard spinning forever");

    alignas(arc::cache_line) std::array<std::uint8_t, arc::cache_line> copy_src{};
    alignas(arc::cache_line) std::array<std::uint8_t, arc::cache_line> copy_dst{};
    copy_src[0] = 42U;
    auto copy_lease = arc::Copy<>::lease_coherent(std::span(copy_dst), std::span(copy_src));
    expect(copy_lease.has_value() && copy_lease->active(), "Copy coherent lease queues");
    expect(copy_lease->finish().has_value() && !copy_lease->active(), "Copy coherent lease finishes");
    expect(copy_lease->finish().has_value() && copy_dst[0] == 42U, "Copy coherent lease finish is idempotent");

    auto copy_src_dma = arc::dmabuf<std::uint8_t>(65U);
    auto copy_dst_dma = arc::dmabuf<std::uint8_t>(65U);
    expect(static_cast<bool>(copy_src_dma) && static_cast<bool>(copy_dst_dma), "Copy CapsBuf allocations succeed");
    copy_src_dma[0] = 33U;
    copy_src_dma[64] = 99U;
    esp_cache_last_msync_bytes = 0U;
    expect(arc::Copy<>::copy_coherent(copy_dst_dma, copy_src_dma) == ESP_OK &&
               copy_dst_dma[0] == 33U && copy_dst_dma[64] == 99U &&
               esp_cache_last_msync_bytes == copy_dst_dma.storage_bytes(),
           "Copy coherent CapsBuf syncs padded storage");

    struct CloakPolicy {
        static std::uint32_t rng() noexcept
        {
            return 3U;
        }

        static void heavy() noexcept
        {
            ++cloak_heavy;
        }

        static void light() noexcept
        {
            ++cloak_light;
        }
    };
    cloak_heavy = 0U;
    cloak_light = 0U;
    const std::array cloak_dummy{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    const auto cloak_stats = arc::crypto::Cloak::scramble<CloakPolicy>(
        {.stall_mask = 0x03U, .dummy_reads = 3U},
        cloak_dummy);
    expect(cloak_stats.stalls == 4U && cloak_stats.dummy_reads == 3U, "Cloak scrambling budget");
    const auto flatten_stats = arc::crypto::Cloak::flatten<CloakPolicy>({.flatten_ticks = 3U}, 1U);
    expect(flatten_stats.flatten_heavy == 1U && flatten_stats.flatten_light == 2U &&
               cloak_heavy == 1U && cloak_light == 2U,
           "Cloak power flatten policy hooks");
    const auto cloaked = arc::crypto::Cloak::run<CloakPolicy>(
        {.stall_mask = 0U, .dummy_reads = 1U, .flatten_ticks = 1U},
        cloak_dummy,
        []() noexcept { return 42; });
    expect(cloaked == 42, "Cloak run returns protected result");

    arc::nav::Eskf<float>::State eskf{};
    const auto predicted = arc::nav::Eskf<float>::predict(
        eskf,
        {.gyro_rad_s = {0.0F, 0.0F, 1.0F}, .accel_m_s2 = {1.0F, 0.0F, 0.0F}, .dt_s = 0.01F});
    expect(predicted.has_value() && eskf.velocity[0] > 0.0F && eskf.covariance(0, 0) > 0.0F,
           "ESKF IMU prediction");
    expect(arc::nav::Eskf<float>::correct_pose(eskf, {1.0F, 2.0F, 3.0F}, {.w = 1.0F}, 0.5F).has_value() &&
               near(eskf.position[0], 0.50005F, 0.001F),
           "ESKF slow sensor correction");

    std::array<std::int8_t, 32> snn_weights{};
    for (auto& weight : snn_weights) {
        weight = 8;
    }
    arc::ml::Snn<16, 2> snn{
        .weights = std::span<const std::int8_t, 32>{snn_weights},
        .params = {.threshold = 64, .leak = 0, .reset = 0},
    };
    const std::array<std::int8_t, 16> snn_input{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    std::array<std::uint8_t, 2> snn_spikes{};
    expect(snn.step(std::span<const std::int8_t, 16>{snn_input}, std::span<std::uint8_t, 2>{snn_spikes}).has_value() &&
               snn_spikes[0] == 1U && snn_spikes[1] == 1U,
           "SNN LIF layer spikes");

    struct FabricPolicy {
        static esp_err_t send(const std::uint32_t next_hop, const std::span<const std::uint8_t> bytes) noexcept
        {
            fabric_next_hop = next_hop;
            fabric_bytes = bytes.size();
            return ESP_OK;
        }
    };
    const arc::net::FabricTable<1> fabric_routes{.routes = {arc::net::FabricRoute{.dst = 4U, .next_hop = 2U, .ttl = 2U}}};
    auto fabric_packet = arc::net::Fabric::make<8>(1U, 4U, std::span<const std::uint8_t>{snn_spikes}, 2U);
    const arc::net::SwarmSchedule<2> mesh_schedule{
        .node_ids = {1U, 2U},
        .tdma = {.period_us = 200U, .slot_us = 100U},
    };
    expect(fabric_packet.has_value() &&
               arc::net::Fabric::forward<FabricPolicy>(fabric_routes, *fabric_packet, mesh_schedule, 150U).has_value() &&
               fabric_next_hop == 2U && fabric_bytes == 2U && fabric_packet->ttl == 1U,
           "Fabric deterministic TDMA forward");

    using Mag = arc::MagLev<1>;
    Mag::State mag_state{};
    Mag::Model mag_model{};
    mag_model.d[0][0] = 1.0F;
    const auto mag_duty = Mag::stabilize(mag_state, mag_model, Mag::Vec{2.0F}, {.min = -0.5F, .max = 0.5F});
    expect(mag_duty.has_value() && near((*mag_duty)[0], 0.5F), "MagLev state-space clamp");

    struct HilPolicy {
        static esp_err_t apply(const arc::HilAction&) noexcept
        {
            ++hil_actions;
            return ESP_OK;
        }
    };
    const arc::HilScript<2> hil_script{.actions = {
                                           arc::HilAction{
                                               .role = arc::HilRole::physical_chaos,
                                               .fault = arc::HilFault::i2c_short,
                                               .at_tick = 10U,
                                               .duration_ticks = 5U,
                                           },
                                           arc::HilAction{
                                               .role = arc::HilRole::radio_jammer,
                                               .fault = arc::HilFault::espnow_jam,
                                               .at_tick = 12U,
                                               .duration_ticks = 1U,
                                           },
                                       }};
    hil_actions = 0U;
    expect(hil_script.run_due<HilPolicy>(12U).has_value() && hil_actions == 2U, "HIL script applies due faults");

    const std::array<std::int16_t, 4> audio_samples{10, -10, 20, -20};
    std::array<std::int8_t, 2> audio_bins{};
    expect(arc::ulp::ml::AudioSignature<4, 2>::bins(
               std::span<const std::int16_t, 4>{audio_samples},
               std::span<std::int8_t, 2>{audio_bins},
               10) &&
               audio_bins[0] == 2 && audio_bins[1] == 4,
           "ULP audio signature bins");

    struct UsbHostPolicy {
        static esp_err_t host_start(const arc::usb::HostConfig&) noexcept
        {
            ++usb_host_started;
            return ESP_OK;
        }

        static arc::Result<arc::usb::HostDevice> host_poll(const std::uint32_t) noexcept
        {
            return arc::usb::HostDevice{.address = 2U, .vid = 0x1209U, .pid = 0x0001U, .configured = true};
        }

        static arc::Result<std::size_t> host_submit(const arc::usb::HostTransfer transfer) noexcept
        {
            ++usb_host_submitted;
            return transfer.tx.size() + transfer.rx.size();
        }

        static esp_err_t host_stop() noexcept
        {
            return ESP_OK;
        }
    };
    std::array<std::uint8_t, 2> usb_tx{1U, 2U};
    std::array<std::uint8_t, 4> usb_rx{};
    const auto usb_device = arc::usb::Host::poll<UsbHostPolicy>();
    const auto usb_bytes = arc::usb::Host::submit<UsbHostPolicy>({
        .address = 2U,
        .endpoint = 1U,
        .tx = usb_tx,
        .rx = usb_rx,
    });
    expect(arc::usb::Host::start<UsbHostPolicy>().has_value() &&
               usb_device.has_value() && usb_device->configured &&
               usb_bytes.has_value() && *usb_bytes == 6U && usb_host_started == 1U && usb_host_submitted == 1U &&
               arc::usb::Host::stop<UsbHostPolicy>().has_value(),
           "USB host policy facade");

    struct RdmaPolicy {
        static esp_err_t store(const std::uintptr_t dst, const std::span<const std::uint8_t> bytes) noexcept
        {
            rdma_store_addr = dst;
            rdma_store_bytes = bytes.size();
            auto* out = reinterpret_cast<std::uint8_t*>(dst);
            for (std::size_t i = 0U; i < bytes.size(); ++i) {
                out[i] = bytes[i];
            }
            return ESP_OK;
        }

        static esp_err_t promiscuous(const bool) noexcept
        {
            return ESP_OK;
        }
    };
    alignas(16) std::array<std::uint8_t, 16> rdma_dst{};
    const arc::net::RdmaWindow rdma_window{
        .node = 2U,
        .base = reinterpret_cast<std::uintptr_t>(rdma_dst.data()),
        .bytes = rdma_dst.size(),
    };
    const std::array<std::uint8_t, 3> rdma_payload{7U, 8U, 9U};
    const auto rdma_frame = arc::net::Rdma::write<8>(rdma_window, 1U, rdma_payload, 4U);
    expect(rdma_frame.has_value() &&
               arc::net::Rdma::apply<RdmaPolicy>(rdma_window, *rdma_frame).has_value() &&
               rdma_dst[4] == 7U && rdma_dst[6] == 9U &&
               rdma_store_addr == rdma_window.base + 4U && rdma_store_bytes == rdma_payload.size() &&
               arc::net::Rdma::promiscuous<RdmaPolicy>().has_value(),
           "RDMA raw-frame write applies to aligned window");

    struct DistributedMmuPolicy {
        static esp_err_t fetch_line(
            const arc::mmu::RemoteLineRequest request,
            const std::span<std::uint8_t> out) noexcept
        {
            distributed_fetch_bytes = out.size();
            for (std::size_t i = 0U; i < out.size(); ++i) {
                out[i] = static_cast<std::uint8_t>(request.node + i);
            }
            return ESP_OK;
        }

        static esp_err_t remap_line(
            const arc::mmu::RemoteFault&,
            const std::span<const std::uint8_t> line) noexcept
        {
            distributed_remap_line = reinterpret_cast<std::uintptr_t>(line.data());
            return ESP_OK;
        }

        static esp_err_t resume(const std::uint8_t core) noexcept
        {
            distributed_resume_core = core;
            return ESP_OK;
        }
    };
    std::array<std::uint8_t, 32> distributed_line{};
    const arc::mmu::RemoteFault remote_fault{
        .address = 0x2004U,
        .span_base = 0x2000U,
        .span_bytes = 128U,
        .node = 7U,
        .core = 1U,
        .line_bytes = 32U,
    };
    const auto remote_resume = arc::mmu::DistributedPager::service_fault<DistributedMmuPolicy>(
        remote_fault,
        distributed_line);
    expect(remote_resume.has_value() && remote_resume->request.node == 7U &&
               remote_resume->request.line == 0x2000U && remote_resume->resumed &&
               distributed_fetch_bytes == distributed_line.size() &&
               distributed_remap_line == reinterpret_cast<std::uintptr_t>(distributed_line.data()) &&
               distributed_resume_core == 1U && distributed_line[1] == 8U,
           "DistributedPager fetches remote cache line and resumes trapped core");

    struct TraceLivePolicy {
        static esp_err_t trace_arm(const arc::trace::LiveStreamConfig config) noexcept
        {
            live_trace_bytes = config.watermark_bytes;
            return ESP_OK;
        }

        static arc::Result<arc::trace::LiveChunk> trace_half(const std::size_t watermark) noexcept
        {
            return arc::trace::LiveChunk{.bytes = std::span<const std::uint8_t>{live_trace_buffer}.first(watermark), .bank = 1U};
        }

        static esp_err_t trace_swap() noexcept
        {
            ++live_trace_swaps;
            return ESP_OK;
        }
    };
    struct TraceSink {
        esp_err_t write(const std::span<const std::uint8_t> bytes) noexcept
        {
            live_trace_sink_bytes += bytes.size();
            return ESP_OK;
        }
    };
    TraceSink trace_sink{};
    live_trace_swaps = 0U;
    live_trace_sink_bytes = 0U;
    expect(arc::trace::LiveStream::arm<TraceLivePolicy>({.bank_bytes = 64U, .watermark_bytes = 32U}).has_value() &&
               arc::trace::LiveStream::drain_isr<TraceLivePolicy>(
                   trace_sink,
                   {.bank_bytes = 64U, .watermark_bytes = 32U})
                   .has_value() &&
               live_trace_bytes == 32U && live_trace_sink_bytes == 32U && live_trace_swaps == 1U,
           "LiveStream swaps TRAX bank and exfiltrates half-full chunk");

    struct GovernorPolicy {
        static esp_err_t cpu_boost() noexcept
        {
            ++governor_boosts;
            return ESP_OK;
        }

        static esp_err_t cpu_release() noexcept
        {
            ++governor_releases;
            return ESP_OK;
        }
    };
    governor_boosts = 0U;
    governor_releases = 0U;
    arc::power::Governor<float> hot_governor{};
    const auto hot_decision = hot_governor.tick<GovernorPolicy>(130U, 100U, {.guard_cycles = 0, .min_samples = 1U});
    arc::power::Governor<float> cool_governor{};
    const auto cool_decision = cool_governor.tick<GovernorPolicy>(50U, 100U, {.guard_cycles = 0, .min_samples = 1U});
    expect(hot_decision.has_value() && cool_decision.has_value() &&
               hot_decision->action == arc::power::GovernorAction::boost &&
               cool_decision->action == arc::power::GovernorAction::release &&
               governor_boosts == 1U && governor_releases == 1U,
           "Governor predicts slack and applies CPU lock policy");

    struct VslamCamera {
        static esp_err_t capture_gray(const std::span<std::uint8_t> frame) noexcept
        {
            for (std::size_t i = 0U; i < frame.size(); ++i) {
                frame[i] = static_cast<std::uint8_t>(i);
            }
            return ESP_OK;
        }
    };
    std::array<std::uint8_t, 100> gray{};
    const auto captured_gray = arc::vision::VSlam::capture_gray<VslamCamera>(gray);
    expect(captured_gray.has_value() && (*captured_gray)[7] == 7U,
           "VSlam captures grayscale frames into caller DMA span");
    for (auto& pixel : gray) {
        pixel = 10U;
    }
    constexpr std::array<std::array<int, 2>, 16> fast_circle{{
        {{0, -3}},
        {{1, -3}},
        {{2, -2}},
        {{3, -1}},
        {{3, 0}},
        {{3, 1}},
        {{2, 2}},
        {{1, 3}},
        {{0, 3}},
        {{-1, 3}},
        {{-2, 2}},
        {{-3, 1}},
        {{-3, 0}},
        {{-3, -1}},
        {{-2, -2}},
        {{-1, -3}},
    }};
    for (const auto& offset : fast_circle) {
        const auto yy = static_cast<std::size_t>(5 + offset[1]);
        const auto xx = static_cast<std::size_t>(5 + offset[0]);
        gray[(yy * 10U) + xx] = 240U;
    }
    std::array<arc::vision::Corner, 64> corners{};
    const auto fast = arc::vision::VSlam::fast_corners(gray, 10U, 10U, corners, 40U);
    const std::array<arc::vision::Corner, 2> prev_corners{
        arc::vision::Corner{.x = 10U, .y = 10U, .score = 12U},
        arc::vision::Corner{.x = 20U, .y = 12U, .score = 12U},
    };
    const std::array<arc::vision::Corner, 2> curr_corners{
        arc::vision::Corner{.x = 12U, .y = 11U, .score = 12U},
        arc::vision::Corner{.x = 22U, .y = 13U, .score = 12U},
    };
    const auto essential = arc::vision::VSlam::from_flow(prev_corners, curr_corners, 2U, 100.0F);
    arc::nav::Eskf<float>::State vision_state{};
    expect(fast.has_value() && !fast->empty() && essential.has_value() &&
               near(essential->essential(1, 2), -0.02F) &&
               arc::vision::VSlam::correct_filter(vision_state, essential->delta, 1.0F).has_value() &&
               near(vision_state.position[0], 0.02F, 0.001F),
           "VSlam extracts FAST corners, solves essential delta, and corrects ESKF");

    struct FtmPolicy {
        static esp_err_t ftm_initiate(
            const arc::net::FtmPeer peer,
            const arc::net::FtmSessionConfig config) noexcept
        {
            ftm_channel = peer.channel;
            ftm_frames = config.frames;
            return ESP_OK;
        }
    };
    const std::array<arc::net::FtmPeer, 4> ftm_peers{
        arc::net::FtmPeer{.node_id = 1U, .position = {.x_mm = 0.0F, .y_mm = 0.0F, .z_mm = 0.0F}, .channel = 6U},
        arc::net::FtmPeer{.node_id = 2U, .position = {.x_mm = 1000.0F, .y_mm = 0.0F, .z_mm = 0.0F}, .channel = 6U},
        arc::net::FtmPeer{.node_id = 3U, .position = {.x_mm = 0.0F, .y_mm = 1000.0F, .z_mm = 0.0F}, .channel = 6U},
        arc::net::FtmPeer{.node_id = 4U, .position = {.x_mm = 0.0F, .y_mm = 0.0F, .z_mm = 1000.0F}, .channel = 6U},
    };
    const arc::swarm::Position3 rf_point{.x_mm = 100.0F, .y_mm = 200.0F, .z_mm = 300.0F};
    const auto rf_range = [](const arc::swarm::Position3 anchor, const arc::swarm::Position3 point) noexcept {
        const auto dx = point.x_mm - anchor.x_mm;
        const auto dy = point.y_mm - anchor.y_mm;
        const auto dz = point.z_mm - anchor.z_mm;
        return std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
    };
    const std::array<arc::net::FtmMeasurement, 4> ftm_ranges{
        arc::net::FtmMeasurement{.node_id = 1U, .distance_mm = rf_range(ftm_peers[0].position, rf_point)},
        arc::net::FtmMeasurement{.node_id = 2U, .distance_mm = rf_range(ftm_peers[1].position, rf_point)},
        arc::net::FtmMeasurement{.node_id = 3U, .distance_mm = rf_range(ftm_peers[2].position, rf_point)},
        arc::net::FtmMeasurement{.node_id = 4U, .distance_mm = rf_range(ftm_peers[3].position, rf_point)},
    };
    ftm_channel = 0U;
    ftm_frames = 0U;
    const auto ftm_position = arc::net::Ftm::trilaterate(
        std::span<const arc::net::FtmPeer, 4>{ftm_peers},
        std::span<const arc::net::FtmMeasurement, 4>{ftm_ranges});
    expect(arc::net::Ftm::initiate<FtmPolicy>(ftm_peers[0], {.frames = 16U}).has_value() &&
               ftm_channel == 6U && ftm_frames == 16U &&
               arc::net::Ftm::measurement(9U, 10U).distance_mm > 0.0F &&
               ftm_position.has_value() && near(ftm_position->x_mm, rf_point.x_mm, 0.1F) &&
               near(ftm_position->y_mm, rf_point.y_mm, 0.1F) &&
               near(ftm_position->z_mm, rf_point.z_mm, 0.1F),
           "FTM starts ESP-IDF session policy and solves RF trilateration");

    struct MigrationPolicy {
        static esp_err_t send(
            const std::uint16_t peer,
            const std::span<const std::uint8_t> bytes) noexcept
        {
            migration_peer = peer;
            migration_bytes = bytes.size();
            return ESP_OK;
        }

        static esp_err_t resume_wasm(
            const std::uint32_t process,
            const std::uint32_t ip,
            const std::uint32_t sp,
            const std::span<std::uint8_t>) noexcept
        {
            migration_process = process;
            migration_ip = ip;
            migration_sp = sp;
            return ESP_OK;
        }
    };
    const std::array<std::uint8_t, 4> wasm_memory{1U, 2U, 3U, 4U};
    const auto migrate_decision = arc::swarm::Migrator::from_governor(
        {.action = arc::power::GovernorAction::boost, .predicted_slack = -5, .overrun_risk = true},
        77U);
    const auto migration_frame = arc::swarm::Migrator::snapshot<16>(
        3U,
        {
            .linear_memory = wasm_memory,
            .stack_pointer = 0x100U,
            .instruction_pointer = 0x200U,
            .fuel = 9U,
        });
    std::array<std::uint8_t, 16> resumed_memory{};
    migration_peer = 0U;
    migration_bytes = 0U;
    migration_process = 0U;
    expect(migrate_decision.migrate && migration_frame.has_value() &&
               arc::swarm::Migrator::teleport<MigrationPolicy>(migrate_decision.peer, *migration_frame).has_value() &&
               arc::swarm::Migrator::resume<MigrationPolicy>(*migration_frame, resumed_memory).has_value() &&
               migration_peer == 77U && migration_bytes > wasm_memory.size() &&
               migration_process == 3U && migration_ip == 0x200U && migration_sp == 0x100U &&
               resumed_memory[0] == 1U && resumed_memory[3] == 4U,
           "Migrator snapshots, teleports, and resumes WASM process state");

    struct ProfilerPolicy {
        static arc::Result<std::uint16_t> current_milliamps() noexcept
        {
            return profiler_current;
        }

        static arc::Result<std::uint32_t> program_counter() noexcept
        {
            return profiler_pc;
        }
    };
    profiler_current = 123U;
    profiler_pc = 0x40080000U;
    const auto power_sample = arc::power::Profiler::sample<ProfilerPolicy>(50U);
    const std::array<std::uint16_t, 2> current_lsb{10U, 20U};
    const std::array<std::uint32_t, 2> pcs{0x10U, 0x20U};
    std::array<arc::power::PowerSample, 2> power_samples{};
    std::array<std::uint8_t, 128> perfetto_json{};
    const auto interleaved = arc::power::Profiler::interleave(
        current_lsb,
        pcs,
        power_samples,
        {.sample_hz = 40'000U, .milliamps_per_lsb = 2.0F});
    const auto heatmap = power_sample.has_value()
        ? arc::power::Profiler::perfetto_counter(perfetto_json, *power_sample)
        : arc::Result<std::span<const std::uint8_t>>{arc::fail(ESP_ERR_INVALID_STATE)};
    const std::string_view heatmap_text{
        heatmap.has_value() ? reinterpret_cast<const char*>(heatmap->data()) : "",
        heatmap.has_value() ? heatmap->size() : 0U,
    };
    expect(power_sample.has_value() && power_sample->milliamps == 123U &&
               interleaved.has_value() && interleaved->size() == 2U &&
               (*interleaved)[1].tick == 25U && (*interleaved)[1].milliamps == 40U &&
               heatmap.has_value() && heatmap_text.find("milliamps") != std::string_view::npos,
           "Power Profiler interleaves 40kHz current samples with trace PCs");

    struct LiFiPolicy {
        static esp_err_t send(const std::span<const arc::optical::LiFiSymbol> symbols) noexcept
        {
            lifi_sent_symbols = symbols.size();
            return ESP_OK;
        }

        static esp_err_t arm_receiver() noexcept
        {
            ++lifi_armed;
            return ESP_OK;
        }
    };
    const std::array<std::uint8_t, 1> lifi_bytes{0b1010'0000U};
    std::array<arc::optical::LiFiSymbol, 8> lifi_symbols{};
    const auto lifi_encoded = arc::optical::LiFi::encode(lifi_bytes, lifi_symbols, {.half_ticks = 3U});
    expect(lifi_encoded.has_value() && lifi_encoded->size() == 8U &&
               (*lifi_encoded)[0].first_high && !(*lifi_encoded)[0].second_high &&
               !(*lifi_encoded)[1].first_high && (*lifi_encoded)[1].second_high &&
               arc::optical::LiFi::transmit<LiFiPolicy>(*lifi_encoded).has_value() &&
               arc::optical::LiFi::arm_receiver<LiFiPolicy>().has_value() &&
               lifi_sent_symbols == 8U && lifi_armed == 1U,
           "LiFi Manchester plan and policy hooks");

    struct JitPolicy {
        static std::uint32_t exit() noexcept
        {
            return arc::vm::PseudoXtensaJitPolicy::exit();
        }

        static std::uint32_t alu(const arc::vm::BpfInsn insn) noexcept
        {
            return arc::vm::PseudoXtensaJitPolicy::alu(insn);
        }

        static esp_err_t finalize(const std::span<const std::uint32_t> image) noexcept
        {
            jit_finalized_words = image.size();
            return ESP_OK;
        }
    };
    const std::array jit_program{
        arc::vm::BpfInsn::make(arc::vm::BpfOp::mov_imm, 0U, 0U, 0, 7),
        arc::vm::BpfInsn::make(arc::vm::BpfOp::add_imm, 0U, 0U, 0, 5),
        arc::vm::BpfInsn::make(arc::vm::BpfOp::exit),
    };
    std::array<std::uint32_t, 4> jit_image{};
    const auto translated = arc::vm::Jit::translate<JitPolicy>(jit_program, jit_image);
    expect(translated.has_value() && translated->size() == jit_program.size() &&
               jit_finalized_words == jit_program.size() &&
               (*translated)[0] == arc::vm::PseudoXtensaJitPolicy::alu(jit_program[0]),
           "BPF JIT emits bounded executable image");

    struct WasmPolicy {
        static std::uint32_t i32_const(const std::int32_t value) noexcept
        {
            return 0x4100'0000U | static_cast<std::uint32_t>(value & 0xffff);
        }

        static std::uint32_t i32_add() noexcept
        {
            return 0x6aU;
        }

        static std::uint32_t i32_sub() noexcept
        {
            return 0x6bU;
        }

        static std::uint32_t i32_mul() noexcept
        {
            return 0x6cU;
        }

        static std::uint32_t end() noexcept
        {
            return 0x0bU;
        }

        static esp_err_t finalize(const std::span<const std::uint32_t> image) noexcept
        {
            wasm_finalized_words = image.size();
            return ESP_OK;
        }
    };
    struct WasmGuardPolicy {
        static esp_err_t configure(const std::span<const arc::WorldRegion> regions) noexcept
        {
            wasm_guard_regions = regions.size();
            return ESP_OK;
        }
    };
    const std::array<std::uint8_t, 6> wasm_code{
        0x41U,
        0x07U,
        0x41U,
        0x05U,
        0x6aU,
        0x0bU,
    };
    std::array<std::uint32_t, 8> wasm_image{};
    const auto wasm_translated = arc::vm::WasmAot::translate<WasmPolicy>(wasm_code, wasm_image);
    expect(wasm_translated.has_value() && wasm_translated->size() == 4U &&
               (*wasm_translated)[0] == 0x4100'0007U && (*wasm_translated)[2] == 0x6aU &&
               wasm_finalized_words == 4U &&
               arc::vm::WasmSandbox::protect<WasmGuardPolicy>(*wasm_translated).has_value() &&
               wasm_guard_regions == 1U,
           "WasmAot translates bounded WASM ops into protected executable image");

    struct BlackBoxHash {
        static void begin() noexcept
        {
            blackbox_hash_acc = 0U;
        }

        static void update(const std::span<const std::uint8_t> bytes) noexcept
        {
            for (const auto byte : bytes) {
                blackbox_hash_acc += byte;
            }
        }

        static esp_err_t finish(const std::span<std::uint8_t, 4> out) noexcept
        {
            out[0] = static_cast<std::uint8_t>(blackbox_hash_acc & 0xffU);
            out[1] = static_cast<std::uint8_t>((blackbox_hash_acc >> 8U) & 0xffU);
            out[2] = 0U;
            out[3] = 0U;
            return ESP_OK;
        }
    };
    struct BlackBoxStore {
        static esp_err_t write_encrypted(const std::uint32_t, const std::span<const std::uint8_t> bytes) noexcept
        {
            blackbox_write_bytes += bytes.size();
            return ESP_OK;
        }
    };
    blackbox_write_bytes = 0U;
    const std::array<std::uint8_t, 4> previous_root{};
    const std::array<std::uint8_t, 2> die_key{1U, 2U};
    const std::array<std::uint8_t, 3> flight_record{3U, 4U, 5U};
    const auto sealed = arc::covert::BlackBox::seal<BlackBoxHash, 4>(
        9U,
        flight_record,
        std::span<const std::uint8_t, 4>{previous_root},
        die_key);
    expect(sealed.has_value() &&
               arc::covert::BlackBox::append<BlackBoxStore>(32U, *sealed, flight_record).has_value() &&
               sealed->payload_bytes == flight_record.size() &&
               blackbox_write_bytes == sizeof(*sealed) + flight_record.size(),
           "BlackBox seals Merkle-linked encrypted record");

    using Twin = arc::hil::DigitalTwin<float, 2, 1, 1>;
    struct TwinCapture {
        static arc::Result<std::uint32_t> sample_ticks() noexcept
        {
            return 77U;
        }
    };
    struct TwinEncoder {
        static esp_err_t emit(const std::span<const float, 1> output) noexcept
        {
            digital_twin_last_output = output[0];
            return ESP_OK;
        }
    };
    Twin::State twin_state{};
    Twin::Model twin_model{};
    twin_model.d[0][0] = 2.0F;
    const auto twin = Twin::tick<TwinCapture, TwinEncoder>(twin_state, twin_model, Twin::InputVec{3.0F});
    expect(twin.has_value() && near(twin->output[0], 6.0F) &&
               near(digital_twin_last_output, 6.0F) && twin->captured_ticks == 77U,
           "DigitalTwin captures PWM and emits simulated encoder state");

    const auto drive_cb = +[](const int left, const int right) noexcept -> arc::Status {
        cli_drive_left = left;
        cli_drive_right = right;
        return arc::ok();
    };
    const auto kp_cb = +[](const float value) noexcept -> arc::Status {
        cli_kp = value;
        return arc::ok();
    };
    const arc::Cli<arc::Command<"drive", int, int>, arc::Command<"set_kp", float>> cli{
        .commands = {arc::Command<"drive", int, int>{drive_cb}, arc::Command<"set_kp", float>{kp_cb}},
    };
    constexpr std::string_view drive_line = "drive -3 7";
    constexpr std::string_view kp_line = "set_kp 1.5";
    expect(cli.parse(std::span<const char>{drive_line.data(), drive_line.size()}).has_value() &&
               cli.parse(std::span<const char>{kp_line.data(), kp_line.size()}).has_value() &&
               cli_drive_left == -3 && cli_drive_right == 7 && near(cli_kp, 1.5F),
           "Cli parses fixed command arguments without mutation");

    struct SdioPolicy {
        static esp_err_t sdio_start(const arc::SdioSlaveConfig config) noexcept
        {
            return config.block_bytes == 512U ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static esp_err_t sdio_queue(const std::span<std::uint8_t> buffer) noexcept
        {
            sdio_queued_bytes = buffer.size();
            return ESP_OK;
        }

        static arc::Result<arc::SdioSlaveTransfer> sdio_finish(const std::uint32_t) noexcept
        {
            return arc::SdioSlaveTransfer{.address = 0x1000U, .bytes = sdio_queued_bytes};
        }

        static esp_err_t sdio_interrupt(const std::uint32_t bits) noexcept
        {
            sdio_irq_bits = bits;
            return ESP_OK;
        }
    };
    std::array<std::uint8_t, 32> sdio_buffer{};
    expect(arc::SdioSlave::start<SdioPolicy>().has_value() &&
               arc::SdioSlave::queue_coherent<SdioPolicy>(sdio_buffer).has_value(),
           "SdioSlave queues coherent DMA");
    const auto sdio_done = arc::SdioSlave::finish_coherent<SdioPolicy>(10U);
    expect(
        sdio_done.has_value() &&
            arc::SdioSlave::interrupt_host<SdioPolicy>(0x2U).has_value() &&
            sdio_done->bytes == sdio_buffer.size() && sdio_queued_bytes == sdio_buffer.size() && sdio_irq_bits == 0x2U,
        "SdioSlave finishes coherent DMA and host interrupt");

    struct UsbDevicePolicy {
        static esp_err_t ep0_write(const std::span<const std::uint8_t> bytes) noexcept
        {
            usb_ep0_bytes = bytes.size();
            return ESP_OK;
        }

        static esp_err_t set_address(const std::uint8_t address) noexcept
        {
            usb_device_address = address;
            return ESP_OK;
        }

        static esp_err_t configured(const std::uint8_t configuration) noexcept
        {
            usb_configuration = configuration;
            return ESP_OK;
        }
    };
    arc::usb::Device<arc::usb::Cdc<0x83U, 0x01U, 0x82U>, UsbDevicePolicy> usb_ch9{
        .device = {.vendor = 0x1209U, .product = 0x0002U},
    };
    expect(usb_ch9.setup({.request = static_cast<std::uint8_t>(arc::usb::StandardRequest::get_descriptor),
                          .value = static_cast<std::uint16_t>(static_cast<std::uint16_t>(arc::usb::DescriptorType::device) << 8U),
                          .length = 18U})
                   .has_value() &&
               usb_ep0_bytes == 18U &&
               usb_ch9.setup({.request = static_cast<std::uint8_t>(arc::usb::StandardRequest::set_address),
                              .value = 5U})
                   .has_value() &&
               usb_device_address == 5U && usb_ch9.state == arc::usb::DeviceState::addressed &&
               usb_ch9.setup({.request = static_cast<std::uint8_t>(arc::usb::StandardRequest::set_configuration),
                              .value = 1U})
                   .has_value() &&
               usb_configuration == 1U && usb_ch9.state == arc::usb::DeviceState::configured,
           "USB Device handles Chapter 9 address/configure requests");

    struct ThreadPolicy {
        static esp_err_t thread_attach(const arc::net::ThreadDataset& dataset) noexcept
        {
            return dataset.channel == 15U ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static esp_err_t thread_send(const arc::net::ThreadPeer& peer, const std::span<const std::uint8_t>) noexcept
        {
            thread_peer_rloc = peer.rloc16;
            return ESP_OK;
        }
    };
    const arc::net::ThreadDataset thread_dataset{.pan_id = 0x1234U, .channel = 15U};
    const arc::net::ThreadPeer thread_peer{.rloc16 = 0x4400U};
    expect(arc::net::Thread::attach<ThreadPolicy>(thread_dataset).has_value() &&
               arc::net::Thread::send<ThreadPolicy>(thread_peer, rdma_payload).has_value() &&
               thread_peer_rloc == thread_peer.rloc16,
           "Thread policy bridge validates dataset and peer send");

    struct BleMeshPolicy {
        static esp_err_t mesh_provision(const arc::ble::MeshAddress address) noexcept
        {
            return address.unicast == 1U ? ESP_OK : ESP_ERR_INVALID_ARG;
        }

        static esp_err_t mesh_publish(
            const arc::ble::MeshAddress address,
            const arc::ble::MeshModel,
            const std::span<const std::uint8_t>) noexcept
        {
            ble_mesh_group = address.group;
            return ESP_OK;
        }
    };
    const arc::ble::MeshAddress mesh_address{.unicast = 1U, .group = 0xc001U};
    expect(arc::ble::Mesh::provision<BleMeshPolicy>(mesh_address).has_value() &&
               arc::ble::Mesh::publish<BleMeshPolicy>(mesh_address, {.company = 0x02E5U, .model = 1U}, rdma_payload).has_value() &&
               ble_mesh_group == mesh_address.group,
           "BLE Mesh policy bridge provisions and publishes");

    struct IpcPolicy {
        static esp_err_t ipc_call(const arc::IpcCore core, arc::Ipc::Fn fn, void* arg) noexcept
        {
            ipc_core = core;
            fn(arg);
            return ESP_OK;
        }

        static esp_err_t ipc_halt(const arc::IpcCore core) noexcept
        {
            ipc_core = core;
            return ESP_OK;
        }
    };
    const auto ipc_fn = +[](void*) noexcept {
        ipc_called = true;
    };
    ipc_called = false;
    expect(arc::Ipc::call<IpcPolicy>(arc::IpcCore::core1, ipc_fn).has_value() &&
               ipc_called && ipc_core == arc::IpcCore::core1 &&
               arc::Ipc::emergency_halt<IpcPolicy>(arc::IpcCore::core0).has_value() &&
               ipc_core == arc::IpcCore::core0,
           "Ipc policy bridge calls opposite core hooks");

    struct CertPolicy {
        static esp_err_t bundle_attach(const arc::x509::CertBundle bundle) noexcept
        {
            cert_bundle_bytes = bundle.der.size();
            return ESP_OK;
        }
    };
    const std::array<std::uint8_t, 3> cert_der{0x30U, 0x01U, 0x00U};
    expect(arc::x509::Bundle::attach<CertPolicy>({.der = cert_der, .certificates = 1U}).has_value() &&
               cert_bundle_bytes == cert_der.size(),
           "x509 bundle policy attaches DER roots");

    struct NvsPolicy {
        static esp_err_t nvs_mount_encrypted(const arc::NvsCryptoConfig config) noexcept
        {
            nvs_crypto_partition = config.partition;
            return ESP_OK;
        }
    };
    expect(arc::NvsCrypto::mount<NvsPolicy>({.partition = "cfg", .key_partition = "keys", .generate_keys = true}).has_value() &&
               nvs_crypto_partition == "cfg",
           "NVS crypto mounts encrypted partition policy");

    struct SecureBootPolicy {
        static arc::Result<arc::secure::BootState> boot_state() noexcept
        {
            return arc::secure::BootState{.enabled = true, .digest_valid = true};
        }

        static esp_err_t boot_revoke(const std::uint8_t key) noexcept
        {
            boot_revoked = key;
            return ESP_OK;
        }

        static arc::Result<arc::secure::BootDigest> boot_digest() noexcept
        {
            arc::secure::BootDigest digest{};
            digest.sha256[0] = 0xA5U;
            return digest;
        }
    };
    const auto boot_state = arc::secure::SecureBoot::state<SecureBootPolicy>();
    const auto boot_digest = arc::secure::SecureBoot::digest<SecureBootPolicy>();
    expect(boot_state.has_value() && boot_state->enabled && boot_state->digest_valid &&
               arc::secure::SecureBoot::revoke<SecureBootPolicy>(2U).has_value() &&
               boot_revoked == 2U && boot_digest.has_value() && boot_digest->sha256[0] == 0xA5U,
           "SecureBoot exposes state revoke and digest hooks");
}

void test_hive_goal_surfaces()
{
    using Matrix = arc::swarm::HyperMatrix<2, 1, 1, 1, 1, 1>;
    const std::array<arc::swarm::Pose6, Matrix::cells> grid{
        arc::swarm::Pose6{.x_mm = 0.0F, .time_us = 10},
        arc::swarm::Pose6{.x_mm = 100.0F, .time_us = 10},
    };
    auto hyper = Matrix::seeded(std::span<const arc::swarm::Pose6, Matrix::cells>{grid});
    expect(hyper.observe(Matrix::from_vslam({.tx_mm = 100.0F, .tracks = 3U}, 10, 4.0F)).has_value() &&
               hyper.observe(Matrix::from_ftm({.x_mm = 100.0F}, 11, 4.0F)).has_value() &&
               hyper.observe(Matrix::from_acoustic({.x_mm = 100.0F}, 12, 16.0F)).has_value(),
           "HyperMatrix fuses visual RF and acoustic observations");
    const auto hyper_pose = hyper.estimate();
    struct HyperPolicy {
        static esp_err_t send(const std::uint32_t node, const std::span<const std::uint8_t> bytes) noexcept
        {
            hypermatrix_node = node;
            hypermatrix_bytes = bytes.size();
            return ESP_OK;
        }
    };
    alignas(16) std::array<std::uint8_t, 128> hyper_remote{};
    std::array<std::uint8_t, 128> hyper_scratch{};
    const arc::net::RdmaWindow hyper_window{
        .node = 8U,
        .base = reinterpret_cast<std::uintptr_t>(hyper_remote.data()),
        .bytes = hyper_remote.size(),
    };
    hypermatrix_node = 0U;
    hypermatrix_bytes = 0U;
    expect(hyper_pose.has_value() && hyper_pose->x_mm > 80.0F &&
               hyper.publish_rdma<HyperPolicy>(hyper_window, 1U, hyper_scratch).has_value() &&
               hypermatrix_node == 8U && hypermatrix_bytes > sizeof(arc::swarm::HyperMatrixHeader),
           "HyperMatrix publishes shared 6D tensor state through RDMA frame");

    const auto corexy = arc::cnc::Kinematics::corexy(10.0F, 5.0F, {.steps_mm = 80.0F, .ticks_step = 2U});
    const auto delta = arc::cnc::Kinematics::delta({0.0F, 0.0F, 10.0F}, {.arm_mm = 120.0F, .radius_mm = 30.0F, .steps_mm = 10.0F});
    const auto five = arc::cnc::Kinematics::five_axis(
        {1.0F, 2.0F, 3.0F, 4.0F, 5.0F},
        {10.0F, 10.0F, 10.0F, 10.0F, 2.0F});
    const std::string_view line{"N7 G1 X10 Y5 F1200"};
    const auto block = arc::cnc::GCode::parse_line(std::span<const char>{line.data(), line.size()});
    std::array<arc::MotionStep<2>, 128> cnc_steps{};
    const auto planned = block.has_value()
        ? arc::cnc::GCode::plan_linear<2>(*block, {0.0F, 0.0F}, 10.0F, cnc_steps)
        : arc::Result<std::span<const arc::MotionStep<2>>>{arc::fail(ESP_ERR_INVALID_STATE)};
    expect(corexy.delta[0] == 1200 && corexy.delta[1] == 400 && corexy.ticks_step == 2U &&
               delta.has_value() && delta->delta[0] > 0 &&
               five.delta[0] == 10 && five.delta[4] == 10 &&
               block.has_value() && block->command == arc::cnc::Command::linear &&
               block->line == 7U && block->has_axis[0] && block->axis[0] == 10.0F &&
               planned.has_value() && planned->size() == 100U,
           "CNC kinematics and zero-allocation G-code feed MotionPlan");

    const std::array<int, 4> coeffs{1, 2, 3, 4};
    const std::array<int, 4> samples{2, 2, 2, 2};
    const auto fir = arc::hls::fir<int, 4>(
        std::span<const int, 4>{coeffs},
        std::span<const int, 4>{samples});
    const auto dot = arc::hls::dot<int, 4>(
        std::span<const int, 4>{coeffs},
        std::span<const int, 4>{samples});
    constexpr auto dense_spec = arc::hls::DenseSpec<4, 2>::hls_spec();
    expect(fir.has_value() && *fir == 20 && dot.has_value() && *dot == 20 &&
               dense_spec.static_bounds && dense_spec.heapless &&
               dense_spec.latency_cycles == 8U,
           "HLS helpers expose fixed-bound heapless kernel metadata");
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
    test_uri();
    test_http_client();
    test_uri_dial_helpers();
    test_codec_fuzzer_smoke();
    test_invalid_codecs();
    test_http_server();
    test_caps();
    test_dsp();
    test_foc_motion_tdma();
    test_ml_tensor();
    test_simd_ml_pipeline();
    test_csi_hotpatch();
    test_netrpc_pms_secure_update();
    test_matrix_kalman_storage_swarm();
    test_trace_provision_ethernet_flashoff_hil_ecs();
    test_log_lane();
    test_text();
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
    test_pipeline_usb_ulp();
    test_pru_isp_vision();
    test_vm_bpf();
    test_ulp_ml_paillier_covert();
    test_goal_wave_surfaces();
    test_resilient_edge_goal_surfaces();
    test_current_goal_surfaces();
    test_hive_goal_surfaces();
    test_refinit();
    std::puts("arc host tests: OK");
}
