#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
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
#include "arc/axi_graph.hpp"
#include "arc/bare_core.hpp"
#include "arc/bft.hpp"
#include "arc/ble_mesh.hpp"
#include "arc/blackbox.hpp"
#include "arc/borrow.hpp"
#include "arc/burst.hpp"
#include "arc/caps.hpp"
#include "arc/cert_bundle.hpp"
#include "arc/chaos.hpp"
#include "arc/cli.hpp"
#include "arc/claim.hpp"
#include "arc/concepts.hpp"
#include "arc/coro.hpp"
#include "arc/covert.hpp"
#include "arc/crdt.hpp"
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
#include "arc/flow.hpp"
#include "arc/foc.hpp"
#include "arc/fram.hpp"
#include "arc/fs.hpp"
#include "arc/fsm.hpp"
#include "arc/hil.hpp"
#include "arc/hotpatch.hpp"
#include "arc/hotswap.hpp"
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
#include "arc/lockstep.hpp"
#include "arc/lp_core.hpp"
#include "arc/log.hpp"
#include "arc/copy.hpp"
#include "arc/core.hpp"
#include "arc/cnc.hpp"
#include "arc/fabric.hpp"
#include "arc/hls.hpp"
#include "arc/hyper_matrix.hpp"
#include "arc/math.hpp"
#include "arc/matrix.hpp"
#include "arc/mcap.hpp"
#include "arc/migrator.hpp"
#include "arc/maglev.hpp"
#include "arc/memory.hpp"
#include "arc/motion.hpp"
#include "arc/mmu_span.hpp"
#include "arc/ml.hpp"
#include "arc/mqtt.hpp"
#include "arc/mpsc.hpp"
#include "arc/nav.hpp"
#include "arc/net_codecs.hpp"
#include "arc/netrpc.hpp"
#include "arc/nvs_crypto.hpp"
#include "arc/ota.hpp"
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
#include "arc/proof.hpp"
#include "arc/provisioning.hpp"
#include "arc/pru.hpp"
#include "arc/puf.hpp"
#include "arc/rcu.hpp"
#include "arc/rdma.hpp"
#include "arc/rpc.hpp"
#include "arc/roles.hpp"
#include "arc/rtos.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/spi.hpp"
#include "arc/spi_slave.hpp"
#include "arc/stream.hpp"
#include "arc/secure_update.hpp"
#include "arc/sdr.hpp"
#include "arc/scrub.hpp"
#include "arc/simd.hpp"
#include "arc/sdio_slave.hpp"
#include "arc/sim.hpp"
#include "arc/snn.hpp"
#include "arc/stack.hpp"
#include "arc/store.hpp"
#include "arc/secure_boot.hpp"
#include "arc/star_tracker.hpp"
#include "arc/swarm.hpp"
#include "arc/task.hpp"
#include "arc/tdma.hpp"
#include "arc/tee.hpp"
#include "arc/thread.hpp"
#include "arc/timesync.hpp"
#include "arc/tcp.hpp"
#include "arc/text.hpp"
#include "arc/tls.hpp"
#include "arc/tsn.hpp"
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
#include "arc/vision_accel.hpp"
#include "arc/vslam.hpp"
#include "arc/ftm.hpp"
#include "arc/wavefront.hpp"
#include "arc/ws.hpp"
#include "arc/w5500.hpp"
#include "arc/xrce.hpp"

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
static_assert(arc::soc::Esp32P4::cores == 2U);
static_assert(arc::soc::Esp32P4::mhz == 400U);
static_assert(!arc::soc::Esp32P4::wifi);
static_assert(arc::soc::Esp32P4::ethernet_mac);
static_assert(arc::soc::Esp32P4::dma2d);
static_assert(arc::soc::Esp32P4::ppa);
static_assert(arc::soc::Esp32P4::jpeg);
static_assert(arc::soc::Esp32P4::h264);
static_assert(arc::soc::has<arc::soc::Cap::ptp, arc::soc::Esp32P4>);
static_assert(!arc::soc::has<arc::soc::Cap::drive, arc::soc::Esp32P4>);
static_assert(arc::stack::round_up(std::numeric_limits<std::size_t>::max() - 3U, 8U) ==
              std::numeric_limits<std::size_t>::max());
static_assert(arc::stack::budget<std::numeric_limits<std::size_t>::max(), 1U, 0U, 0U>() ==
              std::numeric_limits<std::size_t>::max());
static_assert(arc::stack::storage<std::uint16_t,
                                  (std::numeric_limits<std::size_t>::max() / sizeof(std::uint16_t)) + 1U>() ==
              std::numeric_limits<std::size_t>::max());
static_assert(arc::stack::objects<std::uint8_t, std::uint16_t, std::uint32_t>() == 7U);
static_assert(arc::words(0U) == 0U);
static_assert(arc::words(std::numeric_limits<std::size_t>::max()) ==
              ((std::numeric_limits<std::size_t>::max() - 1U) / sizeof(StackType_t)) + 1U);
static_assert(arc::Burst<1, 1'000'000>::symbol(2U, true, 3U, false).val == 0x00038002U);
static_assert(arc::covert::Fsk::fits(1U, 8U));
static_assert(!arc::covert::Fsk::fits((std::numeric_limits<std::size_t>::max() / 8U) + 1U,
                                      std::numeric_limits<std::size_t>::max()));
static_assert(arc::optical::LiFi::fits(1U, 8U));
static_assert(!arc::optical::LiFi::fits((std::numeric_limits<std::size_t>::max() / 8U) + 1U,
                                        std::numeric_limits<std::size_t>::max()));
static_assert(arc::sdr::PulseSynth::fits(1U, 8U));
static_assert(!arc::sdr::PulseSynth::fits((std::numeric_limits<std::size_t>::max() / 8U) + 1U,
                                          std::numeric_limits<std::size_t>::max()));
static_assert(sizeof(arc::net::EthernetFrame<std::numeric_limits<std::uint16_t>::max()>::size) == sizeof(std::uint16_t));
static_assert(sizeof(decltype(arc::claim_token<1, 2, 3>())) == sizeof(std::uint64_t));
static_assert(arc::claim_token<1, 2, 3>() != arc::claim_token<1, 2, 4>());
static_assert(arc::claim_proof<1, 2, 3>() != arc::claim_proof<1, 2, 4>());
using BoardPins = arc::Pins<2, 4, -1, 18>;
static_assert(BoardPins::count == 4U);
static_assert(BoardPins::has<4>());
static_assert(!BoardPins::has<5>());
static_assert(!BoardPins::has<-1>());
static_assert(BoardPins::index<18>() == 3U);
static_assert(BoardPins::index<5>() == BoardPins::npos);
static_assert(arc::Result<int>{2}.and_then([](int value) { return arc::Result<int>{value + 1}; }).value() == 3);
static_assert(arc::status_code(arc::ok()) == ESP_OK);

[[nodiscard]] arc::Result<int> result_value(const bool pass) noexcept
{
    return pass ? arc::ok(41) : arc::Result<int>{arc::fail(ESP_ERR_INVALID_ARG)};
}

[[nodiscard]] arc::Status result_status(const bool pass) noexcept
{
    return pass ? arc::ok() : arc::Status{arc::fail(ESP_ERR_INVALID_STATE)};
}

[[nodiscard]] arc::Result<int> result_flow(const bool value_ok, const bool status_ok) noexcept
{
    ARC_TRY(value, result_value(value_ok));
    ARC_CHECK(result_status(status_ok));
    return arc::ok(value + 1);
}

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

void test_result()
{
    const auto ok = result_flow(true, true);
    const auto value_err = result_flow(false, true);
    const auto status_err = result_flow(true, false);

    expect(ok.has_value() && *ok == 42, "Result ARC_TRY unwraps value");
    expect(!value_err.has_value() && value_err.error() == ESP_ERR_INVALID_ARG, "Result ARC_TRY propagates error");
    expect(!status_err.has_value() && status_err.error() == ESP_ERR_INVALID_STATE, "Result ARC_CHECK propagates error");
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

ARC_LP_CORE void arc_host_lp_entry() noexcept {}

ARC_LP_SHARED arc::lp::Word arc_host_lp_word{};
std::size_t hotpatch_stores{};
std::size_t hotpatch_syncs{};
std::size_t hotpatch_unlocks{};
std::size_t hotswap_verified{};
std::size_t hotswap_protected_words{};
std::uint32_t hotswap_activated_version{};
std::uint32_t covert_hz{};
std::uint32_t covert_duty{};
int covert_density{};
bool semantic_wake{};
std::array<std::byte, 64> fram_storage{};
std::size_t fram_reads{};
std::size_t fram_writes{};
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
std::uint32_t sdio_finish_timeout{};
std::uint8_t usb_device_address{};
std::uint8_t usb_configuration{};
std::size_t usb_ep0_bytes{};
std::uint8_t usb_ep0_first{};
std::uint8_t usb_class_request{};
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

struct PendingCryptoDmaPolicy {
    static inline bool complete{};

    [[nodiscard]] static esp_err_t submit(const arc::CryptoDmaJob&) noexcept
    {
        complete = false;
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

struct NullMmuPolicy {
    [[nodiscard]] static arc::Result<arc::MmuRegion> map(const arc::MmuMapRequest& request) noexcept
    {
        return arc::ok(arc::MmuRegion{
            .address = nullptr,
            .bytes = request.bytes,
            .handle = 8U,
        });
    }

    static void unmap(const arc::MmuRegion&) noexcept {}
};

struct ShortMmuPolicy {
    [[nodiscard]] static arc::Result<arc::MmuRegion> map(const arc::MmuMapRequest& request) noexcept
    {
        return arc::ok(arc::MmuRegion{
            .address = MockMmuPolicy::flash.data(),
            .bytes = request.bytes - sizeof(std::uint32_t),
            .handle = 9U,
        });
    }

    static void unmap(const arc::MmuRegion&) noexcept {}
};

struct OddMmuPolicy {
    [[nodiscard]] static arc::Result<arc::MmuRegion> map(const arc::MmuMapRequest& request) noexcept
    {
        return arc::ok(arc::MmuRegion{
            .address = MockMmuPolicy::flash.data(),
            .bytes = request.bytes + 1U,
            .handle = 10U,
        });
    }

    static void unmap(const arc::MmuRegion&) noexcept {}
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
    MockStaticI2c::sent = 99U;
    expect(i2c.send(nullptr, 0U, 76) == ESP_OK && MockStaticI2c::sent == 99U,
           "AnyI2c empty send is no-op");
    expect(i2c.send(std::span(tx), 77) == ESP_OK, "AnyI2c span send");
    expect(MockStaticI2c::sent == tx.size() && MockStaticI2c::timeout == 77, "AnyI2c send thunks");
    MockStaticI2c::received = 99U;
    expect(i2c.recv(nullptr, 0U, 78) == ESP_OK && MockStaticI2c::received == 99U,
           "AnyI2c empty recv is no-op");
    expect(i2c.recv(std::span(rx), 78) == ESP_OK, "AnyI2c span recv");
    expect(rx[0] == 0x5A && rx[2] == 0x5A && MockStaticI2c::received == rx.size(), "AnyI2c recv thunks");
    MockStaticI2c::xfer_tx = 99U;
    MockStaticI2c::xfer_rx = 99U;
    expect(i2c.xfer(nullptr, 0U, nullptr, 0U, 79) == ESP_OK && MockStaticI2c::xfer_tx == 99U &&
               MockStaticI2c::xfer_rx == 99U,
           "AnyI2c empty xfer is no-op");
    expect(i2c.xfer(std::span(tx), std::span(rx), 79) == ESP_OK, "AnyI2c span xfer");
    expect(rx[0] == 0xEFU && rx[2] == 0xCFU, "AnyI2c xfer transforms");
    expect(MockStaticI2c::xfer_tx == tx.size() && MockStaticI2c::xfer_rx == rx.size(), "AnyI2c xfer lengths");

    MockSpi mock;
    const auto spi = arc::AnySpi::bind(mock);
    expect(static_cast<bool>(spi), "object AnySpi binds");
    mock.sent = 99U;
    expect(spi.send(nullptr, 0U, 500'000U, 1U) == ESP_OK && mock.sent == 99U,
           "AnySpi empty send is no-op");
    expect(spi.send(std::span(tx), 1'000'000U, 3U) == ESP_OK, "AnySpi span send");
    expect(mock.sent == tx.size() && mock.hz == 1'000'000U && mock.flags == 3U, "AnySpi send thunks");
    mock.received = 99U;
    expect(spi.recv(nullptr, 0U, 1'500'000U, 2U) == ESP_OK && mock.received == 99U,
           "AnySpi empty recv is no-op");
    rx = {};
    expect(spi.xfer(std::span(tx), std::span(rx), 2'000'000U, 4U) == ESP_OK, "AnySpi span xfer");
    expect(rx == tx && mock.xfer_bytes == tx.size() && mock.hz == 2'000'000U, "AnySpi xfer copies");
    mock.xfer_bytes = 99U;
    expect(spi.xfer(nullptr, nullptr, 0U, 3'000'000U, 5U) == ESP_OK && mock.xfer_bytes == 99U,
           "AnySpi empty xfer is no-op");
    expect(spi.xfer(std::span(tx), std::span(rx).first(2U)) == ESP_ERR_INVALID_ARG, "AnySpi rejects mismatched spans");

    MockUart uart_mock;
    const auto uart = arc::AnyUart::bind(uart_mock);
    expect(static_cast<bool>(uart), "object AnyUart binds");
    uart_mock.wrote = 99U;
    const auto empty_write = uart.write(nullptr, 0U);
    expect(empty_write.has_value() && *empty_write == 0U && uart_mock.wrote == 99U,
           "AnyUart empty write is no-op");
    const auto wrote = uart.write(std::span(tx));
    expect(wrote.has_value() && *wrote == tx.size() && uart_mock.wrote == tx.size(), "AnyUart writes");
    std::array<std::uint8_t, 4> serial_rx{};
    const auto empty_read = uart.read(nullptr, 0U, 41U);
    expect(empty_read.has_value() && *empty_read == 0U && uart_mock.read_pos == 0U,
           "AnyUart empty read is no-op");
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

template <typename T>
concept HasRootPush = requires(T& roles, int in) { roles.try_push(in); };

template <typename T>
concept HasRootPop = requires(T& roles, int& out) { roles.try_pop(out); };

template <typename T>
concept HasRootFaninPush = requires(T& roles, int in) { roles.template try_push<0>(in); };

template <typename T, typename Op, typename Payload>
concept HasRootRpcCall = requires(T& roles, Op op, Payload payload) { roles.call(op, payload, 1U); };

template <typename T, typename Request>
concept HasRootRpcRecv = requires(T& roles, Request& request) { roles.recv(request); };

template <typename T, typename Reply>
concept HasRootRpcPoll = requires(T& roles, Reply& reply) { roles.poll(reply); };

template <typename T, typename Reply>
concept HasRootRpcPollMatch = requires(T& roles, Reply& reply) { roles.poll_match(1U, reply); };

template <typename T>
concept HasDmaView = requires(T& buffer) { buffer.view(); };

template <typename T>
concept HasDmaIndex = requires(T& buffer) { buffer[0]; };

template <typename T, arc::Core Access>
concept HasCoreGet = requires(T& local) { local.template get<Access>(); };

template <typename T, arc::Core Access, arc::Core To>
concept HasCoreMsg = requires(const T& local) { local.template msg<Access, To>(); };

template <typename T, arc::Core To>
concept HasCoreMsgTo = requires(const T& local) { local.template msg<To>(); };

template <typename T, typename Msg>
concept HasCoreAccept = requires(T& local, const Msg& msg) { local.accept(msg); };

template <typename T, arc::Core Access>
concept HasMsgGet = requires(const T& msg) { msg.template get<Access>(); };

template <typename T>
concept HasMsgGetInferred = requires(const T& msg) { msg.get(); };

template <typename T>
concept HasLoanArrow = requires(T& loan) { loan.operator->(); };

template <typename T>
concept HasConstLoanArrow = requires(const T& loan) { loan.operator->(); };

template <typename T>
concept HasConstLoanDeref = requires(const T& loan) { *loan; };

struct FlowPacket {
    std::uint32_t seq{};
};

struct FlowSource {
    using value_type = FlowPacket;

    static inline std::uint32_t next{};
    static inline std::uint32_t limit{};

    [[nodiscard]] static bool read(value_type& out) noexcept
    {
        if (next >= limit) {
            return false;
        }

        out.seq = ++next;
        return true;
    }
};

struct FlowSink {
    static inline std::uint32_t sum{};
    static inline std::uint32_t writes{};
    static inline bool ready{true};

    [[nodiscard]] static bool write(const FlowPacket& value) noexcept
    {
        if (!ready) {
            return false;
        }

        sum += value.seq;
        ++writes;
        return true;
    }
};

void test_flow()
{
    using Lane = arc::Spsc<FlowPacket, 4>;
    static_assert(std::same_as<Lane::value_type, FlowPacket>);
    static_assert(arc::FlowSource<FlowSource, FlowPacket>);
    static_assert(arc::FlowLane<Lane, FlowPacket>);
    static_assert(arc::FlowLane<arc::Mpsc<FlowPacket, 4>, FlowPacket>);
    static_assert(arc::FlowLane<arc::CheckedSpsc<FlowPacket, 4>, FlowPacket>);
    static_assert(arc::FlowLane<arc::CheckedMpsc<FlowPacket, 4>, FlowPacket>);
    static_assert(arc::FlowSink<FlowSink, FlowPacket>);

    FlowSource::next = 0U;
    FlowSource::limit = 4U;
    FlowSink::sum = 0U;
    FlowSink::writes = 0U;
    FlowSink::ready = true;

    arc::Flow<FlowSource, Lane, FlowSink> flow{};
    expect(flow.cap() == 3U, "Flow exposes lane capacity");
    expect(flow.fill() && flow.fill() && flow.fill(), "Flow fills static lane");
    expect(!flow.fill() && flow.pending() && FlowSource::next == 4U, "Flow holds pending value under backpressure");
    expect(flow.drain() && FlowSink::sum == 1U, "Flow drains one item to sink");
    expect(flow.fill() && !flow.pending(), "Flow queues pending value after room opens");
    expect(flow.drain(8U) == 3U && FlowSink::sum == 10U && FlowSink::writes == 4U, "Flow drains remaining items");

    FlowSource::next = 0U;
    FlowSource::limit = 1U;
    FlowSink::sum = 0U;
    FlowSink::writes = 0U;
    FlowSink::ready = true;
    auto step = flow.step();
    expect(step.queued && step.wrote && !step.pending && FlowSink::sum == 1U, "Flow step pumps source lane sink");

    FlowSource::next = 0U;
    FlowSource::limit = 2U;
    FlowSink::sum = 0U;
    FlowSink::writes = 0U;
    FlowSink::ready = false;

    arc::Flow<FlowSource, arc::CheckedSpsc<FlowPacket, 4>, FlowSink> checked{};
    expect(checked.fill() && checked.fill(), "Flow fills checked lane");
    expect(!checked.drain() && checked.blocked() && checked.pending() && FlowSink::writes == 0U,
           "Flow retains sink-blocked value");
    FlowSink::ready = true;
    expect(checked.drain() && FlowSink::sum == 1U && FlowSink::writes == 1U && !checked.pending(),
           "Flow writes retained value first");
    expect(checked.drain() && FlowSink::sum == 3U && FlowSink::writes == 2U,
           "Flow resumes lane after sink backpressure");
}

void test_spsc()
{
    arc::Spsc<int, 4> queue;
    using SpscInt = arc::Spsc<int, 4>;
    using RoleSpsc = arc::Roles<arc::Spsc<int, 4>>;
    static_assert(!std::is_copy_constructible_v<SpscInt::Producer>);
    static_assert(!std::is_copy_constructible_v<SpscInt::Consumer>);
    static_assert(!HasRootPush<RoleSpsc>);
    static_assert(!HasRootPop<RoleSpsc>);
    expect(queue.cap() == 3U, "SPSC usable capacity");
    expect(queue.bytes() == sizeof(queue), "SPSC bytes");
    expect(queue.empty(), "SPSC starts empty");
    expect(queue.try_push(1), "SPSC push 1");
    expect(queue.try_push(2), "SPSC push 2");
    expect(queue.try_push(3), "SPSC push 3");
    expect(!queue.try_push(4), "SPSC full rejects");

    int value{};
    expect(queue.peek(value) && value == 1 && queue.size() == 3U, "SPSC peek keeps front queued");
    expect(queue.drop() && queue.size() == 2U, "SPSC drop removes peeked front");
    expect(queue.try_push(4), "SPSC wrap push");
    expect(queue.try_pop(value) && value == 2, "SPSC pop 2 after drop");
    expect(queue.try_pop(value) && value == 3, "SPSC pop 3");
    expect(queue.try_pop(value) && value == 4, "SPSC pop 4");
    expect(!queue.try_pop(value) && !queue.peek(value) && !queue.drop(), "SPSC empty rejects");

    const std::array more{5, 6, 7, 8};
    expect(queue.push(std::span<const int>{static_cast<const int*>(nullptr), 1U}) == 0U,
           "SPSC batch push rejects null span");
    expect(queue.push(std::span(more)) == 3U, "SPSC batch push caps at space");
    expect(queue.size() == 3U, "SPSC batch size");
    expect(queue.space() == 0U, "SPSC batch full space");

    std::array<int, 4> out{};
    expect(queue.pop(std::span<int>{static_cast<int*>(nullptr), 1U}) == 0U,
           "SPSC batch pop rejects null span");
    expect(queue.pop(std::span(out)) == 3U, "SPSC batch pop available");
    expect(out[0] == 5 && out[1] == 6 && out[2] == 7, "SPSC batch order");
    expect(queue.empty(), "SPSC batch drains empty");

    expect(queue.push(std::span(more).first(3U)) == 3U, "SPSC drain stop seed");
    std::size_t drain_visits{};
    const auto stopped = queue.drain(value, [&](const int item) noexcept {
        ++drain_visits;
        return item != 6;
    });
    expect(stopped == 2U && drain_visits == 2U && queue.size() == 1U,
           "SPSC drain stops on false callback");
    expect(queue.try_pop(value) && value == 7, "SPSC drain leaves later entries queued");

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

    arc::Roles<arc::Spsc<int, 4>> role_only;
    auto role_endpoints = role_only.split();
    expect(role_endpoints.producer.try_push(14), "Roles SPSC producer endpoint push");
    expect(role_endpoints.consumer.try_pop(value) && value == 14, "Roles SPSC consumer endpoint pop");
}

void test_core_local()
{
    using Core1Counter = arc::CoreLocal<std::uint32_t, arc::Core::core1>;
    static_assert(Core1Counter::owner == arc::Core::core1);
    static_assert(arc::CoreOwner<arc::Core::core1, Core1Counter::owner>);
    static_assert(Core1Counter::can_access<arc::Core::core1>());
    static_assert(!Core1Counter::can_access<arc::Core::core0>());
    static_assert(HasCoreGet<Core1Counter, arc::Core::core1>);
    static_assert(!HasCoreGet<Core1Counter, arc::Core::core0>);
    static_assert(HasCoreMsg<Core1Counter, arc::Core::core1, arc::Core::core0>);
    static_assert(!HasCoreMsg<Core1Counter, arc::Core::core0, arc::Core::core0>);
    static_assert(HasCoreMsgTo<Core1Counter, arc::Core::core0>);

    Core1Counter counter{41U};
    expect(counter.get<arc::Core::core1>() == 41U, "CoreLocal owner reads local state");
    counter.set<arc::Core::core1>(42U);
    expect(counter.get<arc::Core::core1>() == 42U, "CoreLocal owner writes local state");
    const auto scoped = counter.with<arc::Core::core1>([](std::uint32_t& value) {
        value += 1U;
        return value;
    });
    expect(scoped == 43U && counter.get<arc::Core::core1>() == 43U, "CoreLocal scoped access updates local state");
    const auto inferred_scoped = counter.with([](std::uint32_t& value) {
        value += 1U;
        return value;
    });
    expect(inferred_scoped == 44U && counter.get<arc::Core::core1>() == 44U,
           "CoreLocal inferred scoped access updates local state");
    const auto scoped_const = static_cast<const Core1Counter&>(counter).with<arc::Core::core1>(
        [](const std::uint32_t& value) {
            return value;
        });
    expect(scoped_const == 44U, "CoreLocal const scoped access returns copied value");
    const auto inferred_const = static_cast<const Core1Counter&>(counter).with([](const std::uint32_t& value) {
        return value;
    });
    expect(inferred_const == 44U, "CoreLocal inferred const scoped access returns copied value");

    const auto msg = counter.msg<arc::Core::core0>();
    using Msg = decltype(msg);
    static_assert(Msg::from == arc::Core::core1);
    static_assert(Msg::to == arc::Core::core0);
    static_assert(HasMsgGet<Msg, arc::Core::core0>);
    static_assert(!HasMsgGet<Msg, arc::Core::core1>);
    static_assert(HasMsgGetInferred<Msg>);
    using Core0Counter = arc::CoreLocal<std::uint32_t, arc::Core::core0>;
    static_assert(HasCoreAccept<Core0Counter, Msg>);
    expect(msg.get<arc::Core::core0>() == 44U, "CoreMsg destination reads payload");
    expect(msg.get() == 44U, "CoreMsg inferred destination reads payload");

    Core0Counter mirror{};
    mirror.accept(msg);
    expect(mirror.get<arc::Core::core0>() == 44U, "CoreLocal accepts addressed message");

    const auto explicit_msg = counter.msg<arc::Core::core1, arc::Core::core0>();
    mirror.accept<arc::Core::core0>(explicit_msg);
    expect(mirror.get<arc::Core::core0>() == 44U, "CoreLocal explicit handoff stays available");
}

struct BorrowFixture {
    std::uint32_t value{};
};

constinit BorrowFixture borrow_fixture{7U};
constinit BorrowFixture other_borrow_fixture{13U};
constinit const BorrowFixture const_borrow_fixture{9U};

void test_static_borrow()
{
    using Cell = arc::StaticRef<&borrow_fixture, arc::Core::core1>;
    using Read = Cell::Read;
    using Write = Cell::Write;
    using OtherCell = arc::StaticRef<&other_borrow_fixture, arc::Core::core1>;
    using OtherWrite = OtherCell::Write;
    using ConstCell = arc::StaticRef<&const_borrow_fixture>;
    using AnyRead = ConstCell::Read;
    using ReadPack = arc::StaticReads<Cell, Cell>;
    using WritePack = arc::StaticWrites<Cell, OtherCell>;
    using EditPack = arc::StaticEdit<Cell, OtherCell>;

    static_assert(Cell::owner == arc::Core::core1);
    static_assert(Cell::object == &borrow_fixture);
    static_assert(std::same_as<typename Cell::Read, Read>);
    static_assert(std::same_as<typename Cell::Write, Write>);
    static_assert(arc::StaticRefType<Cell>);
    static_assert(Cell::writable);
    static_assert(Cell::can_read<arc::Core::core1>());
    static_assert(Cell::can_write<arc::Core::core1>());
    static_assert(!Cell::can_read<arc::Core::core0>());
    static_assert(!Cell::can_write<arc::Core::core0>());
    static_assert(!ConstCell::writable);
    static_assert(Read::mode == arc::BorrowMode::read);
    static_assert(Write::mode == arc::BorrowMode::mut);
    static_assert(arc::StaticLoanType<Read>);
    static_assert(arc::loans_ok<Read, Read>());
    static_assert(arc::loans_ok<Write, OtherWrite>());
    static_assert(!arc::loans_ok<Read, Write>());
    static_assert(!arc::loans_ok<Write, Write>());
    static_assert(ReadPack::count == 2U);
    static_assert(ReadPack::contains<Read>());
    static_assert(ReadPack::reads<&borrow_fixture>());
    static_assert(ReadPack::reads<Cell>());
    static_assert(!ReadPack::writes<&borrow_fixture>());
    static_assert(!ReadPack::writes<Cell>());
    static_assert(WritePack::writes<&borrow_fixture>());
    static_assert(WritePack::writes<Cell>());
    static_assert(EditPack::can_access<arc::Core::core1>());
    static_assert(!EditPack::can_access<arc::Core::core0>());
    static_assert(EditPack::contains<Write>());
    static_assert(EditPack::reads<OtherCell>());
    static_assert(EditPack::writes<Cell>());
    static_assert(arc::HasLoan<ReadPack, Read>);
    static_assert(arc::HasStaticRead<ReadPack, &borrow_fixture>);
    static_assert(arc::HasStaticRefRead<ReadPack, Cell>);
    static_assert(!arc::HasStaticWrite<ReadPack, &borrow_fixture>);
    static_assert(!arc::HasStaticRefWrite<ReadPack, Cell>);
    static_assert(arc::HasStaticWrite<WritePack, &other_borrow_fixture>);
    static_assert(arc::HasStaticRefWrite<WritePack, OtherCell>);
    static_assert(std::is_copy_constructible_v<Read>);
    static_assert(!std::is_copy_constructible_v<Write>);
    static_assert(std::is_move_constructible_v<Read>);
    static_assert(!std::is_move_constructible_v<Write>);
    static_assert(arc::StaticReadable<Cell, arc::Core::core1>);
    static_assert(!arc::StaticReadable<Cell, arc::Core::core0>);
    static_assert(arc::StaticWritable<Cell, arc::Core::core1>);
    static_assert(!arc::StaticWritable<Cell, arc::Core::core0>);
    static_assert(arc::LoanReadable<Read, arc::Core::core1>);
    static_assert(!arc::LoanReadable<Read, arc::Core::core0>);
    static_assert(arc::LoanWritable<Write, arc::Core::core1>);
    static_assert(!arc::LoanWritable<Write, arc::Core::core0>);
    static_assert(!arc::LoanWritable<Read, arc::Core::core1>);
    static_assert(!HasConstLoanArrow<Read>);
    static_assert(!HasLoanArrow<Write>);
    static_assert(HasConstLoanArrow<AnyRead>);
    static_assert(HasConstLoanDeref<AnyRead>);
    static_assert(arc::StaticReadable<ConstCell, arc::Core::core0>);
    static_assert(!arc::StaticWritable<ConstCell, arc::Core::core0>);

    borrow_fixture.value = 7U;
    {
        const auto read = Cell::read<arc::Core::core1>();
        const auto copied = read;
        expect(read.get<arc::Core::core1>().value == 7U && copied.get<arc::Core::core1>().value == 7U,
               "StaticRef shared loan reads static storage");
    }

    {
        auto write = Cell::write<arc::Core::core1>();
        write.get<arc::Core::core1>().value = 11U;
        write.get<arc::Core::core1>().value = 12U;
    }
    expect(borrow_fixture.value == 12U, "StaticRef mutable loan writes static storage");

    Cell::with_write([](BorrowFixture& state) {
        state.value = 14U;
    });
    const auto scoped_read = Cell::with_read([](const BorrowFixture& state) {
        return state.value;
    });
    expect(scoped_read == 14U, "StaticRef member scoped helpers keep loan use local");
    const auto explicit_member_read = Cell::with_read<arc::Core::core1>([](const BorrowFixture& state) {
        return state.value;
    });
    expect(explicit_member_read == 14U, "StaticRef explicit member read stays available");
    arc::with_write<Cell, arc::Core::core1>([](BorrowFixture& state) {
        state.value = 14U;
    });
    const auto free_read = arc::with_read<Cell, arc::Core::core1>([](const BorrowFixture& state) {
        return state.value;
    });
    expect(free_read == 14U, "StaticRef free scoped helpers stay available");
    const auto reads_sum = arc::with_reads<Cell, OtherCell>(
        [](const BorrowFixture& state, const BorrowFixture& other) {
            return state.value + other.value;
        });
    expect(reads_sum == 27U, "StaticReads scoped helper uses inferred owner");
    const auto explicit_reads_sum = arc::with_reads<arc::Core::core1, Cell, OtherCell>(
        [](const BorrowFixture& state, const BorrowFixture& other) {
            return state.value + other.value;
        });
    expect(explicit_reads_sum == 27U, "StaticReads explicit core helper stays available");

    other_borrow_fixture.value = 5U;
    const auto edited = arc::with_edit<Cell, OtherCell>([](BorrowFixture& state, const BorrowFixture& other) {
        state.value += other.value;
        return state.value;
    });
    expect(edited == 19U && borrow_fixture.value == 19U, "StaticEdit scoped helper mixes one writer with reads");

    const auto explicit_core = arc::with_edit<arc::Core::core1, Cell, OtherCell>(
        [](BorrowFixture& state, const BorrowFixture& other) {
            state.value += other.value;
            return state.value;
        });
    expect(explicit_core == 24U && borrow_fixture.value == 24U, "StaticEdit explicit core helper stays available");

    const auto read_any = ConstCell::read<arc::Core::core0>();
    expect((*read_any).value == 9U, "StaticRef default owner allows readonly global access");
}

struct LockstepOut {
    std::int16_t left{};
    std::int16_t right{};

    constexpr bool operator==(const LockstepOut&) const = default;
};

static_assert(arc::LockstepValue<LockstepOut>);
static_assert(std::is_same_v<arc::Lockstep<LockstepOut>::Fault, arc::LockstepFault<LockstepOut>>);

struct LockstepPolicy {
    static inline arc::LockstepFault<LockstepOut> last{};
    static inline std::size_t captures{};
    static inline std::size_t halts{};

    static void capture(const arc::LockstepFault<LockstepOut>& fault) noexcept
    {
        last = fault;
        ++captures;
    }

    [[nodiscard]] static esp_err_t halt(const arc::LockstepFault<LockstepOut>&) noexcept
    {
        ++halts;
        return ESP_OK;
    }
};

struct LockstepBareHaltPolicy {
    static inline std::size_t halts{};

    static void halt() noexcept
    {
        ++halts;
    }
};

void test_lockstep()
{
    using Check = arc::Lockstep<LockstepOut>;
    constexpr LockstepOut agreed{.left = 10, .right = -10};
    static_assert(Check::match(agreed, agreed));
    static_assert(!Check::match(agreed, {.left = 11, .right = -10}));

    LockstepPolicy::last = {};
    LockstepPolicy::captures = 0U;
    LockstepPolicy::halts = 0U;

    const auto accepted = Check::commit<LockstepPolicy>(agreed, agreed, 7U);
    expect(accepted.has_value() && (*accepted).left == 10 && LockstepPolicy::captures == 0U &&
               LockstepPolicy::halts == 0U,
           "Lockstep accepts matching output without policy hooks");

    const auto rejected = Check::commit<LockstepPolicy>(agreed, {.left = 11, .right = -10}, 8U);
    expect(!rejected && rejected.error() == ESP_ERR_INVALID_STATE, "Lockstep rejects mismatched output");
    expect(LockstepPolicy::captures == 1U && LockstepPolicy::halts == 1U && LockstepPolicy::last.tick == 8U &&
               LockstepPolicy::last.primary.left == 10 && LockstepPolicy::last.replica.left == 11,
           "Lockstep reports mismatch to capture and halt hooks");

    LockstepBareHaltPolicy::halts = 0U;
    const auto bare = Check::commit<LockstepBareHaltPolicy>(agreed, {.left = 10, .right = -11}, 9U);
    expect(!bare && LockstepBareHaltPolicy::halts == 1U, "Lockstep supports a bare halt policy");
}

struct SimTrace {
    static inline int drive_pin{};
    static inline bool drive_level{};
    static inline std::size_t drive_writes{};
    static inline int sense_pin{};
    static inline bool sense_level{};
    static inline std::size_t sense_reads{};
    static inline std::uint8_t spi_mosi{};
    static inline std::uint8_t spi_miso{};
    static inline std::size_t spi_xfers{};

    static void drive(const int pin, const bool level) noexcept
    {
        drive_pin = pin;
        drive_level = level;
        ++drive_writes;
    }

    static void sense(const int pin, const bool level) noexcept
    {
        sense_pin = pin;
        sense_level = level;
        ++sense_reads;
    }

    static void spi(const std::uint8_t mosi, const std::uint8_t miso) noexcept
    {
        spi_mosi = mosi;
        spi_miso = miso;
        ++spi_xfers;
    }
};

void test_sim()
{
    arc::sim::Fifo<int, 3> fifo{};
    static_assert(decltype(fifo)::cap() == 3U);
    int value{};
    expect(fifo.empty() && !fifo.try_pop(value), "Sim Fifo starts empty");
    const std::array input{1, 2, 3};
    expect(fifo.push(std::span<const int>{input}) == 3U && fifo.full(), "Sim Fifo batch push fills");
    expect(!fifo.try_push(4), "Sim Fifo full rejects");
    std::array<int, 2> partial{};
    expect(fifo.pop(std::span<int>{partial}) == 2U && partial[0] == 1 && partial[1] == 2,
           "Sim Fifo batch pop preserves order");
    expect(fifo.try_push(4) && fifo.size() == 2U, "Sim Fifo wraps after pop");
    std::array<int, 3> drained{};
    expect(fifo.pop(std::span<int>{drained}) == 2U && drained[0] == 3 && drained[1] == 4 && fifo.empty(),
           "Sim Fifo drains wrapped entries");

    using Led = arc::sim::Drive<4, SimTrace>;
    Led::configured = false;
    Led::level = false;
    SimTrace::drive_writes = 0U;
    Led::out();
    Led::hi();
    Led::toggle();
    Led::write<true>();
    expect(Led::configured && Led::high() && SimTrace::drive_pin == 4 && SimTrace::drive_level &&
               SimTrace::drive_writes == 3U,
           "Sim Drive traces static pin output");

    using Button = arc::sim::Sense<5, 4, SimTrace>;
    Button::configured = false;
    Button::clear();
    SimTrace::sense_reads = 0U;
    Button::in();
    const std::array samples{true, false, true};
    expect(Button::feed(std::span<const bool>{samples}) == samples.size(), "Sim Sense queues input samples");
    expect(Button::tick() && Button::high() && SimTrace::sense_pin == 5 && SimTrace::sense_level,
           "Sim Sense consumes first FIFO sample");
    expect(Button::tick() && Button::low(), "Sim Sense consumes low sample");
    expect(Button::tick() && Button::high() && !Button::tick() && SimTrace::sense_reads == 3U,
           "Sim Sense reports FIFO exhaustion");

    arc::sim::Spi<4, SimTrace> spi{};
    SimTrace::spi_xfers = 0U;
    const std::array<std::uint8_t, 2> spi_reply{0xa1U, 0xb2U};
    const std::array<std::uint8_t, 3> spi_request{0x10U, 0x20U, 0x30U};
    std::array<std::uint8_t, 3> spi_seen{};
    expect(spi.feed_rx(spi_reply).has_value() &&
               spi.xfer(spi_request, spi_seen).has_value() &&
               spi_seen[0] == 0xa1U && spi_seen[1] == 0xb2U && spi_seen[2] == 0xffU &&
               SimTrace::spi_mosi == 0x30U && SimTrace::spi_miso == 0xffU &&
               SimTrace::spi_xfers == 3U,
           "Sim Spi exchanges fixed host FIFO bytes");
    std::array<std::uint8_t, 4> spi_tx{};
    expect(spi.drain_tx(spi_tx) == 3U && spi_tx[0] == 0x10U && spi_tx[2] == 0x30U,
           "Sim Spi drains captured MOSI bytes");
    expect(!spi.xfer(spi_request, std::span<std::uint8_t>{spi_seen}.first(2U)), "Sim Spi rejects uneven transfer");

    using Log = arc::sim::TraceLog<8>;
    using HarnessLed = arc::sim::Drive<7, Log>;
    using HarnessButton = arc::sim::Sense<8, 4, Log>;
    using Rig = arc::sim::Harness<Log, HarnessButton>;
    Rig::reset();
    HarnessLed::level = false;
    HarnessLed::out();
    HarnessButton::in();
    const std::array harness_samples{true, false, true};
    expect(HarnessButton::feed(std::span<const bool>{harness_samples}) == harness_samples.size(),
           "Sim Harness queues input samples");
    HarnessLed::hi();
    expect(Rig::tick() == 1U, "Sim Harness advances queued inputs");
    HarnessLed::lo();
    Rig::run(2U);
    const auto events = Log::events();
    expect(events.size() == 5U &&
               events[0].tick == 0U && events[0].kind == arc::sim::EventKind::drive &&
               events[0].pin == 7 && events[0].level &&
               events[1].tick == 0U && events[1].kind == arc::sim::EventKind::sense &&
               events[1].pin == 8 && events[1].level &&
               events[2].tick == 1U && events[2].kind == arc::sim::EventKind::drive &&
               !events[2].level &&
               events[4].tick == 2U && events[4].kind == arc::sim::EventKind::sense &&
               events[4].level && !Log::overflow(),
           "Sim TraceLog records deterministic pin timeline");

    using TinyLog = arc::sim::TraceLog<1>;
    TinyLog::reset();
    TinyLog::mark(1);
    TinyLog::mark(2);
    expect(TinyLog::events().size() == 1U && TinyLog::overflow(), "Sim TraceLog reports overflow");
}

void test_checked_spsc()
{
    arc::Audit<arc::Spsc<int, 4>> queue;
    using CheckedSpsc = arc::Audit<arc::Spsc<int, 4>>;
    static_assert(!std::is_copy_constructible_v<CheckedSpsc::Producer>);
    static_assert(!std::is_copy_constructible_v<CheckedSpsc::Consumer>);
    expect(queue.cap() == 3U, "Audit SPSC usable capacity");
    expect(queue.try_push(1), "Audit SPSC push 1");

    int value{};
    expect(queue.try_pop(value) && value == 1, "Audit SPSC pop 1");

    auto endpoints = queue.split();
    expect(static_cast<bool>(endpoints.producer) && static_cast<bool>(endpoints.consumer),
           "Audit SPSC role endpoints split");
    expect(endpoints.producer.try_push(2), "Audit SPSC producer endpoint push");
    expect(endpoints.consumer.try_pop(value) && value == 2, "Audit SPSC consumer endpoint pop");
    auto moved_consumer = std::move(endpoints.consumer);
    expect(!static_cast<bool>(endpoints.consumer) && static_cast<bool>(moved_consumer),
           "Audit SPSC consumer endpoint is move-only");

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
    using RoleMpsc = arc::Roles<arc::Mpsc<int, 4>>;
    static_assert(std::is_copy_constructible_v<MpscInt::Producer>);
    static_assert(!std::is_copy_constructible_v<MpscInt::Consumer>);
    static_assert(!HasRootPush<RoleMpsc>);
    static_assert(!HasRootPop<RoleMpsc>);
    expect(queue.cap() == 4U, "MPSC capacity");
    expect(queue.cell_align() == arc::cache_line, "MPSC cache-line alignment");
    expect(queue.bytes() == sizeof(queue), "MPSC bytes");
    expect(queue.try_push(7), "MPSC push 7");
    expect(queue.try_push(8), "MPSC push 8");
    expect(queue.try_push(9), "MPSC push 9");
    expect(queue.try_push(10), "MPSC push 10");
    expect(!queue.try_push(11), "MPSC full rejects");

    int value{};
    expect(queue.peek(value) && value == 7, "MPSC peek 7");
    expect(queue.drop(), "MPSC drop 7");
    expect(queue.peek(value) && value == 8, "MPSC peek 8 after drop");
    expect(queue.try_pop(value) && value == 8, "MPSC pop 8");
    expect(queue.try_push(11), "MPSC wrap push");
    expect(queue.try_pop(value) && value == 9, "MPSC pop 9");
    expect(queue.try_pop(value) && value == 10, "MPSC pop 10");
    expect(queue.try_pop(value) && value == 11, "MPSC pop 11");
    expect(!queue.try_pop(value) && !queue.peek(value) && !queue.drop(), "MPSC empty rejects");

    const std::array more{16, 17, 18, 19, 20};
    expect(queue.push(std::span<const int>{static_cast<const int*>(nullptr), 1U}) == 0U,
           "MPSC batch push rejects null span");
    expect(queue.push(std::span(more)) == 4U, "MPSC batch push caps at capacity");
    expect(queue.push(std::span(more).first(1U)) == 0U, "MPSC batch full rejects");
    std::array<int, 5> out{};
    expect(queue.pop(std::span<int>{static_cast<int*>(nullptr), 1U}) == 0U,
           "MPSC batch pop rejects null span");
    expect(queue.pop(std::span(out).first(2U)) == 2U, "MPSC batch partial pop count");
    expect(out[0] == 16 && out[1] == 17, "MPSC batch partial pop order");
    out = {};
    expect(queue.pop(std::span(out)) == 2U, "MPSC batch remainder pop count");
    expect(out[0] == 18 && out[1] == 19, "MPSC batch remainder pop order");
    expect(!queue.try_pop(value), "MPSC batch drains empty");

    expect(queue.push(std::span(more).first(4U)) == 4U, "MPSC drain stop seed");
    std::size_t drain_visits{};
    const auto stopped = queue.drain(value, [&](const int item) noexcept {
        ++drain_visits;
        return item != 17;
    });
    out = {};
    expect(stopped == 2U && drain_visits == 2U && queue.pop(std::span(out).first(2U)) == 2U &&
               out[0] == 18 && out[1] == 19,
           "MPSC drain stops on false callback");

    auto endpoints = queue.split();
    auto second_producer = endpoints.producer;
    expect(static_cast<bool>(endpoints.producer) && static_cast<bool>(endpoints.consumer),
           "MPSC role endpoints split");
    expect(endpoints.producer.try_push(12) && second_producer.try_push(13), "MPSC producer endpoints push");
    expect(endpoints.consumer.peek(value) && value == 12, "MPSC consumer endpoint peek 12");
    expect(endpoints.consumer.drop(), "MPSC consumer endpoint drop 12");
    auto moved_consumer = std::move(endpoints.consumer);
    expect(!static_cast<bool>(endpoints.consumer) && static_cast<bool>(moved_consumer),
           "MPSC consumer endpoint is move-only");
    expect(moved_consumer.peek(value) && value == 13, "MPSC moved consumer endpoint peek 13");
    expect(moved_consumer.try_pop(value) && value == 13, "MPSC moved consumer endpoint pop 13");
    expect(second_producer.push(std::span(more).first(3U)) == 3U, "MPSC producer endpoint batch push");
    out = {};
    expect(moved_consumer.pop(std::span(out)) == 3U, "MPSC consumer endpoint batch pop");
    expect(out[0] == 16 && out[1] == 17 && out[2] == 18, "MPSC consumer endpoint batch order");

    arc::Roles<arc::Mpsc<int, 4>> role_only;
    auto role_endpoints = role_only.split();
    auto role_second_producer = role_only.producer();
    expect(role_endpoints.producer.try_push(14) && role_second_producer.try_push(15),
           "Roles MPSC producer endpoints push");
    expect(role_endpoints.consumer.peek(value) && value == 14, "Roles MPSC consumer endpoint peek");
    expect(role_endpoints.consumer.drop(), "Roles MPSC consumer endpoint drop");
    expect(role_endpoints.consumer.try_pop(value) && value == 15, "Roles MPSC consumer endpoint pop");
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
    using CheckedMpsc = arc::Audit<arc::Mpsc<int, 4>>;
    static_assert(std::is_copy_constructible_v<CheckedMpsc::Producer>);
    static_assert(!std::is_copy_constructible_v<CheckedMpsc::Consumer>);
    expect(queue.try_push(1), "Audit MPSC push 1");

    int value{};
    expect(queue.peek(value) && value == 1, "Audit MPSC peek 1");
    expect(queue.try_pop(value) && value == 1, "Audit MPSC pop 1");
    const std::array more{4, 5, 6, 7, 8};
    expect(queue.push(std::span(more)) == 4U, "Audit MPSC batch push caps at capacity");
    std::array<int, 5> out{};
    expect(queue.peek(value) && value == 4, "Audit MPSC batch peek");
    expect(queue.drop(), "Audit MPSC batch drop");
    expect(queue.pop(std::span(out)) == 3U, "Audit MPSC batch pop count");
    expect(out[0] == 5 && out[1] == 6 && out[2] == 7, "Audit MPSC batch pop order");
    expect(queue.push(std::span(more).first(4U)) == 4U, "Audit MPSC drain stop seed");
    std::size_t drain_visits{};
    const auto stopped = queue.drain(value, [&](const int item) noexcept {
        ++drain_visits;
        return item != 5;
    });
    out = {};
    expect(stopped == 2U && drain_visits == 2U && queue.pop(std::span(out).first(2U)) == 2U &&
               out[0] == 6 && out[1] == 7,
           "Audit MPSC drain stops on false callback");

    auto endpoints = queue.split();
    auto second_producer = endpoints.producer;
    expect(static_cast<bool>(endpoints.producer) && static_cast<bool>(endpoints.consumer),
           "Audit MPSC role endpoints split");
    expect(endpoints.producer.try_push(2) && second_producer.try_push(3), "Audit MPSC producer endpoints push");
    expect(endpoints.consumer.peek(value) && value == 2, "Audit MPSC consumer endpoint peek");
    expect(endpoints.consumer.drop(), "Audit MPSC consumer endpoint drop");
    auto moved_consumer = std::move(endpoints.consumer);
    expect(!static_cast<bool>(endpoints.consumer) && static_cast<bool>(moved_consumer),
           "Audit MPSC consumer endpoint is move-only");
    expect(moved_consumer.peek(value) && value == 3, "Audit MPSC moved consumer endpoint peek");
    expect(moved_consumer.try_pop(value) && value == 3, "Audit MPSC moved consumer endpoint pop");
    expect(second_producer.push(std::span(more).subspan(1U, 3U)) == 3U, "Audit MPSC producer endpoint batch push");
    out = {};
    expect(moved_consumer.pop(std::span(out).last(3U)) == 3U, "Audit MPSC consumer endpoint batch pop");
    expect(out[2] == 5 && out[3] == 6 && out[4] == 7, "Audit MPSC consumer endpoint batch order");

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
    using Fan = arc::Fanin<int, 4, 3>;
    using RoleFan = arc::Roles<arc::Fanin<int, 4, 3>>;
    static_assert(!std::is_copy_constructible_v<Fan::Producer<0>>);
    static_assert(!std::is_copy_constructible_v<Fan::Consumer>);
    static_assert(!HasRootFaninPush<RoleFan>);
    static_assert(!HasRootPop<RoleFan>);
    expect(fan.producers() == 3U, "Fanin producer count");
    expect(fan.cap() == 3U, "Fanin lane capacity");
    expect(fan.bytes() == sizeof(fan), "Fanin bytes");
    expect(fan.try_push<0>(10), "Fanin push lane 0");
    expect(fan.try_push<1>(20), "Fanin push lane 1");
    expect(fan.try_push<2>(30), "Fanin push lane 2");
    expect(fan.try_push<0>(11), "Fanin second lane 0");

    std::size_t producer{};
    int value{};
    expect(fan.peek(producer, value) && producer == 0U && value == 10, "Fanin peek lane 0");
    expect(fan.peek(value) && value == 10, "Fanin peek value only");
    expect(fan.drop(), "Fanin drop lane 0");
    expect(fan.try_pop(producer, value) && producer == 1U && value == 20, "Fanin pop lane 1");
    expect(fan.try_pop(producer, value) && producer == 2U && value == 30, "Fanin pop lane 2");
    expect(fan.try_pop(producer, value) && producer == 0U && value == 11, "Fanin round-robin resume");
    expect(!fan.try_pop(producer, value) && !fan.peek(producer, value) && !fan.drop(), "Fanin empty rejects");

    auto lane1 = fan.producer<1>();
    auto lane2 = fan.producer<2>();
    auto sink = fan.consumer();
    expect(static_cast<bool>(lane1) && static_cast<bool>(lane2) && static_cast<bool>(sink),
           "Fanin role endpoints bind");
    auto moved_lane1 = std::move(lane1);
    // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from endpoint state is inert.
    expect(!static_cast<bool>(lane1) && static_cast<bool>(moved_lane1), "Fanin producer endpoint is move-only");
    expect(moved_lane1.try_push(40) && lane2.try_push(50), "Fanin producer endpoints push");
    expect(sink.peek(producer, value) && producer == 1U && value == 40, "Fanin consumer endpoint peek lane 1");
    expect(sink.peek(value) && value == 40, "Fanin consumer endpoint peek value only");
    expect(sink.drop(), "Fanin consumer endpoint drop lane 1");
    expect(sink.try_pop(producer, value) && producer == 2U && value == 50, "Fanin consumer endpoint pop lane 2");

    expect(fan.try_push<0>(1), "Fanin batch lane 0 first");
    expect(fan.try_push<0>(2), "Fanin batch lane 0 second");
    expect(fan.try_push<1>(10), "Fanin batch lane 1 first");
    expect(fan.try_push<2>(20), "Fanin batch lane 2 first");
    expect(fan.try_push<2>(21), "Fanin batch lane 2 second");
    std::array<int, 6> out{};
    expect(fan.pop(std::span(out)) == 5U, "Fanin batch pop count");
    expect(out[0] == 1 && out[1] == 2 && out[2] == 10 && out[3] == 20 && out[4] == 21, "Fanin batch lane order");
    expect(fan.empty(), "Fanin batch drains empty");

    const std::array lane{100, 101, 102, 103};
    expect(fan.push<1>(std::span(lane)) == 3U, "Fanin batch push caps lane");
    expect(fan.size<1>() == 3U && fan.space<1>() == 0U, "Fanin lane size and space");
    out = {};
    expect(fan.pop(std::span(out)) == 3U, "Fanin batch pushed pop count");
    expect(out[0] == 100 && out[1] == 101 && out[2] == 102, "Fanin batch pushed order");

    expect(fan.push<0>(std::span(lane).first(3U)) == 3U, "Fanin drain stop seed");
    std::size_t drain_visits{};
    const auto stopped = fan.drain(value, [&](const std::size_t source, const int item) noexcept {
        expect(source == 0U, "Fanin drain stop source");
        ++drain_visits;
        return item != 101;
    });
    expect(stopped == 2U && drain_visits == 2U && fan.size<0>() == 1U,
           "Fanin drain stops on false callback");
    expect(fan.try_pop(producer, value) && producer == 0U && value == 102,
           "Fanin drain leaves later entries queued");

    arc::Roles<arc::Fanin<int, 4, 3>> role_only;
    auto role_lane = role_only.producer<2>();
    auto role_sink = role_only.consumer();
    expect(role_lane.try_push(200), "Roles Fanin producer endpoint push");
    expect(role_sink.peek(producer, value) && producer == 2U && value == 200,
           "Roles Fanin consumer endpoint peek");
    expect(role_sink.try_pop(producer, value) && producer == 2U && value == 200,
           "Roles Fanin consumer endpoint pop");
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
    using Rpc = decltype(rpc);
    using RoleRpc = arc::Roles<arc::RpcLane<Op, Request, Reply, 4>>;
    static_assert(!std::is_copy_constructible_v<Rpc::Client>);
    static_assert(!std::is_copy_constructible_v<Rpc::Server>);
    static_assert(!HasRootRpcCall<RoleRpc, Op, Request>);
    static_assert(!HasRootRpcRecv<RoleRpc, Rpc::Request>);
    static_assert(!HasRootRpcPoll<RoleRpc, Rpc::Reply>);
    static_assert(!HasRootRpcPollMatch<RoleRpc, Rpc::Reply>);

    auto client = rpc.client();
    auto server = rpc.server();
    expect(static_cast<bool>(client) && static_cast<bool>(server), "RPC role endpoints bind");
    expect(client.call(Op::set, Request{.value = 42U}, 7U), "RPC client queues request");

    decltype(rpc)::Request request{};
    expect(server.recv(request), "RPC server recv request");
    expect(request.serial == 7U && request.op == Op::set && request.payload.value == 42U, "RPC request fields");
    expect(server.reply(request.serial, ESP_OK, Reply{.applied = request.payload.value + 1U}), "RPC server queues reply");

    decltype(rpc)::Reply reply{};
    expect(client.poll_match(7U, reply), "RPC client match reply");
    expect(reply.status == ESP_OK && reply.payload.applied == 43U, "RPC reply fields");

    auto moved_client = std::move(client);
    // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from endpoint state is inert.
    expect(!static_cast<bool>(client) && static_cast<bool>(moved_client), "RPC client endpoint is move-only");
    expect(moved_client.call(Op::set, Request{.value = 1U}, 8U), "RPC second client call");
    expect(server.recv(request), "RPC second server recv");
    expect(server.reply(9U, ESP_OK, Reply{.applied = 9U}), "RPC out-of-order reply");
    expect(!moved_client.poll_match(8U, reply), "RPC client defers unmatched reply");
    expect(moved_client.poll_deferred(reply) && reply.serial == 9U, "RPC client deferred reply preserved");

    arc::RpcLane<Op, Request, Reply, 4, 2> narrow_rpc;
    auto narrow_client = narrow_rpc.client();
    auto narrow_server = narrow_rpc.server();
    expect(narrow_server.reply(9U, ESP_OK, Reply{.applied = 9U}), "RPC narrow out-of-order reply");
    expect(!narrow_client.poll_match(8U, reply), "RPC narrow defers unmatched reply");
    expect(narrow_server.reply(8U, ESP_OK, Reply{.applied = 8U}), "RPC narrow exact reply queues");
    expect(narrow_client.poll_match(8U, reply) && reply.serial == 8U && reply.payload.applied == 8U,
           "RPC poll_match accepts exact reply with full deferred queue");
    expect(narrow_client.poll_deferred(reply) && reply.serial == 9U,
           "RPC deferred queue keeps earlier unmatched reply");

    RoleRpc role_only;
    auto role_client = role_only.client();
    auto role_server = role_only.server();
    expect(role_client.call(Op::set, Request{.value = 55U}, 10U), "Roles RPC client endpoint call");
    expect(role_server.recv(request), "Roles RPC server endpoint recv");
    expect(request.serial == 10U && request.payload.value == 55U, "Roles RPC request fields");
    expect(role_server.reply(request.serial, ESP_OK, Reply{.applied = 56U}), "Roles RPC server endpoint reply");
    expect(role_client.poll_match(10U, reply) && reply.payload.applied == 56U, "Roles RPC client endpoint reply");
}

void test_checked_fanin()
{
    arc::Audit<arc::Fanin<int, 4, 2>> fan;
    using CheckedFan = arc::Audit<arc::Fanin<int, 4, 2>>;
    static_assert(!std::is_copy_constructible_v<CheckedFan::Producer<0>>);
    static_assert(!std::is_copy_constructible_v<CheckedFan::Consumer>);
    expect(fan.producers() == 2U, "Audit Fanin producer count");
    expect(fan.try_push<0>(10), "Audit Fanin push lane 0");
    expect(fan.try_push<1>(20), "Audit Fanin push lane 1");

    std::size_t producer{};
    int value{};
    expect(fan.peek(producer, value) && producer == 0U && value == 10, "Audit Fanin peek lane 0");
    expect(fan.drop(), "Audit Fanin drop lane 0");
    expect(fan.try_pop(producer, value) && producer == 1U && value == 20, "Audit Fanin pop lane 1");

    expect(fan.try_push<0>(30), "Audit Fanin batch lane 0");
    expect(fan.try_push<1>(40), "Audit Fanin batch lane 1");
    std::array<int, 2> out{};
    expect(fan.pop(std::span(out)) == 2U && out[0] == 30 && out[1] == 40, "Audit Fanin batch pop");

    const std::array lane{50, 51, 52};
    expect(fan.push<0>(std::span(lane)) == 3U, "Audit Fanin batch push");
    expect(fan.size<0>() == 3U && fan.space<0>() == 0U, "Audit Fanin lane size and space");
    expect(fan.pop(std::span(out).first(2U)) == 2U && out[0] == 50 && out[1] == 51, "Audit Fanin partial batch pop");
    expect(fan.pop(std::span(out).first(1U)) == 1U && out[0] == 52, "Audit Fanin partial batch remainder");
    expect(fan.push<0>(std::span(lane)) == 3U, "Audit Fanin drain stop seed");
    std::size_t drain_visits{};
    const auto stopped = fan.drain(value, [&](const int item) noexcept {
        ++drain_visits;
        return item != 51;
    });
    expect(stopped == 2U && drain_visits == 2U && fan.size<0>() == 1U,
           "Audit Fanin drain stops on false callback");
    expect(fan.try_pop(producer, value) && producer == 0U && value == 52,
           "Audit Fanin drain leaves later entries queued");

    arc::Audit<arc::Fanin<int, 4, 2>> checked_roles;
    auto role_producer = checked_roles.producer<1>();
    auto role_consumer = checked_roles.consumer();
    expect(static_cast<bool>(role_producer) && static_cast<bool>(role_consumer),
           "Audit Fanin role endpoints bind");
    auto moved_producer = std::move(role_producer);
    // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from endpoint state is inert.
    expect(!static_cast<bool>(role_producer) && static_cast<bool>(moved_producer),
           "Audit Fanin producer endpoint is move-only");
    expect(moved_producer.try_push(60), "Audit Fanin producer endpoint push");
    expect(role_consumer.peek(producer, value) && producer == 1U && value == 60,
           "Audit Fanin consumer endpoint peek");
    expect(role_consumer.try_pop(producer, value) && producer == 1U && value == 60,
           "Audit Fanin consumer endpoint pop");
}

void test_mqtt()
{
    struct MqttMock {
        std::size_t publishes{};

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> connect(
            const std::span<std::uint8_t> out,
            const arc::net::MqttConnect& cfg) noexcept
        {
            return arc::net::Mqtt::connect(out, cfg);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> publish(
            const std::span<std::uint8_t> out,
            const arc::net::MqttPublish& cfg) noexcept
        {
            ++publishes;
            return arc::net::Mqtt::publish(out, cfg);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> subscribe(
            const std::span<std::uint8_t> out,
            const std::uint16_t packet,
            const std::span<const arc::net::MqttSubscription> topics) noexcept
        {
            return arc::net::Mqtt::subscribe(out, packet, topics);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> ping(
            const std::span<std::uint8_t> out) noexcept
        {
            return arc::net::Mqtt::ping(out);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> disconnect(
            const std::span<std::uint8_t> out) noexcept
        {
            return arc::net::Mqtt::disconnect(out);
        }

        [[nodiscard]] arc::Result<arc::net::MqttPacket> parse(
            const std::span<const std::uint8_t> frame) noexcept
        {
            return arc::net::Mqtt::parse(frame);
        }

        [[nodiscard]] arc::Result<arc::net::MqttPublishView> view(
            const arc::net::MqttPacket& packet) noexcept
        {
            return arc::net::Mqtt::view(packet);
        }

        [[nodiscard]] arc::Result<arc::net::MqttConnAck> connack(
            const arc::net::MqttPacket& packet) noexcept
        {
            return arc::net::Mqtt::connack(packet);
        }

        [[nodiscard]] arc::Result<arc::net::MqttSubAck> suback(
            const arc::net::MqttPacket& packet) noexcept
        {
            return arc::net::Mqtt::suback(packet);
        }

        [[nodiscard]] bool match(const char* const filter, const char* const topic) noexcept
        {
            return arc::net::Mqtt::match(filter, topic);
        }
    };

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
    std::array<std::uint8_t, 128> in_place_connect_buf{
        'a',
        'r',
        'c',
        '-',
        'c',
        'l',
        'i',
        'e',
        'n',
        't',
        '\0',
        'u',
        's',
        'e',
        'r',
        '\0',
        'p',
        'a',
        's',
        's',
        '\0',
    };
    const auto in_place_connect = arc::net::Mqtt::connect(
        std::span(in_place_connect_buf),
        {
            .client = reinterpret_cast<const char*>(in_place_connect_buf.data()),
            .user = reinterpret_cast<const char*>(in_place_connect_buf.data() + 11U),
            .pass = reinterpret_cast<const char*>(in_place_connect_buf.data() + 16U),
            .keep_alive = 60U,
            .clean = true,
        });
    expect(in_place_connect.has_value() &&
               in_place_connect->size() == 36U &&
               (*in_place_connect)[14] == 'a' &&
               (*in_place_connect)[23] == 't' &&
               (*in_place_connect)[26] == 'u' &&
               (*in_place_connect)[29] == 'r' &&
               (*in_place_connect)[32] == 'p' &&
               (*in_place_connect)[35] == 's',
           "MQTT connect preserves in-place strings");

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

    const std::uint8_t one = 0x5aU;
    expect(!arc::net::Mqtt::publish(
               buffer,
               arc::net::MqttPublish{
                   .topic = "arc/topic",
                   .data = &one,
                   .bytes = std::numeric_limits<std::size_t>::max(),
               }),
           "MQTT publish length overflow rejects");
    expect(!arc::net::Mqtt::publish(buffer, "", std::span(payload)),
           "MQTT publish rejects empty topic");

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
    std::array<std::uint8_t, 96> in_place_publish{0xdeU, 0xadU, 0xbeU, 0xefU, 0x5aU};
    const std::array<std::uint8_t, 5> in_place_payload{0xdeU, 0xadU, 0xbeU, 0xefU, 0x5aU};
    const auto in_place_mqtt = arc::net::Mqtt::publish(
        std::span(in_place_publish),
        "arc/in-place",
        std::span(in_place_publish).first(in_place_payload.size()));
    expect(in_place_mqtt.has_value(), "MQTT publish in-place encodes");
    const auto in_place_packet = arc::net::Mqtt::parse(*in_place_mqtt);
    expect(in_place_packet.has_value(), "MQTT publish in-place parses");
    const auto in_place_view = arc::net::Mqtt::view(*in_place_packet);
    expect(in_place_view.has_value() &&
               in_place_view->payload.size() == in_place_payload.size() &&
               std::memcmp(in_place_view->payload.data(), in_place_payload.data(), in_place_payload.size()) == 0,
           "MQTT publish preserves in-place payload");
    std::array<std::uint8_t, 96> in_place_topic_buf{};
    constexpr char in_place_topic_text[] = "arc/in-place/topic";
    std::memcpy(in_place_topic_buf.data(), in_place_topic_text, sizeof(in_place_topic_text));
    const std::array<std::uint8_t, 2> in_place_topic_payload{0x10U, 0x20U};
    const auto in_place_topic_mqtt = arc::net::Mqtt::publish(
        std::span(in_place_topic_buf),
        reinterpret_cast<const char*>(in_place_topic_buf.data()),
        std::span(in_place_topic_payload));
    expect(in_place_topic_mqtt.has_value(), "MQTT publish in-place topic encodes");
    const auto in_place_topic_packet = arc::net::Mqtt::parse(*in_place_topic_mqtt);
    expect(in_place_topic_packet.has_value(), "MQTT publish in-place topic parses");
    const auto in_place_topic_view = arc::net::Mqtt::view(*in_place_topic_packet);
    expect(in_place_topic_view.has_value() &&
               in_place_topic_view->topic.size() == sizeof(in_place_topic_text) - 1U &&
               std::memcmp(
                   in_place_topic_view->topic.data(),
                   in_place_topic_text,
                   sizeof(in_place_topic_text) - 1U) == 0 &&
               in_place_topic_view->payload.size() == in_place_topic_payload.size() &&
               std::memcmp(
                   in_place_topic_view->payload.data(),
                   in_place_topic_payload.data(),
                   in_place_topic_payload.size()) == 0,
           "MQTT publish preserves in-place topic");

    const std::array subscriptions{
        arc::net::MqttSubscription{.filter = "arc/+/status", .qos = arc::net::MqttQos::at_least},
        arc::net::MqttSubscription{.filter = "arc/#", .qos = arc::net::MqttQos::at_most},
    };
    const auto subscribe = arc::net::Mqtt::subscribe(buffer, 9U, std::span(subscriptions));
    expect(subscribe.has_value(), "MQTT subscribe encodes");
    expect((*subscribe)[0] == 0x82U, "MQTT subscribe flags");
    expect((*subscribe)[2] == 0x00U && (*subscribe)[3] == 0x09U, "MQTT subscribe packet id");
    std::array<std::uint8_t, 64> in_place_subscribe_buf{'a', '/', 'b', '\0', 'c', '/', 'd', '\0'};
    const std::array in_place_subscriptions{
        arc::net::MqttSubscription{
            .filter = reinterpret_cast<const char*>(in_place_subscribe_buf.data()),
            .qos = arc::net::MqttQos::at_least},
        arc::net::MqttSubscription{
            .filter = reinterpret_cast<const char*>(in_place_subscribe_buf.data() + 4U),
            .qos = arc::net::MqttQos::at_most},
    };
    const auto in_place_subscribe =
        arc::net::Mqtt::subscribe(std::span(in_place_subscribe_buf), 11U, std::span(in_place_subscriptions));
    expect(in_place_subscribe.has_value() &&
               in_place_subscribe->size() == 16U &&
               (*in_place_subscribe)[6] == 'a' &&
               (*in_place_subscribe)[7] == '/' &&
               (*in_place_subscribe)[8] == 'b' &&
               (*in_place_subscribe)[9] == 0x01U &&
               (*in_place_subscribe)[12] == 'c' &&
               (*in_place_subscribe)[13] == '/' &&
               (*in_place_subscribe)[14] == 'd' &&
               (*in_place_subscribe)[15] == 0x00U,
           "MQTT subscribe preserves in-place filters");
    const std::array empty_filter{arc::net::MqttSubscription{.filter = "", .qos = arc::net::MqttQos::at_most}};
    expect(!arc::net::Mqtt::subscribe(buffer, 10U, std::span(empty_filter)),
           "MQTT subscribe rejects empty filter");

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
    expect(!arc::net::Mqtt::ping(std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 2U}),
           "MQTT rejects null output span");
    const std::array<std::uint8_t, 2> pingresp_raw{0xD0U, 0x00U};
    const auto pingresp = arc::net::Mqtt::parse(pingresp_raw);
    expect(pingresp.has_value(), "MQTT pingresp parses");
    expect(session.observe(*pingresp, 3050U) == ESP_OK, "MQTT session pingresp");
    expect(!session.awaiting_ping(), "MQTT session pingresp clears");
    expect(!session.expired(6000U), "MQTT session not expired");
    expect(session.expired(6060U), "MQTT session expired");

    auto codec = arc::net::AnyMqtt::arc();
    expect(static_cast<bool>(codec), "AnyMqtt arc codec binds");
    const auto any_ping = codec.ping(buffer);
    expect(any_ping.has_value() && any_ping->size() == 2U, "AnyMqtt arc ping");
    MqttMock mock{};
    static_assert(arc::net::MqttAdapter<MqttMock>);
    auto erased = arc::net::AnyMqtt::bind(mock);
    const auto any_publish = erased.publish(
        buffer,
        arc::net::MqttPublish{
            .topic = "arc/any",
            .data = payload.data(),
            .bytes = payload.size(),
        });
    expect(any_publish.has_value() && mock.publishes == 1U, "AnyMqtt bound publish forwards");
    const auto any_packet = erased.parse(*any_publish);
    expect(any_packet.has_value() && erased.view(*any_packet).has_value(), "AnyMqtt bound parse and view");
    arc::net::AnyMqtt empty{};
    expect(!empty.ping(buffer) && !static_cast<bool>(empty), "AnyMqtt empty rejects");

    const arc::net::MqttPacket huge_publish{
        .type = arc::net::MqttType::publish,
        .body = std::span<const std::uint8_t>{
            static_cast<const std::uint8_t*>(nullptr),
            std::numeric_limits<std::size_t>::max(),
        },
    };
    expect(!arc::net::Mqtt::view(huge_publish), "MQTT publish view rejects null body");
    expect(!arc::net::Mqtt::parse(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 2U}),
           "MQTT parse rejects null frame span");
    expect(!arc::net::Mqtt::connack(
               {.type = arc::net::MqttType::connack,
                .body = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 2U}}),
           "MQTT connack rejects null body span");
    expect(!arc::net::Mqtt::suback(
               {.type = arc::net::MqttType::suback,
                .body = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 3U}}),
           "MQTT suback rejects null body span");
    const std::array<std::uint8_t, 4> empty_topic_publish{0x30U, 0x02U, 0x00U, 0x00U};
    const auto empty_topic_packet = arc::net::Mqtt::parse(empty_topic_publish);
    expect(empty_topic_packet.has_value() && !arc::net::Mqtt::view(*empty_topic_packet),
           "MQTT publish view rejects empty topic");

    expect(arc::net::Mqtt::match("arc/+/status", "arc/node/status"), "MQTT single wildcard");
    expect(arc::net::Mqtt::match("arc/#", "arc/node/status"), "MQTT multi wildcard");
    expect(arc::net::Mqtt::match("arc/#", "arc"), "MQTT parent wildcard root");
    expect(!arc::net::Mqtt::match("", "arc") && !arc::net::Mqtt::match("arc/#", ""),
           "MQTT match rejects empty filter or topic");
    expect(!arc::net::Mqtt::match("arc#", "arc"), "MQTT invalid hash placement");
    expect(!arc::net::Mqtt::match("arc+status", "arc/status"), "MQTT invalid plus placement");
    expect(!arc::net::Mqtt::match("arc/+/status", "arc/node/state"), "MQTT mismatch");
    expect(!arc::net::Mqtt::match("arc/#", "$SYS/broker"), "MQTT system topic isolation");
}

void test_ws()
{
    struct WsMock {
        std::size_t frames{};

        [[nodiscard]] arc::Result<std::span<const char>> key(
            const std::span<char> out,
            const std::span<const std::uint8_t> nonce) noexcept
        {
            return arc::net::Ws::key(out, nonce);
        }

        [[nodiscard]] arc::Result<std::span<const char>> accept(
            const std::span<char> out,
            const char* const key) noexcept
        {
            return arc::net::Ws::accept(out, key);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> frame(
            const std::span<std::uint8_t> out,
            const arc::net::WsOpcode opcode,
            const void* const data,
            const std::size_t bytes,
            const bool fin,
            const std::uint32_t mask,
            const bool rsv1,
            const bool rsv2,
            const bool rsv3) noexcept
        {
            ++frames;
            return arc::net::Ws::frame(out, opcode, data, bytes, fin, mask, rsv1, rsv2, rsv3);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> text(
            const std::span<std::uint8_t> out,
            const char* const value,
            const bool fin,
            const std::uint32_t mask) noexcept
        {
            return arc::net::Ws::text(out, value, fin, mask);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> close(
            const std::span<std::uint8_t> out,
            const std::uint16_t code,
            const std::span<const std::uint8_t> reason,
            const std::uint32_t mask) noexcept
        {
            return arc::net::Ws::close(out, code, reason, mask);
        }

        [[nodiscard]] arc::Result<arc::net::WsFrame> parse(
            const std::span<const std::uint8_t> frame,
            const std::span<std::uint8_t> scratch) noexcept
        {
            return arc::net::Ws::parse(frame, scratch);
        }

        [[nodiscard]] arc::Result<arc::net::WsClose> close_view(
            const arc::net::WsFrame& frame) noexcept
        {
            return arc::net::Ws::close_view(frame);
        }
    };

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
    std::array<std::uint8_t, 128> in_place_ws_buf{'a', 'r', 'c', '!'};
    const std::array<std::uint8_t, 4> in_place_ws_payload{'a', 'r', 'c', '!'};
    const auto in_place_ws_frame = arc::net::Ws::binary(
        std::span(in_place_ws_buf),
        std::span(in_place_ws_buf).first(in_place_ws_payload.size()),
        true,
        0x01020304U);
    expect(in_place_ws_frame.has_value(), "WS in-place binary encodes");
    std::array<std::uint8_t, 8> in_place_ws_scratch{};
    const auto in_place_ws = arc::net::Ws::parse(*in_place_ws_frame, std::span(in_place_ws_scratch));
    expect(in_place_ws.has_value() &&
               in_place_ws->payload.size() == in_place_ws_payload.size() &&
               std::memcmp(in_place_ws->payload.data(), in_place_ws_payload.data(), in_place_ws_payload.size()) == 0,
           "WS preserves in-place masked payload");

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

    auto codec = arc::net::AnyWs::arc();
    expect(static_cast<bool>(codec), "AnyWs arc codec binds");
    const auto any_text = codec.text(frame, "abi", true, 0U);
    expect(any_text.has_value() && codec.parse(*any_text).has_value(), "AnyWs arc text parses");
    WsMock mock{};
    static_assert(arc::net::WsAdapter<WsMock>);
    auto erased = arc::net::AnyWs::bind(mock);
    const auto any_binary = erased.frame(
        frame,
        arc::net::WsOpcode::binary,
        ping_data.data(),
        ping_data.size());
    expect(any_binary.has_value() && mock.frames == 1U, "AnyWs bound frame forwards");
    const auto any_frame = erased.parse(*any_binary);
    expect(any_frame.has_value() && any_frame->opcode == arc::net::WsOpcode::binary, "AnyWs bound parse");
    arc::net::AnyWs empty{};
    expect(!empty.text(frame, "x") && !static_cast<bool>(empty), "AnyWs empty rejects");
}

void test_coap()
{
    struct CoapMock {
        std::size_t messages{};

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> message(
            const std::span<std::uint8_t> out,
            const arc::net::CoapType type,
            const std::uint8_t code,
            const std::uint16_t id,
            const std::span<const std::uint8_t> token,
            const std::span<const arc::net::CoapOption> options,
            const std::span<const std::uint8_t> payload) noexcept
        {
            ++messages;
            return arc::net::Coap::message(out, type, code, id, token, options, payload);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> ping(
            const std::span<std::uint8_t> out,
            const std::uint16_t id,
            const std::span<const std::uint8_t> token) noexcept
        {
            return arc::net::Coap::ping(out, id, token);
        }

        [[nodiscard]] arc::Result<std::span<const std::uint8_t>> reset(
            const std::span<std::uint8_t> out,
            const std::uint16_t id,
            const std::span<const std::uint8_t> token) noexcept
        {
            return arc::net::Coap::reset(out, id, token);
        }

        [[nodiscard]] arc::Result<arc::net::CoapMessage> parse(
            const std::span<const std::uint8_t> frame) noexcept
        {
            return arc::net::Coap::parse(frame);
        }

        [[nodiscard]] arc::Result<arc::net::CoapOptionView> next(
            const std::span<const std::uint8_t> options,
            std::size_t& offset,
            std::uint16_t& number) noexcept
        {
            return arc::net::Coap::next(options, offset, number);
        }
    };

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
    std::array<std::uint8_t, 32> in_place_token_buf{0xdeU, 0xadU};
    const auto in_place_token_request = arc::net::Coap::message(
        std::span(in_place_token_buf),
        arc::net::CoapType::confirmable,
        static_cast<std::uint8_t>(arc::net::CoapCode::get),
        0x5678U,
        std::span(in_place_token_buf).first(token.size()));
    expect(in_place_token_request.has_value(), "CoAP in-place token encodes");
    const auto in_place_token_parsed = arc::net::Coap::parse(*in_place_token_request);
    expect(in_place_token_parsed.has_value() &&
               in_place_token_parsed->token.size() == token.size() &&
               std::memcmp(in_place_token_parsed->token.data(), token.data(), token.size()) == 0,
           "CoAP preserves in-place token");

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
    std::array<std::uint8_t, 64> in_place_options_buf{'s', 'e', 'n', 's', 'o', 'r', 's', 't', 'e', 'm', 'p'};
    const std::array in_place_options{
        arc::net::Coap::option(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
            std::span(in_place_options_buf).first(7U)),
        arc::net::Coap::option(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
            std::span(in_place_options_buf).subspan(7U, 4U)),
    };
    const auto in_place_options_request = arc::net::Coap::message(
        std::span(in_place_options_buf),
        arc::net::CoapType::confirmable,
        static_cast<std::uint8_t>(arc::net::CoapCode::get),
        0x9876U,
        {},
        std::span(in_place_options));
    expect(in_place_options_request.has_value(), "CoAP in-place options encode");
    const auto in_place_options_parsed = arc::net::Coap::parse(*in_place_options_request);
    expect(in_place_options_parsed.has_value(), "CoAP in-place options parse");
    auto in_place_options_offset = std::size_t{};
    auto in_place_options_number = std::uint16_t{};
    const auto in_place_options_first =
        arc::net::Coap::next(in_place_options_parsed->options, in_place_options_offset, in_place_options_number);
    expect(in_place_options_first.has_value() &&
               std::memcmp(in_place_options_first->value.data(), "sensors", in_place_options_first->value.size()) == 0,
           "CoAP preserves first in-place option");
    const auto in_place_options_second =
        arc::net::Coap::next(in_place_options_parsed->options, in_place_options_offset, in_place_options_number);
    expect(in_place_options_second.has_value() &&
               std::memcmp(in_place_options_second->value.data(), "temp", in_place_options_second->value.size()) == 0,
           "CoAP preserves second in-place option");

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
    std::array<std::uint8_t, 96> in_place_coap{'t', 'i', 'g', 'h', 't'};
    const std::array<std::uint8_t, 5> in_place_coap_payload{'t', 'i', 'g', 'h', 't'};
    const auto in_place_coap_msg = arc::net::Coap::message(
        std::span(in_place_coap),
        arc::net::CoapType::nonconfirmable,
        static_cast<std::uint8_t>(arc::net::CoapCode::post),
        0x4567U,
        {},
        {},
        std::span(in_place_coap).first(in_place_coap_payload.size()));
    expect(in_place_coap_msg.has_value(), "CoAP in-place payload encodes");
    const auto in_place_coap_parsed = arc::net::Coap::parse(*in_place_coap_msg);
    expect(in_place_coap_parsed.has_value() &&
               in_place_coap_parsed->payload.size() == in_place_coap_payload.size() &&
               std::memcmp(
                   in_place_coap_parsed->payload.data(),
                   in_place_coap_payload.data(),
                   in_place_coap_payload.size()) == 0,
           "CoAP preserves in-place payload");

    const auto ping = arc::net::Coap::ping(buffer, 0x40U);
    expect(ping.has_value(), "CoAP ping encodes");
    const auto ping_msg = arc::net::Coap::parse(*ping);
    expect(ping_msg.has_value(), "CoAP ping parses");
    expect(ping_msg->type == arc::net::CoapType::confirmable, "CoAP ping type");
    expect(ping_msg->code == static_cast<std::uint8_t>(arc::net::CoapCode::empty), "CoAP ping code");
    expect(!arc::net::Coap::ping(buffer, 0x40U, std::span(token)),
           "CoAP empty ping rejects token");

    const auto reset = arc::net::Coap::reset(buffer, 0x40U);
    expect(reset.has_value(), "CoAP reset encodes");
    expect(!arc::net::Coap::reset(buffer, 0x40U, std::span(token)),
           "CoAP empty reset rejects token");
    expect(!arc::net::Coap::message(
               buffer,
               arc::net::CoapType::confirmable,
               static_cast<std::uint8_t>(arc::net::CoapCode::empty),
               0x40U,
               {},
               std::span(options)),
           "CoAP empty message rejects options");
    expect(!arc::net::Coap::message(
               buffer,
               arc::net::CoapType::confirmable,
               static_cast<std::uint8_t>(arc::net::CoapCode::empty),
               0x40U,
               {},
               {},
               std::span(body)),
           "CoAP empty message rejects payload");
    const std::array<std::uint8_t, 5> empty_with_token{0x41U, 0x00U, 0x12U, 0x34U, 0x00U};
    expect(!arc::net::Coap::parse(empty_with_token), "CoAP empty parse rejects token");
    const std::array<std::uint8_t, 5> empty_with_option{0x40U, 0x00U, 0x12U, 0x34U, 0x00U};
    expect(!arc::net::Coap::parse(empty_with_option), "CoAP empty parse rejects options");
    const std::array<std::uint8_t, 6> empty_with_payload{0x40U, 0x00U, 0x12U, 0x34U, 0xffU, 0x00U};
    expect(!arc::net::Coap::parse(empty_with_payload), "CoAP empty parse rejects payload");
    expect(!arc::net::Coap::message(
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), buffer.size()},
               arc::net::CoapType::confirmable,
               1U,
               1U),
           "CoAP rejects null output span");
    expect(!arc::net::Coap::parse(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 4U}),
           "CoAP parse rejects null frame span");
    std::size_t null_option_offset{};
    std::uint16_t null_option_number{};
    expect(!arc::net::Coap::next(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
               null_option_offset,
               null_option_number),
           "CoAP next rejects null options span");

    auto codec = arc::net::AnyCoap::arc();
    expect(static_cast<bool>(codec), "AnyCoap arc codec binds");
    const auto any_ping = codec.ping(buffer, 0x41U);
    expect(any_ping.has_value() && codec.parse(*any_ping).has_value(), "AnyCoap arc ping parses");
    CoapMock mock{};
    static_assert(arc::net::CoapAdapter<CoapMock>);
    auto erased = arc::net::AnyCoap::bind(mock);
    const auto any_request = erased.message(
        buffer,
        arc::net::CoapType::confirmable,
        static_cast<std::uint8_t>(arc::net::CoapCode::get),
        0x4321U,
        std::span(token),
        std::span(options));
    expect(any_request.has_value() && mock.messages == 1U, "AnyCoap bound message forwards");
    const auto any_msg = erased.parse(*any_request);
    expect(any_msg.has_value() && any_msg->id == 0x4321U, "AnyCoap bound parse");
    arc::net::AnyCoap empty{};
    expect(!empty.ping(buffer, 0x42U) && !static_cast<bool>(empty), "AnyCoap empty rejects");

    const std::array<std::uint8_t, 4> overflow_option{0xe0U, 0xffU, 0xf2U, 0x00U};
    std::size_t overflow_offset{};
    std::uint16_t overflow_number = 65530U;
    const auto overflow = arc::net::Coap::next(overflow_option, overflow_offset, overflow_number);
    expect(!overflow.has_value(), "CoAP option number overflow rejects");

    expect(!arc::net::Coap::message(
               buffer,
               arc::net::CoapType::confirmable,
               static_cast<std::uint8_t>(arc::net::CoapCode::get),
               0x88U,
               {},
               {},
               std::span<const std::uint8_t>{body.data(), std::numeric_limits<std::size_t>::max()}),
           "CoAP payload length overflow rejects");
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
    std::array<char, 16> in_place_encode{'a', '/', 'b', ' ', '?'};
    const auto in_place_wire = arc::net::Uri::encode(
        std::span(in_place_encode),
        std::span<const char>{in_place_encode.data(), 5U},
        true);
    expect(in_place_wire.has_value() &&
               std::memcmp(in_place_wire->data(), "a%2Fb+%3F", in_place_wire->size()) == 0,
           "URI encode preserves in-place expansion");
    std::array<char, 16> shifted_encode{'x', 'a', '/', 'b'};
    expect(!arc::net::Uri::encode(
               std::span<char>{shifted_encode.data(), shifted_encode.size()},
               std::span<const char>{shifted_encode.data() + 1U, 3U}),
           "URI encode rejects shifted overlap");
    std::array<char, 16> shifted_decode{'a', 'b', '%', '2', '0', 'z'};
    expect(!arc::net::Uri::decode(
               std::span<char>{shifted_decode.data() + 1U, shifted_decode.size() - 1U},
               std::span<const char>{shifted_decode.data(), 6U},
               true),
           "URI decode rejects shifted overlap");

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
    expect(!arc::net::Uri::parse(std::span<const char>{static_cast<const char*>(nullptr), 1U}),
           "URI null input span rejects");
    std::size_t null_query_offset{};
    expect(!arc::net::Uri::next(std::span<const char>{static_cast<const char*>(nullptr), 1U}, null_query_offset),
           "URI query null span rejects");
    expect(!arc::net::Uri::decode(
               std::span(decoded),
               std::span<const char>{static_cast<const char*>(nullptr), 1U}),
           "URI decode null input span rejects");
    expect(!arc::net::Uri::decode(
               std::span<char>{static_cast<char*>(nullptr), 1U},
               std::span<const char>{full, 1U}),
           "URI decode null output span rejects");
    expect(!arc::net::Uri::encode(
               std::span(encoded),
               std::span<const char>{static_cast<const char*>(nullptr), 1U}),
           "URI encode null input span rejects");
    expect(!arc::net::Uri::encode(
               std::span<char>{static_cast<char*>(nullptr), 1U},
               std::span<const char>{full, 1U}),
           "URI encode null output span rejects");
    const arc::net::UriView manual_null_path{
        .path = std::span<const char>{static_cast<const char*>(nullptr), 1U},
    };
    expect(!arc::net::Uri::path_query(std::span(target), manual_null_path),
           "URI path query null path span rejects");
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
    expect(client->body(nullptr, 1U) == ESP_ERR_INVALID_ARG, "HTTP body rejects null non-empty");
    expect(client->body(nullptr, 0U) == ESP_OK, "HTTP body accepts empty null");
    expect(client->body(std::span(body)) == ESP_OK, "HTTP body forwards");
    expect(client->perform() == ESP_OK, "HTTP perform forwards");
    expect(client->open() == ESP_OK, "HTTP open forwards");
    expect(client->fetch().has_value(), "HTTP fetch forwards");
    std::array<std::uint8_t, 4> scratch{};
    expect(!client->write(nullptr, 1U).has_value(), "HTTP write rejects null non-empty");
    const auto empty_write = client->write(nullptr, 0U);
    expect(empty_write.has_value() && *empty_write == 0U, "HTTP write accepts empty null");
    expect(client->write(std::span(body)).has_value(), "HTTP write forwards");
    expect(!client->read(nullptr, 1U).has_value(), "HTTP read rejects null non-empty");
    const auto empty_read = client->read(nullptr, 0U);
    expect(empty_read.has_value() && *empty_read == 0U, "HTTP read accepts empty null");
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
    std::array<char, 32> ws_text{};
    const std::uint8_t ws_byte = 0x5aU;
    const std::array<std::uint8_t, 16> ws_nonce{};
    expect(!arc::net::Ws::frame(
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), ws_buffer.size()},
               arc::net::WsOpcode::binary,
               &ws_byte,
               1U),
           "WS frame null output span rejects");
    expect(!arc::net::Ws::frame(ws_buffer, static_cast<arc::net::WsOpcode>(0x3U), nullptr, 0U),
           "WS reserved opcode encode rejects");
    expect(!arc::net::Ws::frame(
               ws_buffer,
               arc::net::WsOpcode::binary,
               &ws_byte,
               std::numeric_limits<std::size_t>::max()),
           "WS frame length overflow rejects");
    expect(!arc::net::Ws::close(
               ws_buffer,
               1000U,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}),
           "WS close reason null span rejects");
    expect(!arc::net::Ws::close(ws_buffer, 999U), "WS close low status rejects");
    expect(!arc::net::Ws::close(ws_buffer, 1005U), "WS close reserved status rejects");
    const std::array<std::uint8_t, 3> ws_close_one_byte{0x88U, 0x01U, 0x03U};
    expect(!arc::net::Ws::parse(ws_close_one_byte), "WS close one-byte payload rejects");
    const std::array<std::uint8_t, 4> ws_close_reserved_code{0x88U, 0x02U, 0x03U, 0xedU};
    expect(!arc::net::Ws::parse(ws_close_reserved_code), "WS close reserved code parse rejects");
    const arc::net::WsFrame ws_reserved_close{
        .opcode = arc::net::WsOpcode::close,
        .payload = std::span(ws_close_reserved_code).subspan(2U),
    };
    expect(!arc::net::Ws::close_view(ws_reserved_close), "WS close view reserved code rejects");
    const std::array<std::uint8_t, 5> ws_short_extended{0x81U, 0x7eU, 0x00U, 0x01U, 'x'};
    expect(!arc::net::Ws::parse(ws_short_extended), "WS non-minimal 16-bit length rejects");
    const std::array<std::uint8_t, 11> ws_long_extended{
        0x82U,
        0x7fU,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x01U,
        0x5aU,
    };
    expect(!arc::net::Ws::parse(ws_long_extended), "WS non-minimal 64-bit length rejects");
    expect(!arc::net::Ws::key(
               ws_text,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 16U}),
           "WS key nonce null span rejects");
    expect(!arc::net::Ws::key(
               std::span<char>{static_cast<char*>(nullptr), ws_text.size()},
               std::span<const std::uint8_t>{ws_nonce}),
           "WS key output null span rejects");
    expect(!arc::net::Ws::parse(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), reserved_ws_opcode.size()}),
           "WS parse null frame span rejects");
    const std::array<std::uint8_t, 6> masked_empty_ws{0x81U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U};
    expect(!arc::net::Ws::parse(
               masked_empty_ws,
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U}),
           "WS parse null scratch span rejects");

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

    constexpr char duplicate_length_raw[] =
        "POST /config HTTP/1.1\r\n"
        "Content-Length: 7\r\n"
        "content-length: 7\r\n"
        "\r\n"
        "enabled";
    std::array<arc::net::HttpHeaderView, 4> duplicate_headers{};
    const auto duplicate_length_req = arc::net::HttpServer::parse(
        std::span<const char>{duplicate_length_raw, sizeof(duplicate_length_raw) - 1U},
        std::span(duplicate_headers));
    expect(duplicate_length_req.has_value() && duplicate_length_req->body.size() == 7U,
           "HTTP duplicate matching content length parses");

    constexpr char conflicting_length_raw[] =
        "POST /config HTTP/1.1\r\n"
        "Content-Length: 7\r\n"
        "content-length: 8\r\n"
        "\r\n"
        "enabled";
    std::array<arc::net::HttpHeaderView, 4> conflict_headers{};
    expect(!arc::net::HttpServer::parse(
               std::span<const char>{conflicting_length_raw, sizeof(conflicting_length_raw) - 1U},
               std::span(conflict_headers)),
           "HTTP conflicting content length rejects");

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
    std::array<char, 128> in_place_response{};
    std::memcpy(in_place_response.data(), body, sizeof(body) - 1U);
    const auto in_place = arc::net::HttpServer::text_response(
        std::span(in_place_response),
        200,
        "OK",
        std::span<const char>{in_place_response.data(), sizeof(body) - 1U});
    expect(in_place.has_value() && in_place->size() >= sizeof(body) - 1U &&
               std::memcmp(in_place->data() + in_place->size() - (sizeof(body) - 1U), body, sizeof(body) - 1U) == 0,
           "HTTP response preserves in-place body span");

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
    const auto null_json_body = arc::net::HttpServer::json_body(
        std::span<char>{static_cast<char*>(nullptr), json_body.size()},
        std::span(json_fields).first(1U));
    expect(!null_json_body.has_value() && null_json_body.error() == ESP_ERR_INVALID_ARG,
           "HTTP JSON body null output span rejects");

    std::array<arc::net::HttpHeaderView, 1> too_few{};
    expect(
        !arc::net::HttpServer::parse(std::span<const char>{raw, sizeof(raw) - 1U}, std::span(too_few)),
        "HTTP header capacity rejects");
    expect(
        !arc::net::HttpServer::parse(
            std::span<const char>{static_cast<const char*>(nullptr), 1U},
            std::span(headers)),
        "HTTP parse null request span rejects");
    expect(
        !arc::net::HttpServer::parse(
            std::span<const char>{raw, sizeof(raw) - 1U},
            std::span<arc::net::HttpHeaderView>{static_cast<arc::net::HttpHeaderView*>(nullptr), 1U}),
        "HTTP parse null header span rejects");

    constexpr char short_body[] = "POST /config HTTP/1.1\r\nContent-Length: 8\r\n\r\nenabled";
    expect(
        !arc::net::HttpServer::parse(std::span<const char>{short_body, sizeof(short_body) - 1U}, std::span(headers)),
        "HTTP short body rejects");

    constexpr char bad_header[] = "GET /config HTTP/1.1\r\nBroken\r\n\r\n";
    expect(
        !arc::net::HttpServer::parse(std::span<const char>{bad_header, sizeof(bad_header) - 1U}, std::span(headers)),
        "HTTP malformed header rejects");
    constexpr char bad_header_space[] = "GET /config HTTP/1.1\r\nHost : arc.local\r\n\r\n";
    expect(
        !arc::net::HttpServer::parse(
            std::span<const char>{bad_header_space, sizeof(bad_header_space) - 1U},
            std::span(headers)),
        "HTTP header whitespace before colon rejects");
    constexpr char bad_header_name[] = "GET /config HTTP/1.1\r\nBad@Name: value\r\n\r\n";
    expect(
        !arc::net::HttpServer::parse(
            std::span<const char>{bad_header_name, sizeof(bad_header_name) - 1U},
            std::span(headers)),
        "HTTP invalid header name rejects");
    constexpr char bad_header_value[] = "GET /config HTTP/1.1\r\nX-Mode: bad\001value\r\n\r\n";
    expect(
        !arc::net::HttpServer::parse(
            std::span<const char>{bad_header_value, sizeof(bad_header_value) - 1U},
            std::span(headers)),
        "HTTP invalid header value rejects");

    std::array<arc::net::HttpHeaderView, 1> null_length_header{{
        {
            .name = std::span<const char>{"content-length", 14U},
            .value = std::span<const char>{static_cast<const char*>(nullptr), 1U},
        },
    }};
    const arc::net::HttpRequestView manual_bad_length{
        .headers = std::span(null_length_header),
    };
    expect(!arc::net::HttpServer::content_length(manual_bad_length),
           "HTTP content length null span rejects");

    std::array<char, 8> small_response{};
    expect(
        !arc::net::HttpServer::text_response(
            std::span(small_response),
            200,
            "OK",
            std::span<const char>{body, sizeof(body) - 1U}),
        "HTTP response buffer cap rejects");
    expect(
        !arc::net::HttpServer::text_response(
            std::span(response),
            200,
            "OK",
            std::span<const char>{static_cast<const char*>(nullptr), 1U}),
        "HTTP response null body span rejects");
    expect(
        !arc::net::HttpServer::text_response(
            std::span(response),
            200,
            "OK\r\nX-Injected: 1",
            std::span<const char>{body, sizeof(body) - 1U}),
        "HTTP response reason injection rejects");
    expect(
        !arc::net::HttpServer::text_response(
            std::span(response),
            200,
            "OK",
            std::span<const char>{body, sizeof(body) - 1U},
            "text/plain\r\nX-Injected: 1"),
        "HTTP response content type injection rejects");
    expect(
        !arc::net::HttpServer::text_response(
            std::span(response),
            200,
            "OK",
            std::span<const char>{body, sizeof(body) - 1U},
            ""),
        "HTTP response empty content type rejects");
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
    arc::CapsBuf<std::uint16_t> wrapped_view{
        nullptr,
        (std::numeric_limits<std::size_t>::max() / sizeof(std::uint16_t)) + 1U,
        0U,
    };
    expect(!wrapped_view.bytes_fit() && wrapped_view.bytes() == std::numeric_limits<std::size_t>::max(),
           "CapsBuf byte view saturates manual size overflow");
    heap_caps_last_calloc_bytes = 777U;
    auto overflow = arc::inbuf<std::uint16_t>(
        (std::numeric_limits<std::size_t>::max() / sizeof(std::uint16_t)) + 1U);
    expect(!overflow && heap_caps_last_calloc_bytes == 777U, "cap buffer rejects byte overflow before allocation");

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
    auto cache_lines = arc::Cache::lines(dma.storage());
    expect(cache_lines.has_value() && cache_lines->bytes == padded, "Cache line token accepts padded storage");
    expect(arc::Cache::discard(*cache_lines) == ESP_OK && esp_cache_last_msync_bytes == padded &&
               (esp_cache_last_msync_flags & arc::Cache::unaligned) == 0,
           "Cache line token keeps strict discard aligned");
    expect(!arc::Cache::lines(dma.data() + 1U, arc::cache_line), "Cache line token rejects unaligned data");
    expect(!arc::Cache::lines(dma.data(), dma.bytes()), "Cache line token rejects partial cache line");
    static_assert(ARC_ENABLE_UNSAFE_CACHE_RAW == 0);

    using CpuDma = arc::DmaBuf<std::uint8_t>;
    using DevDma = arc::DmaBuf<std::uint8_t, arc::CacheState::device>;
    static_assert(CpuDma::state == arc::CacheState::cpu);
    static_assert(DevDma::state == arc::CacheState::device);
    static_assert(HasDmaView<CpuDma>);
    static_assert(HasDmaIndex<CpuDma>);
    static_assert(!HasDmaView<DevDma>);
    static_assert(!HasDmaIndex<DevDma>);

    CpuDma typed{std::move(dma)};
    typed[0] = 7U;
    auto device = std::move(typed).to_device();
    expect(device.has_value() && device->addr() != nullptr && device->bytes() == 65U,
           "DmaBuf flush moves CPU buffer to device state");
    auto back = std::move(*device).from_device();
    expect(back.has_value() && (*back)[0] == 7U, "DmaBuf invalidate restores CPU access");
    auto raw = std::move(*back).take();
    expect(raw.size() == 65U && raw[0] == 7U, "DmaBuf returns raw CapsBuf only in CPU state");

    CpuDma rx{arc::dmabuf<std::uint8_t>(65U)};
    auto rx_device = std::move(rx).discard();
    expect(rx_device.has_value(), "DmaBuf discard prepares device-write ownership");
    auto rx_cpu = std::move(*rx_device).from_device();
    expect(rx_cpu.has_value(), "DmaBuf device-write ownership returns to CPU");

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
    const std::array<int, 1> min_wave{std::numeric_limits<int>::min()};
    expect(arc::dsp::peak(min_wave.data(), min_wave.size()) == std::numeric_limits<int>::max(),
           "DSP peak saturates signed minimum magnitude");

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
    expect(arc::dsp::DspAccel<>::dot_f32(
               std::span<const float>{static_cast<const float*>(nullptr), 1U},
               std::span<const float>{accel_rhs}) == 0.0F,
           "DSP accel dot rejects null span");
    std::array mac_acc{1.0F, 2.0F};
    arc::dsp::DspAccel<>::mac_f32(std::span<float>{mac_acc}, std::span<const float>{accel_rhs}, 2.0F);
    expect(mac_acc[0] == 7.0F && mac_acc[1] == 10.0F, "DSP accel MAC fallback");
    const auto mac_before = mac_acc;
    arc::dsp::DspAccel<>::mac_f32(std::span<float>{mac_acc}, std::span<const float>{accel_rhs}, std::numeric_limits<float>::infinity());
    expect(mac_acc == mac_before, "DSP accel MAC rejects non-finite gain");

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
    expect(!Beam::delay_sum(
               beam_inputs,
               {.delay_samples = {0U, 1U}, .gain = {1.0F, 1.0F}, .scale = 0.5F},
               std::span<float>{static_cast<float*>(nullptr), 1U}),
           "DSP Beamform rejects null output span");
    const Beam::Inputs null_beam_inputs{std::span<const float>{static_cast<const float*>(nullptr), 1U}, std::span<const float>{mic1}};
    expect(!Beam::delay_sum(
               null_beam_inputs,
               {.delay_samples = {0U, 1U}, .gain = {1.0F, 1.0F}, .scale = 0.5F},
               std::span(beam_out)),
           "DSP Beamform rejects null input span");
    expect(!Beam::delay_sum(
               beam_inputs,
               {.delay_samples = {0U, 1U}, .gain = {std::numeric_limits<float>::infinity(), 1.0F}, .scale = 0.5F},
               std::span(beam_out)),
           "DSP Beamform rejects non-finite gain");

    const auto lag = Beam::lag_xcorr(std::span<const float>{mic0}, std::span<const float>{mic1}, 2U);
    expect(lag.has_value() && lag->lag_samples == 1, "DSP Beamform xcorr lag");
    expect(!Beam::lag_xcorr(
               std::span<const float>{static_cast<const float*>(nullptr), 1U},
               std::span<const float>{mic1},
               2U),
           "DSP Beamform rejects null xcorr span");
    expect(!Beam::lag_xcorr(
               std::span<const float>{mic0},
               std::span<const float>{mic1},
               static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())),
           "DSP Beamform rejects oversized lag windows");

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
    expect(!Beam::gcc_phat(
               std::span(ref_fft),
               std::span(delayed_fft),
               std::span<arc::simd::ComplexF32>{static_cast<arc::simd::ComplexF32*>(nullptr), gcc.size()},
               1U,
               48'000.0F,
               0.05F),
           "DSP Beamform rejects null GCC scratch span");
    expect(!Beam::gcc_phat(
               std::span(ref_fft),
               std::span(delayed_fft),
               std::span(gcc),
               1U,
               48'000.0F,
               std::numeric_limits<float>::quiet_NaN()),
           "DSP Beamform rejects non-finite GCC geometry");

    using Echo = arc::dsp::Aec<float, 2>;
    Echo::State echo{};
    const std::array<float, 4> far{1.0F, 0.0F, 0.0F, 0.0F};
    const std::array<float, 4> mic{0.5F, 0.0F, 0.0F, 0.0F};
    std::array<float, 4> clean{};
    const auto cancelled = Echo::run(std::span(clean), echo, std::span<const float>{far}, std::span<const float>{mic});
    expect(cancelled.has_value() && *cancelled == clean.size(), "DSP AEC run");
    expect(echo.taps[0] > 0.0F, "DSP AEC adapts tap");
    expect(!Echo::run(
               std::span<float>{static_cast<float*>(nullptr), 1U},
               echo,
               std::span<const float>{far},
               std::span<const float>{mic}),
           "DSP AEC rejects null output span");
    expect(!Echo::run(
               std::span(clean),
               echo,
               std::span<const float>{static_cast<const float*>(nullptr), 1U},
               std::span<const float>{mic}),
           "DSP AEC rejects null input span");
    expect(!Echo::run(
               std::span(clean),
               echo,
               std::span<const float>{far},
               std::span<const float>{mic},
               {.mu = 0.25F, .epsilon = std::numeric_limits<float>::quiet_NaN(), .leak = 1.0F}),
           "DSP AEC rejects invalid config");
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
    const auto null_motion_plan = arc::MotionPlan<3>::line(
        std::span<arc::MotionStep<3>>{static_cast<arc::MotionStep<3>*>(nullptr), 4U},
        {.delta = {4, 2, 1}, .ticks_step = 10U});
    expect(!null_motion_plan.has_value() && null_motion_plan.error() == ESP_ERR_INVALID_ARG,
           "Motion line rejects null output span");
    std::array<arc::MotionStep<1>, 1> tiny_steps{};
    const auto min_delta_plan = arc::MotionPlan<1>::line(
        std::span(tiny_steps),
        {.delta = {std::numeric_limits<std::int32_t>::min()}, .ticks_step = 1U});
    expect(!min_delta_plan.has_value() && min_delta_plan.error() == ESP_ERR_NO_MEM,
           "Motion line handles INT32_MIN distance without signed overflow");

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
    expect(arc::ml::dot<float, 4>(
               std::span<const float, 4>{static_cast<const float*>(nullptr), 4U},
               std::span<const float, 4>{input.values}) == 0.0F,
           "ML dot rejects null span");
    expect(!dense.eval(
               std::span<const float, 4>{static_cast<const float*>(nullptr), 4U},
               output.span()),
           "ML dense rejects null input span");
    const arc::ml::Dense<4, 2> null_dense{
        .weights = std::span<const float, 8>{static_cast<const float*>(nullptr), 8U},
        .bias = std::span<const float, 2>{bias},
    };
    expect(!null_dense.eval(std::span<const float, 4>{input.values}, output.span()), "ML dense rejects null weights span");

    output[1] = -3.0F;
    arc::ml::relu(output.span());
    expect(output[1] == 0.0F && arc::ml::argmax(std::span<const float, 2>{output.values}) == 0U, "ML relu argmax");
    arc::ml::relu(std::span<float, 2>{static_cast<float*>(nullptr), 2U});
    expect(arc::ml::argmax(std::span<const float, 2>{static_cast<const float*>(nullptr), 2U}) == 0U,
           "ML argmax rejects null span");

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
    expect(!qdense.eval(
               std::span<const std::int8_t, 4>{static_cast<const std::int8_t*>(nullptr), 4U},
               std::span<std::int8_t, 1>{qy}),
           "ML quant dense rejects null input span");
    expect(arc::ml::quantize_s8(3, 1, 1, 0) == 2 && arc::ml::quantize_s8(-3, 1, 1, 0) == -2,
           "ML quantize rounds symmetrically");
    expect(arc::ml::quantize_s8(std::numeric_limits<std::int64_t>::max(), 2, 0, 0) == 127 &&
               arc::ml::quantize_s8(std::numeric_limits<std::int64_t>::min(), 2, 0, 0) == -128 &&
               arc::ml::quantize_s8(std::numeric_limits<std::int64_t>::max(), 1, 0, 1) == 127 &&
               arc::ml::quantize_s8(std::numeric_limits<std::int64_t>::min(), 1, 0, -1) == -128,
           "ML quantize saturates overflow paths");
}

void test_simd_ml_pipeline()
{
    const std::array<std::int8_t, 16> lhs{1, 2, 3, 4, 5, 6, 7, 8, -1, -2, -3, -4, -5, -6, -7, -8};
    const std::array<std::int8_t, 16> rhs{1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1};
    expect(arc::simd::dot_s8(std::span(lhs), std::span(rhs)) == 72, "SIMD int8 dot");
    expect(arc::simd::dot_s8(
               std::span<const std::int8_t>{static_cast<const std::int8_t*>(nullptr), 16U},
               std::span<const std::int8_t>{rhs},
               0,
               0,
               7) == 7,
           "SIMD int8 dot rejects null span");
    expect(arc::simd::pie::mac_s8(0, arc::simd::load_s8x16(lhs.data()), arc::simd::load_s8x16(rhs.data())) == 72,
           "PIE int8 MAC wrapper");
    expect(std::string_view{arc::simd::pie::MacS8x16::instruction} == "ee.mac.s8", "PIE int8 wrapper name");

    const std::array<std::uint8_t, 4> white_yuv{235, 128, 235, 128};
    std::array<std::uint16_t, 2> rgb{};
    const auto pixels = arc::simd::Rgb565::from_yuv422(std::span(white_yuv), std::span(rgb));
    expect(pixels.has_value() && *pixels == 2U && rgb[0] == 0xffffU && rgb[1] == 0xffffU, "YUV422 RGB565 conversion");
    expect(!arc::simd::Rgb565::from_yuv422(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), white_yuv.size()},
               std::span<std::uint16_t>{rgb}),
           "YUV422 conversion rejects null input span");

    std::array<arc::simd::ComplexF32, 4> spectrum{
        arc::simd::ComplexF32{.re = 1.0F},
        arc::simd::ComplexF32{},
        arc::simd::ComplexF32{},
        arc::simd::ComplexF32{},
    };
    expect(arc::simd::fft_radix2(std::span(spectrum)).has_value(), "SIMD FFT runs");
    expect(near(spectrum[0].re, 1.0F) && near(spectrum[1].re, 1.0F) && near(spectrum[2].re, 1.0F), "SIMD FFT impulse");
    expect(!arc::simd::fft_radix2(
               std::span<arc::simd::ComplexF32>{static_cast<arc::simd::ComplexF32*>(nullptr), spectrum.size()}),
           "SIMD FFT rejects null span");

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
    const arc::MmuSpan<std::int8_t> null_mapped_conv{
        .region = {},
        .values = std::span<const std::int8_t>{static_cast<const std::int8_t*>(nullptr), conv_w.size()},
    };
    expect(!arc::ml::mapped_weights<std::int8_t, 4>(null_mapped_conv), "ML MmuSpan rejects null mapped weights");
    expect(conv.eval(std::span<const std::int8_t, 9>{conv_in}, std::span(conv_out)).has_value(), "Conv2D S8 eval");
    expect(conv_out[0] == 12 && conv_out[1] == 16 && conv_out[2] == 24 && conv_out[3] == 28, "Conv2D S8 output");
    expect(!conv.eval(
               std::span<const std::int8_t, 9>{static_cast<const std::int8_t*>(nullptr), conv_in.size()},
               std::span(conv_out)),
           "Conv2D S8 rejects null input span");

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
    expect(!depth.eval(
               std::span<const std::int8_t, 8>{depth_in},
               std::span<std::int8_t, arc::ml::DepthwiseConv2dS8<2, 2, 2, 1, 1>::output_size>{
                   static_cast<std::int8_t*>(nullptr),
                   depth_out.size()}),
           "Depthwise Conv2D S8 rejects null output span");

    const std::array<std::int8_t, 4> pool_in{1, 4, 2, 3};
    std::array<std::int8_t, arc::ml::MaxPool2d<std::int8_t, 2, 2, 1, 2, 2>::output_size> pool_out{};
    expect(arc::ml::MaxPool2d<std::int8_t, 2, 2, 1, 2, 2>::eval(std::span(pool_in), std::span(pool_out)).has_value(),
           "MaxPool eval");
    expect(pool_out[0] == 4, "MaxPool output");
    expect(!arc::ml::MaxPool2d<std::int8_t, 2, 2, 1, 2, 2>::eval(
               std::span<const std::int8_t, 4>{static_cast<const std::int8_t*>(nullptr), pool_in.size()},
               std::span(pool_out)),
           "MaxPool rejects null input span");

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
    expect(!arc::net::Csi::frame<8>(meta, std::span<const std::int8_t>{static_cast<const std::int8_t*>(nullptr), 2U}),
           "CSI rejects null raw IQ span");
    const auto stats = arc::net::Csi::stats(*csi_frame);
    expect(stats.bins == 4U && near(stats.amplitude_mean, 5.0F), "CSI RF stats");

    arc::Spsc<arc::net::CsiFrame<8>, 4> csi_queue{};
    expect(arc::net::Csi::push(csi_queue, meta, std::span<const std::int8_t>{raw_iq}).has_value(), "CSI SPSC push");
    arc::net::CsiFrame<8> popped{};
    expect(csi_queue.try_pop(popped) && popped.meta.channel == 6U, "CSI SPSC pop");

    std::array<float, 3> features{};
    expect(arc::net::Csi::features(*csi_frame, std::span<float, 3>{features}).has_value(), "CSI feature vector");
    expect(!arc::net::Csi::features(*csi_frame, std::span<float, 3>{static_cast<float*>(nullptr), 3U}),
           "CSI rejects null feature output span");
    std::array<std::int8_t, 3> quantized{};
    expect(arc::net::Csi::quantize(std::span<const float, 3>{features}, std::span<std::int8_t, 3>{quantized}, 1.0F).has_value(),
           "CSI quantize RF tensor");
    const std::array<float, 3> huge_features{1.0e30F, -1.0e30F, 0.49F};
    expect(arc::net::Csi::quantize(std::span<const float, 3>{huge_features}, std::span<std::int8_t, 3>{quantized}, 1.0F).has_value() &&
               quantized[0] == std::numeric_limits<std::int8_t>::max() &&
               quantized[1] == std::numeric_limits<std::int8_t>::min() &&
               quantized[2] == 0,
           "CSI quantize saturates huge finite features");
    const std::array<float, 3> nan_features{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F};
    expect(!arc::net::Csi::quantize(std::span<const float, 3>{nan_features}, std::span<std::int8_t, 3>{quantized}, 1.0F),
           "CSI quantize rejects non-finite features");
    expect(!arc::net::Csi::quantize(
               std::span<const float, 3>{features},
               std::span<std::int8_t, 3>{quantized},
               std::numeric_limits<float>::infinity()),
           "CSI quantize rejects non-finite scale");
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
    expect(
        !arc::HotPatch::load<PatchHeap>(
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), payload.size()},
            0U),
        "HotPatch rejects null payload span");

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
    expect(!arc::WorldGuard<TeePolicy>::apply(
               {.regions = std::span<const arc::WorldRegion>{static_cast<const arc::WorldRegion*>(nullptr), 1U}}),
           "TEE rejects null region span");
    expect(!arc::WorldGuard<TeePolicy>::apply(
               {.regions = std::span<const arc::WorldRegion>{worlds},
                .trusted_peripherals = std::span<const std::uint32_t>{static_cast<const std::uint32_t*>(nullptr), 1U}}),
           "TEE rejects null trusted peripheral span");
    expect(!arc::WorldGuard<TeePolicy>::apply(
               {.regions = std::span<const arc::WorldRegion>{worlds},
                .untrusted_peripherals = std::span<const std::uint32_t>{static_cast<const std::uint32_t*>(nullptr), 1U}}),
           "TEE rejects null untrusted peripheral span");

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
    expect(update.write({.ciphertext = cipher, .nonce = cipher, .aad = {}, .tag = cipher}, plain) ==
               ESP_ERR_INVALID_STATE,
           "SecureUpdate rejects write before begin");
    expect(update.finish(cipher) == ESP_ERR_INVALID_STATE, "SecureUpdate rejects finish before begin");
    expect(update.begin(4U) == ESP_OK, "SecureUpdate begin");
    expect(update.begin(4U) == ESP_ERR_INVALID_STATE, "SecureUpdate rejects duplicate begin");
    expect(update.write({.ciphertext = cipher, .nonce = cipher, .aad = {}, .tag = cipher}, plain) == ESP_OK,
           "SecureUpdate write");
    expect(update.finish(cipher) == ESP_OK && update.crypto.verified && update.writer.done, "SecureUpdate finish");
    expect(update.writer.bytes == 4U && plain[3] == 4U, "SecureUpdate plaintext write");
    expect(update.write({.ciphertext = cipher, .nonce = cipher, .aad = {}, .tag = cipher}, plain) ==
               ESP_ERR_INVALID_STATE,
           "SecureUpdate rejects write after finish");
    expect(update.finish(cipher) == ESP_ERR_INVALID_STATE, "SecureUpdate rejects duplicate finish");
    expect(
        update.write(
            {
                .ciphertext = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
                .nonce = cipher,
                .aad = {},
                .tag = cipher,
            },
            plain) == ESP_ERR_INVALID_ARG,
        "SecureUpdate rejects null ciphertext span");
    expect(
        update.write(
            {.ciphertext = cipher, .nonce = cipher, .aad = {}, .tag = cipher},
            std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), cipher.size()}) == ESP_ERR_INVALID_ARG,
        "SecureUpdate rejects null plaintext span");
    expect(update.finish(std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}) ==
               ESP_ERR_INVALID_ARG,
           "SecureUpdate rejects null signature span");

    esp_ota_stub_reset();
    arc::Ota::Session ota{};
    expect(arc::Ota::begin(ota, cipher.size(), &esp_ota_stub_next) == ESP_OK && ota.open(),
           "Ota begins explicit update slot");
    expect(arc::Ota::write(ota, nullptr, 1U) == ESP_ERR_INVALID_ARG && esp_ota_stub_written == 0U,
           "Ota write rejects null non-empty payload");
    expect(arc::Ota::write(ota, cipher.data(), cipher.size()) == ESP_OK &&
               esp_ota_stub_written == cipher.size(),
           "Ota write forwards valid payload");
    expect(arc::Ota::write_at(ota, nullptr, 1U, 4U) == ESP_ERR_INVALID_ARG &&
               esp_ota_stub_written_at == 0U,
           "Ota write_at rejects null non-empty payload");
    expect(arc::Ota::write_at(ota, cipher.data(), 2U, 4U) == ESP_OK &&
               esp_ota_stub_written_at == 2U && esp_ota_stub_last_offset == 4U,
           "Ota write_at forwards valid payload with offset");
    esp_ota_stub_end_result = ESP_FAIL;
    expect(arc::Ota::finish(ota) == ESP_FAIL && ota.open(),
           "Ota finish keeps session open after end failure");
    expect(arc::Ota::cancel(ota) == ESP_OK && !ota.open() && esp_ota_stub_aborted == 1U,
           "Ota cancel aborts preserved failed session");

    esp_ota_stub_reset();
    arc::Ota::Session ota_done{};
    esp_err_t ota_state_err{};
    esp_ota_stub_state = ESP_OTA_IMG_PENDING_VERIFY;
    expect(arc::Ota::begin(ota_done, cipher.size(), &esp_ota_stub_next) == ESP_OK &&
               arc::Ota::finish(ota_done) == ESP_OK && !ota_done.open() &&
               esp_ota_stub_boot_sets == 1U &&
               arc::Ota::pending_verify(&ota_state_err) && ota_state_err == ESP_OK,
           "Ota finish closes session and reports pending verify");
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

    using ScalarLqr = arc::dsp::Lqr<float, 1, 1>;
    const ScalarLqr::A lqr_a{{1.0F}};
    const ScalarLqr::B lqr_b{{1.0F}};
    const ScalarLqr::Q lqr_q{{1.0F}};
    const ScalarLqr::R lqr_r{{1.0F}};
    const ScalarLqr::P terminal{{1.0F}};
    const auto lqr_k = ScalarLqr::solve(lqr_a, lqr_b, lqr_q, lqr_r, terminal, 1U);
    expect(lqr_k.has_value() && near((*lqr_k)(0, 0), 0.6F), "LQR finite gain");
    const auto lqr_u = ScalarLqr::act(*lqr_k, ScalarLqr::State{{10.0F}});
    expect(near(lqr_u(0, 0), -6.0F), "LQR control action");
    const auto adapted_lqr = ScalarLqr::adapt(
        lqr_a,
        lqr_b,
        lqr_q,
        lqr_r,
        terminal,
        ScalarLqr::AdaptSample{
            .previous = ScalarLqr::State{{2.0F}},
            .input = ScalarLqr::Input{{1.0F}},
            .observed = ScalarLqr::State{{4.0F}},
        },
        {.rate = 0.1F, .limit = 0.5F, .epsilon = 0.001F, .steps = 1U});
    expect(adapted_lqr.has_value() &&
               adapted_lqr->a(0, 0) > lqr_a(0, 0) &&
               adapted_lqr->b(0, 0) > lqr_b(0, 0) &&
               adapted_lqr->residual > 0.0F,
           "LQR adapt identifies model deltas from observed state");
    expect(!ScalarLqr::adapt(lqr_a, lqr_b, lqr_q, lqr_r, terminal, {}, {.rate = 0.0F}),
           "LQR adapt rejects invalid config");
    expect(!arc::dsp::inverse(arc::dsp::Matrix<float, 1, 1>{{0.0F}}), "Matrix inverse rejects singular");

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
    struct FailingSink {
        std::size_t bytes{};
        bool fail{};
        esp_err_t write(std::span<const std::uint8_t> data) noexcept
        {
            if (fail) {
                return ESP_ERR_INVALID_STATE;
            }
            bytes += data.size();
            return ESP_OK;
        }
    };
    arc::FlashLog<Record, 4, FailingSink> failing_log{};
    expect(failing_log.push({.seq = 3U, .value = 4U}) &&
               (failing_log.sink.fail = true) &&
               failing_log.flush() == ESP_ERR_INVALID_STATE &&
               failing_log.queue.size() == 1U,
           "FlashLog keeps record queued after sink failure");
    failing_log.sink.fail = false;
    expect(failing_log.flush() == ESP_OK &&
               failing_log.queue.empty() &&
               failing_log.sink.bytes == sizeof(Record),
           "FlashLog retries queued record after sink recovery");

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
    expect(!Fleet::parse(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), sizeof(Fleet::Frame)}),
           "DistributedRcu parse rejects null wire span");

    Fleet slave{10U};
    expect(slave.assign(0U, 20U).has_value(), "DistributedRcu assign remote");
    const auto slot = slave.ingest(*decoded, schedule, 1'120);
    expect(slot.has_value() && *slot == 0U, "DistributedRcu ingest slot");
    auto zero_node_frame = *decoded;
    zero_node_frame.node_id = 0U;
    expect(!slave.ingest(zero_node_frame, 1'130), "DistributedRcu ingest rejects zero node");
    const auto remote = slave.read_remote(20U);
    expect(remote.has_value() && (*remote)->value == 42U, "DistributedRcu remote RCU read");
    expect(slave.stale(20U, 1'400, 100), "DistributedRcu stale detection");

    arc::GCounter<std::uint32_t, 4> gc_a{};
    arc::GCounter<std::uint32_t, 4> gc_b{};
    expect(gc_a.add(0U, 2U).has_value() && gc_a.add(1U, 3U).has_value(), "GCounter local increments");
    expect(gc_b.add(0U, 1U).has_value() && gc_b.add(2U, 5U).has_value(), "GCounter peer increments");
    expect(gc_a.merge(gc_b) && gc_a.value() == 10U, "GCounter merge keeps per-peer maxima");
    expect(gc_a.exact().has_value() && *gc_a.exact() == 10U, "GCounter exact sum");
    gc_a.cells[3] = std::numeric_limits<std::uint32_t>::max();
    expect(!gc_a.add(3U, 1U) && !gc_a.exact(), "GCounter rejects overflow");

    arc::PnCounter<std::uint32_t, 4> pn_a{};
    arc::PnCounter<std::uint32_t, 4> pn_b{};
    expect(pn_a.add(0U, 7U).has_value() && pn_a.sub(1U, 2U).has_value(), "PnCounter local mutation");
    expect(pn_b.add(2U, 1U).has_value() && pn_b.sub(1U, 4U).has_value(), "PnCounter peer mutation");
    expect(pn_a.merge(pn_b) && pn_a.value() == 4, "PnCounter converges signed value");

    arc::LwwReg<std::uint32_t> reg_a{};
    arc::LwwReg<std::uint32_t> reg_b{};
    expect(reg_a.set(10U, 5U, 1U).has_value(), "LwwReg first write");
    expect(reg_b.set(20U, 4U, 2U).has_value(), "LwwReg older peer write");
    expect(!reg_a.merge(reg_b) && reg_a.value() == 10U, "LwwReg ignores older stamp");
    expect(reg_b.set(30U, 5U, 2U).has_value(), "LwwReg tie-break write");
    expect(reg_a.merge(reg_b) && reg_a.value() == 30U, "LwwReg breaks ties by node");
    expect(!reg_a.set(31U, 5U, 1U), "LwwReg rejects non-monotonic local write");

    using SharedCount = arc::GCounter<std::uint32_t, 4>;
    using SharedCrdt = arc::Crdt<SharedCount, 4>;
    SharedCrdt drone_a{10U};
    SharedCrdt drone_b{20U};
    expect(drone_b.assign(3U, 99U).has_value() && !drone_b.assign(2U, 99U),
           "Crdt assign rejects duplicate node");
    expect(drone_a.state.add(0U, 4U).has_value(), "Crdt local counter state");
    expect(drone_b.state.add(1U, 5U).has_value(), "Crdt peer counter state");
    const auto crdt_frame = drone_a.transmit();
    expect(crdt_frame.has_value() && crdt_frame->node == 10U, "Crdt transmit frame");
    const auto crdt_wire = SharedCrdt::bytes(*crdt_frame);
    const auto crdt_decoded = SharedCrdt::parse(crdt_wire);
    expect(crdt_decoded.has_value(), "Crdt fixed frame parse");
    const auto crdt_slot = drone_b.ingest(*crdt_decoded);
    expect(crdt_slot.has_value() && *crdt_slot == 0U && drone_b.peers[0] == 10U && drone_b.state.value() == 9U,
           "Crdt ingest merges fixed state");
    auto zero_crdt_frame = *crdt_decoded;
    zero_crdt_frame.node = 0U;
    expect(!drone_b.ingest(zero_crdt_frame), "Crdt rejects zero node");
    expect(!SharedCrdt::parse(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), sizeof(SharedCrdt::Frame)}),
           "Crdt parse rejects null wire span");

    struct AbortVote {
        std::uint8_t abort{};
        auto operator==(const AbortVote&) const -> bool = default;
    };
    using AbortBft = arc::Bft<AbortVote, 4>;
    static_assert(AbortBft::faults() == 1U);
    static_assert(AbortBft::quorum() == 3U);
    AbortBft abort{2U, 9U};
    const AbortVote abort_yes{.abort = 1U};
    const AbortVote abort_no{.abort = 0U};
    const auto vote1 = abort.vote(1U, abort_yes, 0xA5U);
    const auto vote2 = abort.vote(2U, abort_yes, 0xA5U);
    const auto vote3 = abort.vote(3U, abort_yes, 0xA5U);
    expect(vote1.has_value() && vote2.has_value() && vote3.has_value(), "Bft creates fixed votes");
    expect(abort.ingest(*vote1).has_value() && abort.ingest(*vote1).has_value(), "Bft duplicate vote is idempotent");
    auto equivocate = *vote1;
    equivocate.value = abort_no;
    expect(!abort.ingest(equivocate), "Bft rejects same-node equivocation");
    expect(abort.ingest(*vote2).has_value() && !abort.cert(), "Bft waits for quorum");
    expect(abort.ingest(*vote3).has_value(), "Bft accepts quorum vote");
    const auto abort_cert = abort.cert();
    expect(abort_cert.has_value() && abort_cert->votes == 3U && abort_cert->value.abort == 1U,
           "Bft emits quorum certificate");
    auto wrong_view = *vote3;
    wrong_view.view = 3U;
    expect(!abort.ingest(wrong_view), "Bft rejects wrong view");
    const auto bft_wire = AbortBft::bytes(*vote1);
    const auto bft_decoded = AbortBft::parse(bft_wire);
    expect(bft_decoded.has_value() && bft_decoded->node == 1U && bft_decoded->value.abort == 1U,
           "Bft parses fixed vote frame");
    expect(!AbortBft::parse(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), sizeof(AbortBft::Vote)}),
           "Bft parse rejects null wire span");

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
    expect(stream.drain(lane, nullptr) == ESP_ERR_INVALID_ARG, "TraceStream rejects null event name");
    arc::TraceStream<192, TraceSink> empty_stream{};
    expect(empty_stream.finish() == ESP_OK &&
               std::string_view{empty_stream.sink.bytes.data(), empty_stream.sink.pos} == "{\"traceEvents\":[]}",
           "TraceStream finish emits empty document");
    struct FailingTraceSink {
        std::array<char, 384> bytes{};
        std::size_t pos{};
        std::size_t writes{};
        std::size_t fail_at{};
        esp_err_t write(std::span<const char> data) noexcept
        {
            ++writes;
            if (writes == fail_at) {
                return ESP_ERR_INVALID_STATE;
            }
            if (bytes.size() - pos < data.size()) {
                return ESP_ERR_NO_MEM;
            }
            std::memcpy(bytes.data() + pos, data.data(), data.size());
            pos += data.size();
            return ESP_OK;
        }
    };
    arc::LogLane<4> failing_lane{};
    expect(failing_lane.push(arc::log_id("trace.a"), 1U) &&
               failing_lane.push(arc::log_id("trace.b"), 2U) &&
               failing_lane.push(arc::log_id("trace.c"), 3U),
           "TraceStream queues sink-failure case");
    arc::TraceStream<192, FailingTraceSink> failing_stream{};
    failing_stream.sink.fail_at = 3U;
    expect(failing_stream.drain(failing_lane, "core1") == ESP_ERR_INVALID_STATE && failing_lane.size() == 2U,
           "TraceStream keeps failed event queued");
    expect(failing_stream.drain(failing_lane, "core1") == ESP_ERR_INVALID_STATE && failing_lane.size() == 2U,
           "TraceStream preserves queued events while failed");
    expect(failing_stream.finish() == ESP_ERR_INVALID_STATE && failing_stream.started && failing_lane.size() == 2U,
           "TraceStream finish preserves failed stream state");

    std::array<std::uint8_t, 32> pf{};
    const auto perfetto = arc::PerfettoWriter::event(pf, {.tick = 300U, .id = 2U, .payload = 4U, .aux = 1U});
    const auto null_perfetto = arc::PerfettoWriter::event(
        std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 8U},
        {.tick = 1U});
    expect(perfetto.has_value() && perfetto->size() > 0U && pf[0] == 0x08U && !null_perfetto.has_value(),
           "Perfetto binary event");

    std::array<std::uint8_t, 256> mcap_bytes{};
    arc::mcap::Writer mcap{.out = mcap_bytes};
    const std::array<std::uint8_t, 4> mcap_schema{'p', 'o', 's', 'e'};
    const std::array arc_metadata{
        arc::mcap::Meta{.key = "frame", .value = "map"},
    };
    expect(mcap.start("ros2", "arc").has_value() &&
               mcap.schema({.id = 1U, .name = "Pose2", .encoding = "json", .data = mcap_schema}).has_value() &&
               mcap.channel({
                                .id = 1U,
                                .schema_id = 1U,
                                .topic = "/pose",
                                .message_encoding = "json",
                                .metadata = arc_metadata,
                            })
                   .has_value(),
           "MCAP writes header schema and channel records");
    const std::array<std::uint8_t, 3> pose_payload{'{', '}', '\n'};
    const auto message_offset = mcap.pos;
    expect(mcap.message({
                            .channel = 1U,
                            .sequence = 7U,
                            .log_time = 0x0102'0304'0506'0708ULL,
                            .publish_time = 0x1112'1314'1516'1718ULL,
                            .data = pose_payload,
                        })
                   .has_value() &&
               mcap_bytes[message_offset] == static_cast<std::uint8_t>(arc::mcap::Op::message) &&
               mcap_bytes[message_offset + 1U] == 25U &&
               mcap_bytes[message_offset + 9U] == 1U &&
               mcap_bytes[message_offset + 11U] == 7U &&
               mcap_bytes[message_offset + 15U] == 0x08U &&
               mcap_bytes[message_offset + 31U] == static_cast<std::uint8_t>('{'),
           "MCAP message record uses little-endian header and timestamp fields");
    expect(mcap.finish().has_value() &&
               mcap.finished &&
               mcap.bytes().size() > arc::mcap::Writer::magic.size() * 2U &&
               mcap.bytes()[0] == 0x89U &&
               mcap.bytes()[arc::mcap::Writer::magic.size()] == static_cast<std::uint8_t>(arc::mcap::Op::header) &&
               mcap.bytes()[9] == 15U &&
               mcap.bytes()[mcap.bytes().size() - arc::mcap::Writer::magic.size()] == 0x89U,
           "MCAP finishes DataEnd Footer and trailing magic");
    arc::mcap::Writer null_mcap{.out = std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U}};
    expect(!null_mcap.start("ros2", "arc"), "MCAP rejects null output span");
    arc::mcap::Writer tiny_mcap{.out = std::span<std::uint8_t>{mcap_bytes.data(), 8U}};
    expect(!tiny_mcap.start("ros2", "arc"), "MCAP rejects undersized output span");
    std::array<std::uint8_t, 48> short_tail_bytes{};
    arc::mcap::Writer short_tail{.out = short_tail_bytes};
    expect(short_tail.start("r", "a").has_value(), "MCAP starts with a minimal header");
    const auto short_tail_pos = short_tail.pos;
    expect(!short_tail.finish() && short_tail.pos == short_tail_pos && !short_tail.finished,
           "MCAP rejects undersized footer without partial tail");

    std::array<std::uint8_t, 64> xrce_bytes{};
    arc::xrce::Writer xrce{.out = xrce_bytes};
    const arc::xrce::Header xrce_header{
        .session = 0x81U,
        .stream = arc::xrce::stream_reliable,
        .seq = 0x1234U,
        .key = {.bytes = {0x44U, 0x33U, 0x22U, 0x11U}},
        .keyed = true,
    };
    const std::array<std::uint8_t, 3> xrce_payload{0xdeU, 0xadU, 0xbeU};
    expect(xrce.header(xrce_header).has_value() &&
               xrce.sub({
                            .id = arc::xrce::Sub::write_data,
                            .flags = arc::xrce::little,
                            .payload = xrce_payload,
                        })
                   .has_value() &&
               xrce_bytes[0] == 0x81U &&
               xrce_bytes[1] == arc::xrce::stream_reliable &&
               xrce_bytes[2] == 0x34U &&
               xrce_bytes[3] == 0x12U &&
               xrce_bytes[8] == static_cast<std::uint8_t>(arc::xrce::Sub::write_data) &&
               xrce_bytes[10] == 3U &&
               xrce_bytes[12] == 0xdeU,
           "XRCE writes keyed message and little-endian submessage");
    const auto xrce_frame = xrce.bytes();
    const auto parsed_xrce_header = arc::xrce::read_header(xrce_frame, true);
    expect(parsed_xrce_header.has_value() &&
               parsed_xrce_header->next == 8U &&
               parsed_xrce_header->header.seq == 0x1234U &&
               parsed_xrce_header->header.key.bytes[3] == 0x11U &&
               arc::xrce::reliable(parsed_xrce_header->header.stream),
           "XRCE parses keyed message header");
    const auto parsed_xrce_sub = arc::xrce::read_sub(xrce_frame, parsed_xrce_header->next);
    expect(parsed_xrce_sub.has_value() &&
               parsed_xrce_sub->id == arc::xrce::Sub::write_data &&
               parsed_xrce_sub->payload.size() == xrce_payload.size() &&
               parsed_xrce_sub->payload[2] == 0xbeU,
           "XRCE parses submessage payload");
    std::array<std::uint8_t, 8> xrce_big_bytes{};
    arc::xrce::Writer xrce_big{.out = xrce_big_bytes};
    const auto xrce_reserved = xrce_big.reserve(arc::xrce::Sub::data, 0U, 2U);
    expect(xrce_reserved.has_value() &&
               xrce_big_bytes[2] == 0U &&
               xrce_big_bytes[3] == 2U &&
               arc::xrce::best(arc::xrce::stream_best),
           "XRCE reserves big-endian submessage payloads");
    arc::xrce::Writer tiny_xrce{.out = std::span<std::uint8_t>{xrce_big_bytes.data(), 3U}};
    expect(!tiny_xrce.header(xrce_header) && tiny_xrce.pos == 0U, "XRCE rejects undersized header without partial write");

    arc::Provisioning<32, 64, 128> provisioning{};
    const std::array<std::uint8_t, 4> nonce{1, 2, 3, 4};
    const std::array<std::uint8_t, 3> key{5, 6, 7};
    const std::array<std::uint8_t, 4> ssid{'a', 'r', 'c', 0};
    expect(provisioning.begin(nonce) == ESP_OK, "Provisioning begin");
    expect(provisioning.keys(key) == ESP_OK, "Provisioning keys");
    expect(provisioning.credentials(ssid, key) == ESP_OK && provisioning.ready(), "Provisioning credentials");
    arc::Provisioning<32, 64, 128> null_nonce_provisioning{};
    expect(null_nonce_provisioning.begin(std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}) ==
               ESP_ERR_INVALID_ARG,
           "Provisioning rejects null nonce span");
    arc::Provisioning<32, 64, 128> early_key_provisioning{};
    expect(early_key_provisioning.keys(key) == ESP_ERR_INVALID_STATE,
           "Provisioning rejects key before nonce");
    arc::Provisioning<32, 64, 128> null_key_provisioning{};
    expect(null_key_provisioning.begin(nonce) == ESP_OK &&
               null_key_provisioning.keys(std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}) ==
                   ESP_ERR_INVALID_ARG,
           "Provisioning rejects null key span");
    arc::Provisioning<32, 64, 128> early_credentials_provisioning{};
    expect(early_credentials_provisioning.credentials(ssid, key) == ESP_ERR_INVALID_STATE,
           "Provisioning rejects credentials before keys");
    arc::Provisioning<32, 64, 128> null_credentials_provisioning{};
    expect(null_credentials_provisioning.begin(nonce) == ESP_OK &&
               null_credentials_provisioning.keys(key) == ESP_OK &&
               null_credentials_provisioning.credentials(
                   std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
                   key) == ESP_ERR_INVALID_ARG,
           "Provisioning rejects null SSID span");
    arc::Provisioning<32, 64, 128> null_pass_provisioning{};
    expect(null_pass_provisioning.begin(nonce) == ESP_OK &&
               null_pass_provisioning.keys(key) == ESP_OK &&
               null_pass_provisioning.credentials(
                   ssid,
                   std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}) == ESP_ERR_INVALID_ARG,
           "Provisioning rejects null password span");
    arc::Provisioning<32, 64, 128> null_cert_provisioning{};
    expect(null_cert_provisioning.begin(nonce) == ESP_OK &&
               null_cert_provisioning.keys(key) == ESP_OK &&
               null_cert_provisioning.credentials(
                   ssid,
                   key,
                   std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}) == ESP_ERR_INVALID_ARG,
           "Provisioning rejects null certificate span");

    arc::net::EthernetRing<128, 4> eth{};
    const std::array<std::uint8_t, 6> frame{0, 1, 2, 3, 4, 5};
    expect(eth.push(frame), "Ethernet frame push");
    arc::net::EthernetRing<128, 4>::Frame popped{};
    const auto null_eth = eth.push(std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U});
    expect(eth.pop(popped) && popped.size == frame.size() && popped.bytes[5] == 5U && !null_eth,
           "Ethernet frame pop");

    arc::net::TsnSchedule<3> tsn{.cycle_ns = 1'000'000U, .base_ns = 10'000U};
    expect(tsn.set(0U, {.offset_ns = 0U, .duration_ns = 100'000U, .guard_ns = 10'000U, .traffic = 3U}).has_value() &&
               tsn.set(1U, {.offset_ns = 200'000U, .duration_ns = 50'000U, .guard_ns = 5'000U, .traffic = 7U}).has_value(),
           "TSN schedule installs guarded gates");
    expect(!tsn.set(2U, {.offset_ns = 400'000U, .duration_ns = 10U, .guard_ns = 6U, .traffic = 1U}),
           "TSN schedule rejects closed guard window");
    expect(!tsn.set(2U, {.offset_ns = 400'000U, .duration_ns = 10U, .guard_ns = 5U, .traffic = 1U}),
           "TSN schedule rejects zero-width guard window");
    const auto tsn_window = tsn.window(30'000U, 3U);
    expect(tsn_window.has_value() && tsn_window->open && tsn_window->start_ns == 20'000U &&
               tsn_window->end_ns == 100'000U,
           "TSN window opens inside guarded gate");
    expect(!tsn.allow(15'000U, 3U), "TSN guard time closes early phase");
    expect(tsn.allow(217'000U, 7U), "TSN allows matching traffic class");
    const auto next_tsn = tsn.next_open(105'000U, 7U);
    expect(next_tsn.has_value() && *next_tsn == 215'000U, "TSN reports next same-cycle open");
    const auto wrapped_tsn = tsn.next_open(260'000U, 7U);
    expect(wrapped_tsn.has_value() && *wrapped_tsn == 1'215'000U, "TSN reports next-cycle open");
    expect(!tsn.next_open(260'000U, 9U), "TSN rejects missing traffic class");

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

    expect(lane.push(event, 1U) && lane.push(event, 2U) && lane.push(event, 3U),
           "LogLane drain stop seed");
    std::uint32_t stopped_payloads{};
    const auto stopped = lane.drain([&](const arc::LogEvent& item) noexcept {
        stopped_payloads += item.payload;
        return item.payload != 2U;
    });
    expect(stopped == 2U && stopped_payloads == 3U && lane.size() == 1U,
           "LogLane drain stops on false callback");
    arc::LogEvent retained{};
    expect(lane.pop(retained) && retained.payload == 3U, "LogLane drain leaves later events queued");
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
    expect(text.append("abcd"), "Text overlap seed");
    text.clear();
    expect(text.push('x') && text.push('y'), "Text overlap prefix");
    expect(text.append(std::span<const char>{out.data() + 1U, 3U}) &&
               std::string_view{text.view()} == "xyycd",
           "Text append preserves overlapping source span");
    text.clear();
    expect(text.append("a\"b"), "Text json overlap seed");
    text.clear();
    expect(text.json(std::string_view{out.data(), 3U}) &&
               std::string_view{text.view()} == "a\\\"b",
           "Text json preserves in-place expansion");
    text.clear();
    expect(text.push('x') && text.append("a\"b"), "Text json shifted overlap seed");
    text.clear();
    expect(!text.json(std::string_view{out.data() + 1U, 3U}) && text.empty(),
           "Text json rejects shifted overlap");

    std::array<char, 4> small{};
    arc::Text tiny{std::span(small)};
    expect(!tiny.append("toolong"), "Text rejects oversized append");
    expect(tiny.empty(), "Text oversized append is not partial");
    expect(tiny.append("1234") && tiny.full(), "Text fills exactly");
    expect(!tiny.push('5'), "Text rejects full push");

    arc::Text empty{};
    expect(empty.view().empty() && !empty.push('x'), "Text default storage is inert");

    arc::Text null_text{std::span<char>{static_cast<char*>(nullptr), 16U}};
    expect(!null_text.u64(42U) && !null_text.hex(0x2aU), "Text numeric writers reject null storage");

    std::array<char, 2> signed_small{};
    arc::Text negative{std::span(signed_small)};
    expect(!negative.i32(-123) && negative.empty(), "Text failed signed write rolls back");

    std::array<char, 4> json_small{};
    arc::Text escaped{std::span(json_small)};
    expect(!escaped.json("ab\nc") && escaped.empty(), "Text failed json write rolls back");

    std::array<char, 64> fmt{};
    const auto formatted = arc::format_to(
        std::span(fmt),
        "temp={} mv={} ok={} pc=0x{} {{}}",
        -12,
        3300U,
        true,
        arc::hex(0x1afU, 4U));
    expect(
        formatted.has_value() && std::string_view{formatted->data(), formatted->size()} == "temp=-12 mv=3300 ok=true pc=0x01af {}",
        "format_to writes common telemetry without heap formatting");
    std::array<char, 4> small_fmt{};
    expect(!arc::format_to(std::span(small_fmt), "{}", "too long"), "format_to rejects small buffer");
    expect(!arc::format_to(std::span(fmt), "{} {}", 1), "format_to rejects missing arg");
    expect(!arc::format_to(std::span(fmt), "{}", 1, 2), "format_to rejects extra arg");
    expect(!arc::format_to(std::span(fmt), "{", 1), "format_to rejects malformed braces");
    expect(!arc::format_to(
               std::span<char>{static_cast<char*>(nullptr), fmt.size()},
               "x"),
           "format_to rejects null output span");
    expect(!arc::format_to(
               std::span(fmt),
               std::string_view{static_cast<const char*>(nullptr), 1U}),
           "format_to rejects null format view");
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
    expect(!arc::TraceEventWriter::json_begin(std::span<char>{static_cast<char*>(nullptr), 1U}),
           "TraceEvent begin rejects null output span");
    expect(!arc::TraceEventWriter::json_end(std::span<char>{static_cast<char*>(nullptr), 1U}),
           "TraceEvent end rejects null output span");
    expect(!arc::TraceEventWriter::json_event(
               std::span<char>{static_cast<char*>(nullptr), out.size()},
               event),
           "TraceEvent event rejects null output span");
    expect(!arc::TraceEventWriter::json_event(std::span(out), event, false, nullptr),
           "TraceEvent event rejects null name");
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
    expect(!Telemetry::write(
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), Telemetry::bytes},
               Kind::sample,
               0x1234U,
               -2),
           "Pack write rejects null output span");
    expect(!Telemetry::read(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), Telemetry::bytes},
               kind,
               id,
               value),
           "Pack read rejects null input span");

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
    expect(
        !arc::pack::serialize<arc::pack::Endian::big, PlainWire>(
            std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), PlainWire::bytes},
            plain),
        "Pack struct write rejects null output span");
    expect(
        !arc::pack::deserialize<arc::pack::Endian::big, PlainWire>(
            std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), PlainWire::bytes},
            decoded),
        "Pack struct read rejects null input span");
    const arc::pack::Overlay<PlainWire> null_overlay{
        .bytes = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), PlainWire::bytes},
    };
    expect(!null_overlay.valid(), "Pack overlay rejects null byte span");

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
    const auto crypto_job = arc::CryptoDmaJob{
        .input = input.head(),
        .output = output.head(),
        .bytes = in_bytes.size(),
        .mode = arc::CryptoDmaMode::gcm_encrypt,
    };
    expect((arc::crypto_submit<1, 1, MockCryptoDmaPolicy>(
                input,
                output,
                in_bytes.size(),
                arc::CryptoDmaMode::gcm_encrypt)
                .has_value()),
           "CryptoDma submit");
    expect(crypto_dma_bytes == in_bytes.size() && arc::CryptoDma<MockCryptoDmaPolicy>::done(), "CryptoDma policy");
    auto crypto_lease = arc::CryptoDma<MockCryptoDmaPolicy>::lease(crypto_job);
    expect(crypto_lease.has_value() && crypto_lease->active(), "CryptoDma lease submit");
    expect(crypto_lease->finish().has_value() && !crypto_lease->active(), "CryptoDma lease finish");
    expect(crypto_lease->finish().has_value(), "CryptoDma lease finish is idempotent");
    auto null_iv_job = crypto_job;
    null_iv_job.iv = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U};
    expect(!arc::CryptoDma<MockCryptoDmaPolicy>::submit(null_iv_job), "CryptoDma rejects null IV span");
    auto null_tag_job = crypto_job;
    null_tag_job.tag = std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U};
    expect(!arc::CryptoDma<MockCryptoDmaPolicy>::lease(null_tag_job), "CryptoDma lease rejects null tag span");
    auto pending_crypto = arc::CryptoDma<PendingCryptoDmaPolicy>::lease(crypto_job);
    expect(pending_crypto.has_value(), "CryptoDma pending lease submit");
    expect(!pending_crypto->wait_for(2U) && pending_crypto->active(), "CryptoDma bounded wait reports unfinished job");
    PendingCryptoDmaPolicy::complete = true;
    expect(pending_crypto->wait_for(2U) && !pending_crypto->active(), "CryptoDma bounded wait observes completion");

    auto mapped = arc::MmuSpan<std::uint32_t>::map<MockMmuPolicy>(
        {.source = 0U, .offset = 0U, .bytes = 3U * sizeof(std::uint32_t), .memory = 0U});
    expect(mapped.has_value(), "MmuSpan map");
    expect(mapped->size() == 3U && (*mapped)[1] == 0x20U, "MmuSpan view");
    mapped->unmap<MockMmuPolicy>();
    expect(MockMmuPolicy::unmapped, "MmuSpan unmap");
    expect(!arc::MmuSpan<std::uint32_t>::map<NullMmuPolicy>(
               {.source = 0U, .offset = 0U, .bytes = 3U * sizeof(std::uint32_t), .memory = 0U}),
           "MmuSpan rejects null policy mapping");
    expect(!arc::MmuSpan<std::uint32_t>::map<ShortMmuPolicy>(
               {.source = 0U, .offset = 0U, .bytes = 3U * sizeof(std::uint32_t), .memory = 0U}),
           "MmuSpan rejects undersized policy mapping");
    expect(!arc::MmuSpan<std::uint32_t>::map<OddMmuPolicy>(
               {.source = 0U, .offset = 0U, .bytes = 3U * sizeof(std::uint32_t), .memory = 0U}),
           "MmuSpan rejects unaligned policy mapping");

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

    arc_host_lp_word.write(3U);
    expect(arc_host_lp_word.add(4U) == 7U && arc_host_lp_word.read() == 7U, "LP core shared word");

    struct LpPayload {
        std::uint32_t seq{};
        std::uint32_t flags{};
    };

    static_assert(sizeof(LpPayload) > sizeof(std::uint32_t));
    arc::lp::Mailbox<LpPayload> mailbox{};
    mailbox.host_write({.seq = 11U, .flags = 0xaaU});
    LpPayload lp_seen{};
    expect(mailbox.lp_read(lp_seen) && lp_seen.seq == 11U && lp_seen.flags == 0xaaU,
           "LP core mailbox reads host payload");
    mailbox.lp_write({.seq = 12U, .flags = 0x55U});
    LpPayload host_seen{};
    expect(mailbox.host_read(host_seen) && host_seen.seq == 12U && host_seen.flags == 0x55U,
           "LP core mailbox reads LP payload");

    struct LpPolicy {
        int loads{};
        int starts{};
        int stops{};

        [[nodiscard]] esp_err_t load(const arc::lp::Image& image) noexcept
        {
            ++loads;
            return image.load == 0x5000'0000U ? ESP_OK : ESP_FAIL;
        }

        [[nodiscard]] esp_err_t start(const arc::lp::Control& control) noexcept
        {
            ++starts;
            return control.wake_main ? ESP_OK : ESP_FAIL;
        }

        [[nodiscard]] esp_err_t stop() noexcept
        {
            ++stops;
            return ESP_OK;
        }
    };

    const std::array<std::uint8_t, 4> lp_binary{0x13U, 0U, 0U, 0U};
    const arc::lp::Image lp_image{
        .binary = std::span<const std::uint8_t>{lp_binary},
        .load = 0x5000'0000U,
        .entry = arc_host_lp_entry,
        .stack = 256U,
    };
    const arc::lp::Control lp_control{
        .image = lp_image,
        .period_us = 1000U,
        .wake_main = true,
    };
    LpPolicy lp_policy{};
    expect(arc::lp::load(lp_policy, lp_image).has_value() &&
               arc::lp::start(lp_policy, lp_control).has_value() &&
               arc::lp::stop(lp_policy).has_value() &&
               lp_policy.loads == 1 && lp_policy.starts == 1 && lp_policy.stops == 1,
           "LP core policy load start stop");
    expect(!arc::lp::load(lp_policy, {}) && !arc::lp::start(lp_policy, {}),
           "LP core rejects invalid image and control");
    struct EmptyLpPolicy {};
    EmptyLpPolicy empty_lp{};
    const auto unsupported_lp = arc::lp::load(empty_lp, lp_image);
    expect(!unsupported_lp && unsupported_lp.error() == ESP_ERR_NOT_SUPPORTED, "LP core reports unsupported policy hooks");
}

void test_probe_stats()
{
    using ControlProof = arc::proof::Pack<
        10'000U,
        arc::proof::Fact<arc::proof::Kind::deadline, 17U, 10'000U>,
        arc::proof::Fact<arc::proof::Kind::no_heap, 17U>,
        arc::proof::Fact<arc::proof::Kind::static_life, 17U>>;
    static_assert(ControlProof::cycles == 10'000U);
    static_assert(ControlProof::count == 3U);
    static_assert(ControlProof::has<arc::proof::Kind::deadline>());
    static_assert(!ControlProof::has<arc::proof::Kind::no_null>());
    static_assert(ControlProof::bound<arc::proof::Kind::deadline>() == 10'000U);
    constexpr auto proof_claims = ControlProof::claims();
    static_assert(proof_claims[0].kind == arc::proof::Kind::deadline);
    static_assert(proof_claims[2].kind == arc::proof::Kind::static_life);
    expect(proof_claims.size() == 3U && proof_claims[1].subject == 17U,
           "Proof pack carries compile-time workload claims");

    arc::CycleStats cycles{};
    cycles.add(10U);
    cycles.add(30U);
    expect(cycles.min == 10U && cycles.max == 30U && cycles.avg() == 20U, "CycleStats aggregate");
    cycles.samples = std::numeric_limits<std::uint32_t>::max();
    cycles.total = std::numeric_limits<std::uint64_t>::max() - 1U;
    cycles.add(30U);
    expect(cycles.samples == std::numeric_limits<std::uint32_t>::max() &&
               cycles.total == std::numeric_limits<std::uint64_t>::max() &&
               cycles.avg() == std::numeric_limits<esp_cpu_cycle_count_t>::max(),
           "CycleStats saturates counter and total overflow");
    cycles.clear();
    expect(cycles.samples == 0U && cycles.max == 0U, "CycleStats clear");

    arc::JitterStats jitter{};
    jitter.add(-5);
    jitter.add(7);
    jitter.add(1);
    expect(jitter.min == -5 && jitter.max == 7, "JitterStats min/max");
    expect(jitter.avg_abs() == 4U && jitter.max_abs() == 7U, "JitterStats abs");
    jitter.samples = std::numeric_limits<std::uint32_t>::max();
    jitter.abs_total = std::numeric_limits<std::uint64_t>::max() - 1U;
    jitter.add(std::numeric_limits<std::int32_t>::min());
    expect(jitter.samples == std::numeric_limits<std::uint32_t>::max() &&
               jitter.abs_total == std::numeric_limits<std::uint64_t>::max() &&
               jitter.avg_abs() == std::numeric_limits<std::uint32_t>::max(),
           "JitterStats saturates abs accumulation overflow");
    jitter.clear();
    expect(jitter.samples == 0U && jitter.avg_abs() == 0U, "JitterStats clear");

    arc::DeadlineStats deadline{};
    deadline.add(80U, 100U);
    deadline.add(130U, 100U);
    expect(deadline.min_slack == -30 && deadline.max_slack == 20, "DeadlineStats slack");
    expect(deadline.avg_slack() == -5 && deadline.overruns == 1U && deadline.max_overrun == 30, "DeadlineStats overrun");
    deadline.samples = std::numeric_limits<std::uint32_t>::max();
    deadline.overruns = std::numeric_limits<std::uint32_t>::max();
    deadline.total_slack = std::numeric_limits<std::int64_t>::min() + 1;
    deadline.add(std::numeric_limits<esp_cpu_cycle_count_t>::max(), 0U);
    expect(deadline.samples == std::numeric_limits<std::uint32_t>::max() &&
               deadline.overruns == std::numeric_limits<std::uint32_t>::max() &&
               deadline.total_slack == std::numeric_limits<std::int64_t>::min() &&
               deadline.max_overrun == std::numeric_limits<std::int32_t>::max() &&
               deadline.avg_slack() == std::numeric_limits<std::int32_t>::min(),
           "DeadlineStats saturates slack and overrun counters");
    deadline.clear();
    expect(deadline.samples == 0U && deadline.overruns == 0U, "DeadlineStats clear");

    arc::StallStats stalls{};
    stalls.add(100U, 90U, 10U);
    stalls.add(125U, 90U, 10U);
    stalls.add(UINT32_MAX, 0U, 0U);
    expect(stalls.samples == 3U && stalls.stalls == 2U && stalls.max == UINT32_MAX, "StallStats counts excess cycles");
    expect(stalls.avg() == ((25ULL + UINT32_MAX) / 2ULL), "StallStats averages stall excess");
    stalls.samples = std::numeric_limits<std::uint32_t>::max();
    stalls.stalls = std::numeric_limits<std::uint32_t>::max();
    stalls.total = std::numeric_limits<std::uint64_t>::max() - 1U;
    stalls.add(std::numeric_limits<esp_cpu_cycle_count_t>::max(), 0U, 0U);
    expect(stalls.samples == std::numeric_limits<std::uint32_t>::max() &&
               stalls.stalls == std::numeric_limits<std::uint32_t>::max() &&
               stalls.total == std::numeric_limits<std::uint64_t>::max() &&
               stalls.avg() == std::numeric_limits<esp_cpu_cycle_count_t>::max(),
           "StallStats saturates stall counters and totals");
    stalls.clear();
    expect(stalls.samples == 0U && stalls.stalls == 0U && stalls.avg() == 0U, "StallStats clear");

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
        expect(!handle.write(nullptr, 1U).has_value(), "File write rejects null non-empty");
        const auto empty = handle.write(nullptr, 0U);
        expect(empty.has_value() && empty.value() == 0U, "File write accepts empty null");
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
        expect(!handle.read(nullptr, 1U).has_value(), "File read rejects null non-empty");
        const auto empty = handle.read(nullptr, 0U);
        expect(empty.has_value() && empty.value() == 0U, "File read accepts empty null");
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

void test_fs()
{
    expect(arc::Fs::spiffs("/fs", "storage", 2U, true) == ESP_OK, "Fs SPIFFS mount");
    expect(arc::Fs::spiffs_on("storage"), "Fs SPIFFS mounted");
    arc::Fs::SpiffsInfo spiffs{};
    expect(arc::Fs::spiffs_info(spiffs, "storage") == ESP_OK && spiffs.total == 64U && spiffs.used == 8U,
           "Fs SPIFFS info");
    expect(arc::Fs::spiffs_check("storage") == ESP_OK && arc::Fs::spiffs_gc(4U, "storage") == ESP_OK &&
               arc::Fs::spiffs_format("storage") == ESP_OK && arc::Fs::spiffs_off("storage") == ESP_OK &&
               !arc::Fs::spiffs_on("storage"),
           "Fs SPIFFS maintenance");

    expect(arc::Fs::fat("/fat", "storage", 2, true, 4096U) == ESP_OK, "Fs FAT mount");
    arc::Fs::FatInfo fat{};
    expect(arc::Fs::fat_info(fat, "/fat") == ESP_OK && fat.total == 128U && fat.free == 32U, "Fs FAT info");
    expect(arc::Fs::fat_format("/fat", "storage") == ESP_OK && arc::Fs::fat_off() == ESP_OK, "Fs FAT off");
    expect(arc::Fs::fat_ro("/fat", "storage", 2) == ESP_OK && arc::Fs::ro_off("/fat", "storage") == ESP_OK,
           "Fs FAT read-only mount");

#if ARC_HAS_LITTLEFS
    expect(arc::Fs::littlefs("/littlefs", "storage", true) == ESP_OK, "Fs LittleFS mount");
    expect(arc::Fs::littlefs_on("storage"), "Fs LittleFS mounted");
    arc::Fs::LittlefsInfo littlefs{};
    expect(arc::Fs::littlefs_info(littlefs, "storage") == ESP_OK && littlefs.total == 256U &&
               littlefs.used == 16U,
           "Fs LittleFS info");
    expect(arc::Fs::littlefs_format("storage") == ESP_OK && arc::Fs::littlefs_off("storage") == ESP_OK &&
               !arc::Fs::littlefs_on("storage"),
           "Fs LittleFS maintenance");
#endif
}

void test_store()
{
    expect(nvs_flash_erase() == ESP_OK, "Store host reset");
    expect(arc::Store::boot() == ESP_OK, "Store boot");

    struct Control {
        std::uint16_t pace;
        std::uint8_t mark;
        std::uint8_t flags;
    };
    static_assert(std::is_trivially_copyable_v<Control>);

    constexpr Control fallback{.pace = 2000U, .mark = 0x31U, .flags = 0x01U};
    constexpr Control stored{.pace = 1000U, .mark = 0x42U, .flags = 0x03U};

    esp_err_t status = ESP_OK;
    auto cfg = arc::Store::load_or("cfg", "control", fallback, &status);
    expect(status == ESP_ERR_NVS_NOT_FOUND && cfg.pace == fallback.pace, "Store load_or falls back on missing blob");
    expect(arc::Store::save("cfg", "control", stored) == ESP_OK, "Store saves typed blob");
    cfg = arc::Store::load_or("cfg", "control", fallback, &status);
    expect(status == ESP_OK && cfg.mark == stored.mark && cfg.flags == stored.flags, "Store loads typed blob");

    arc::StoreText<16> text{};
    expect(text.set("fallback") == ESP_OK && text.view() == "fallback" && text.c_str()[8] == '\0',
           "StoreText owns NUL-terminated fallback");
    expect(text.set("this-name-is-too-long") == ESP_ERR_NVS_INVALID_LENGTH && text.view() == "fallback",
           "StoreText rejects oversized text without mutation");
    const std::array<char, 3> embedded_nul_text{'a', '\0', 'b'};
    expect(text.set(std::string_view{embedded_nul_text.data(), embedded_nul_text.size()}) ==
                   ESP_ERR_INVALID_ARG &&
               text.view() == "fallback",
           "StoreText rejects embedded NUL without mutation");
    expect(text.set(std::string_view{static_cast<const char*>(nullptr), 1U}) == ESP_ERR_INVALID_ARG &&
               text.view() == "fallback",
           "StoreText rejects null non-empty string view");

    expect(arc::Store::save_text<16>("cfg", "name", "router.local") == ESP_OK, "Store saves string_view text");
    auto loaded = arc::Store::load_text<16>("cfg", "name", "fallback", &status);
    expect(loaded.has_value() && status == ESP_OK && loaded->view() == "router.local" &&
               std::strcmp(loaded->c_str(), "router.local") == 0,
           "Store loads fixed text config");

    auto missing = arc::Store::load_text<16>("cfg", "missing", "fallback", &status);
    expect(missing.has_value() && status == ESP_ERR_NVS_NOT_FOUND && missing->view() == "fallback",
           "Store load_text keeps fallback for missing key");
    expect(!arc::Store::load_text<4>("cfg", "name", "ok", &status) && status == ESP_ERR_NVS_INVALID_LENGTH,
           "Store load_text rejects oversized persisted value");
    expect(arc::Store::save_text<4>("cfg", "tiny", "too long") == ESP_ERR_NVS_INVALID_LENGTH,
           "Store save_text rejects oversized source");
    expect(arc::Store::save_text<16>(
               "cfg",
               "nul",
               std::string_view{embedded_nul_text.data(), embedded_nul_text.size()}) == ESP_ERR_INVALID_ARG,
           "Store save_text rejects embedded NUL source");
    std::size_t loaded_chars{};
    expect(arc::Store::load_string(
               "cfg",
               "name",
               std::span<char>{static_cast<char*>(nullptr), 16U},
               &loaded_chars) == ESP_ERR_INVALID_ARG,
           "Store load_string rejects null output span");
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
    expect(erased.send_all(nullptr, 0U).has_value() && io.tx_pos == 0U,
           "AnyStream empty send is no-op");
    const auto empty_recv = erased.recv(nullptr, 0U);
    expect(empty_recv.has_value() && *empty_recv == 0U && io.rx_pos == 0U,
           "AnyStream empty recv is no-op");

    expect(arc::net::Stream::write_frame16(io, std::span(payload)).has_value(), "Stream frame write");
    expect(io.tx_pos == 5U, "Stream frame bytes");
    expect(io.tx[0] == 0U && io.tx[1] == 3U && io.tx[4] == 3U, "Stream frame layout");
    expect(arc::net::Stream::write_frame16(io, nullptr, 0U).has_value(), "Stream empty frame write");
    expect(io.tx_pos == 7U && io.tx[5] == 0U && io.tx[6] == 0U, "Stream empty frame layout");

    std::array<std::uint8_t, 8> encoded{};
    const auto coded = arc::net::Stream::frame16(std::span(encoded), std::span(payload));
    expect(coded.has_value() && coded->size() == 5U, "Stream frame encode");
    expect((*coded)[0] == 0U && (*coded)[1] == 3U && (*coded)[4] == 3U, "Stream frame encode layout");
    std::array<std::uint8_t, 8> in_place_frame{1U, 2U, 3U};
    const auto in_place_coded = arc::net::Stream::frame16(
        std::span(in_place_frame),
        std::span(in_place_frame).first(3U));
    expect(in_place_coded.has_value() &&
               (*in_place_coded)[0] == 0U &&
               (*in_place_coded)[1] == 3U &&
               (*in_place_coded)[2] == 1U &&
               (*in_place_coded)[3] == 2U &&
               (*in_place_coded)[4] == 3U,
           "Stream frame preserves in-place payload");
    expect(
        !arc::net::Stream::frame16(
            std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), encoded.size()},
            std::span(payload)),
        "Stream frame rejects null output span");

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

    io.rx = {0U, 1U, 0xaaU};
    io.rx_pos = 0U;
    io.rx_len = 3U;
    expect(!arc::net::Stream::read_frame16(
               io,
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U}) &&
               io.rx_pos == 0U,
           "Stream frame read rejects null output before consuming header");
}

void test_pipeline_usb_ulp()
{
    arc::DmaChain<2> camera{};
    arc::DmaChain<2> display{};
    std::array<std::uint8_t, 8> camera_bytes{};
    std::array<std::uint8_t, 8> display_bytes{};
    camera.bind(0, std::span(camera_bytes).first(4));
    camera.bind(1, std::span(camera_bytes).subspan(4), true);
    expect(!camera.try_bind(2, std::span(camera_bytes).first(1)), "DmaChain checked bind rejects index");
    expect(!camera.try_bind(0, std::span<std::uint8_t>{}), "DmaChain checked bind rejects empty span");
    static_assert(arc::detail::dma_bytes_fit<std::uint16_t>(
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) / sizeof(std::uint16_t)));
    static_assert(!arc::detail::dma_bytes_fit<std::uint16_t>(
        (static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) / sizeof(std::uint16_t)) + 1U));
    std::array<std::uint16_t, 2> typed_dma{};
    arc::DmaChain<1> typed_chain{};
    expect(typed_chain.try_bind(0, std::span(typed_dma)).has_value() &&
               typed_chain.head()->length == typed_dma.size() * sizeof(std::uint16_t),
           "DmaChain checked bind accepts typed spans");
    alignas(arc::cache_line) std::array<std::uint8_t, 128> coherent_dma{};
    arc::DmaChain<1> coherent_chain{};
    expect(coherent_chain.try_bind(0, arc::CacheLines{.data = coherent_dma.data(), .bytes = 128U}).has_value() &&
               coherent_chain.head()->buffer == coherent_dma.data() &&
               coherent_chain.head()->length == 128U,
           "DmaChain cache-line bind accepts coherent spans");
    expect(!coherent_chain.try_bind(0, arc::CacheLines{.data = coherent_dma.data() + 1U, .bytes = 64U}),
           "DmaChain cache-line bind rejects unaligned data");
    expect(!coherent_chain.try_bind(0, arc::CacheLines{.data = coherent_dma.data(), .bytes = 65U}),
           "DmaChain cache-line bind rejects partial lines");
    arc::OwnedDmaChain<std::uint8_t, 2> owned_dma{};
    static_assert(!std::is_move_constructible_v<decltype(owned_dma)>);
    expect(owned_dma.alloc(4U, true).has_value(), "OwnedDmaChain allocates descriptor buffers");
    expect(owned_dma.head()->buffer == owned_dma.buf(0).data() &&
               owned_dma.head()->length == owned_dma.buf(0).bytes() &&
               owned_dma.chain().tail()->next == owned_dma.head(),
           "OwnedDmaChain binds owned circular buffers");
    display.bind(0, std::span(display_bytes).first(4));
    display.bind(1, std::span(display_bytes).subspan(4), true);

    arc::Pipeline<2> pipe{};
    arc::DmaChain<1> empty_endpoint{};
    expect(!pipe.bind(0, arc::endpoint(empty_endpoint)), "Pipeline rejects empty DMA endpoints");
    expect(pipe.bind(0, arc::endpoint(camera)).has_value(), "Pipeline source bind");
    expect(pipe.bind(1, arc::endpoint(display)).has_value(), "Pipeline sink bind");
    expect(pipe.link_linear().has_value(), "Pipeline linear link");
    expect(camera.tail()->next == display.head() && display.tail()->next == nullptr, "Pipeline descriptor chain");

    struct AxiMockNode {
        arc::AxiPort in{};
        arc::AxiPort out{};

        [[nodiscard]] arc::AxiPort input() noexcept
        {
            return in;
        }

        [[nodiscard]] arc::AxiPort output() noexcept
        {
            return out;
        }
    };
    struct AxiMockPolicy {
        std::size_t links{};
        std::size_t arms{};

        [[nodiscard]] esp_err_t link(const std::size_t index, const arc::AxiEdge& edge) noexcept
        {
            links += edge.bytes != 0U && index == links ? 1U : 0U;
            return ESP_OK;
        }

        [[nodiscard]] esp_err_t arm(const std::size_t index, AxiMockNode&) noexcept
        {
            arms += index == arms ? 1U : 0U;
            return ESP_OK;
        }
    };
    arc::DmaChain<1> cam_out{};
    arc::DmaChain<1> aes_in{};
    arc::DmaChain<1> aes_out{};
    arc::DmaChain<1> spi_in{};
    std::array<std::uint8_t, 4> cam_data{};
    std::array<std::uint8_t, 4> aes_rx{};
    std::array<std::uint8_t, 4> aes_tx{};
    std::array<std::uint8_t, 4> spi_data{};
    cam_out.bind(0, std::span(cam_data), true);
    aes_in.bind(0, std::span(aes_rx), true);
    aes_out.bind(0, std::span(aes_tx), true);
    spi_in.bind(0, std::span(spi_data), true);
    AxiMockNode cam_node{.out = {.dma = arc::endpoint(cam_out), .signal = 1U}};
    AxiMockNode aes_node{
        .in = {.dma = arc::endpoint(aes_in), .signal = 2U},
        .out = {.dma = arc::endpoint(aes_out), .signal = 3U},
    };
    AxiMockNode spi_node{.in = {.dma = arc::endpoint(spi_in), .signal = 4U}};
    using AxiMockGraph = arc::AxiGraph<AxiMockNode, AxiMockNode, AxiMockNode>;
    static_assert(AxiMockGraph::nodes == 3U);
    static_assert(AxiMockGraph::edges == 2U);
    const auto axi_plan = AxiMockGraph::plan(cam_node, aes_node, spi_node);
    expect(axi_plan.has_value() && axi_plan->bytes == 8U, "AxiGraph plans adjacent DMA ports");
    AxiMockPolicy axi_policy{};
    expect(AxiMockGraph::boot(axi_policy, cam_node, aes_node, spi_node).has_value() &&
               axi_policy.links == 2U &&
               axi_policy.arms == 3U,
           "AxiGraph boots policy hooks");
    expect(cam_out.tail()->next == aes_in.head() && aes_out.tail()->next == spi_in.head(), "AxiGraph links DMA tails");
    AxiMockNode bad_axi{};
    expect(!AxiMockGraph::plan(bad_axi, aes_node, spi_node), "AxiGraph rejects missing ports");
    const arc::DmaEndpoint huge_ep{
        .head = cam_out.head(),
        .tail = cam_out.tail(),
        .descriptors = 1U,
        .bytes = std::numeric_limits<std::size_t>::max(),
    };
    AxiMockNode huge_a{.out = {.dma = huge_ep}};
    AxiMockNode huge_b{.in = {.dma = huge_ep}, .out = {.dma = huge_ep}};
    AxiMockNode huge_c{.in = {.dma = huge_ep}};
    expect(!AxiMockGraph::plan(huge_a, huge_b, huge_c), "AxiGraph rejects byte accumulation overflow");

    alignas(arc::cache_line) std::array<std::uint8_t, 384> frame{};
    arc::DmaChain<2> rows{};
    const arc::Dma2dWindow unaligned{.x_bytes = 1U, .y = 1U, .width_bytes = 2U, .height = 2U, .stride_bytes = 4U};
    expect(!arc::bind_rows(rows, std::span(frame), unaligned), "Pipeline 2D rejects cache-unsafe rows");
    const arc::Dma2dWindow wrapping{
        .x_bytes = std::numeric_limits<std::size_t>::max() - 63U,
        .y = 0U,
        .width_bytes = 128U,
        .height = 1U,
        .stride_bytes = std::numeric_limits<std::size_t>::max(),
    };
    expect(!arc::valid(wrapping) && !arc::bind_rows(rows, std::span(frame), wrapping),
           "Pipeline 2D rejects wrapped row bounds");
    const arc::Dma2dWindow huge_y{
        .x_bytes = 0U,
        .y = std::numeric_limits<std::size_t>::max(),
        .width_bytes = 64U,
        .height = 2U,
        .stride_bytes = 64U,
    };
    expect(!arc::fits(frame.size(), huge_y) && !arc::bind_rows(rows, std::span(frame), huge_y),
           "Pipeline 2D rejects wrapped row index");
    const arc::Dma2dWindow too_wide{
        .x_bytes = 0U,
        .y = 0U,
        .width_bytes = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1U,
        .height = 1U,
        .stride_bytes = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1U,
    };
    expect(!arc::fits(std::numeric_limits<std::size_t>::max(), too_wide),
           "Pipeline 2D rejects descriptor-wide rows");
    const arc::Dma2dWindow crop{.x_bytes = 64U, .y = 1U, .width_bytes = 64U, .height = 2U, .stride_bytes = 128U};
    expect(!arc::cache_safe(std::span<std::uint8_t>{}, crop), "Pipeline 2D cache safety rejects empty frames");
    expect(arc::bind_rows(rows, std::span(frame), crop).has_value(), "Pipeline 2D row bind");
    expect(rows.desc[0].buffer == frame.data() + 192U && rows.desc[1].buffer == frame.data() + 320U,
           "Pipeline 2D offsets");
    expect(rows.desc[0].length == 64U && rows.desc[1].next == nullptr, "Pipeline 2D lengths");

    const std::array<std::uint8_t, 2> payload{0xaaU, 0x55U};
    std::array<std::uint8_t, 32> packet{};
    const auto rtp = arc::net::Rtp::packet(std::span(packet), std::span(payload), 7U, 0x01020304U, 0xaabbccddU, 97U, true);
    expect(rtp.has_value() && rtp->size() == 14U, "RTP packet size");
    expect(packet[1] == 0xe1U && packet[2] == 0U && packet[3] == 7U && packet[12] == 0xaaU, "RTP packet layout");
    std::array<std::uint8_t, 32> in_place_rtp{0xaaU, 0x55U};
    const auto in_place_rtp_packet =
        arc::net::Rtp::packet(std::span(in_place_rtp), std::span(in_place_rtp).first(2U), 7U, 0x01020304U, 0xaabbccddU);
    expect(in_place_rtp_packet.has_value() && (*in_place_rtp_packet)[12] == 0xaaU && (*in_place_rtp_packet)[13] == 0x55U,
           "RTP packet preserves in-place payload");
    expect(!arc::net::Rtp::packet(
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), packet.size()},
               std::span(payload),
               7U,
               0x01020304U,
               0xaabbccddU),
           "RTP packet rejects null output span");
    expect(!arc::net::Rtp::packet(
               std::span(packet),
               std::span<const std::uint8_t>{
                   payload.data(),
                   std::numeric_limits<std::size_t>::max(),
               },
               7U,
               0x01020304U,
               0xaabbccddU),
           "RTP packet length overflow rejects");

    std::array<std::uint8_t, 96> part{};
    const auto mjpeg = arc::net::Mjpeg::part(std::span(part), std::span(payload), "cam");
    expect(mjpeg.has_value() && mjpeg->size() > payload.size(), "MJPEG part");
    expect(part[0] == '-' && part[2] == 'c' && part[mjpeg->size() - 2U] == '\r', "MJPEG boundary layout");
    std::array<std::uint8_t, 96> in_place_mjpeg{0xaaU, 0x55U};
    const auto in_place_mjpeg_part =
        arc::net::Mjpeg::part(std::span(in_place_mjpeg), std::span(in_place_mjpeg).first(2U), "cam");
    expect(in_place_mjpeg_part.has_value() &&
               (*in_place_mjpeg_part)[in_place_mjpeg_part->size() - 4U] == 0xaaU &&
               (*in_place_mjpeg_part)[in_place_mjpeg_part->size() - 3U] == 0x55U &&
               (*in_place_mjpeg_part)[in_place_mjpeg_part->size() - 2U] == '\r' &&
               (*in_place_mjpeg_part)[in_place_mjpeg_part->size() - 1U] == '\n',
           "MJPEG part preserves in-place payload");
    std::array<std::uint8_t, 96> in_place_mjpeg_boundary{0xaaU, 0x55U, 'c', 'a', 'm'};
    const auto aliased_mjpeg_part = arc::net::Mjpeg::part(
        std::span(in_place_mjpeg_boundary),
        std::span(in_place_mjpeg_boundary).first(2U),
        std::string_view{reinterpret_cast<const char*>(in_place_mjpeg_boundary.data() + 2U), 3U});
    expect(aliased_mjpeg_part.has_value() &&
               (*aliased_mjpeg_part)[2] == 'c' &&
               (*aliased_mjpeg_part)[3] == 'a' &&
               (*aliased_mjpeg_part)[4] == 'm' &&
               (*aliased_mjpeg_part)[aliased_mjpeg_part->size() - 4U] == 0xaaU &&
               (*aliased_mjpeg_part)[aliased_mjpeg_part->size() - 3U] == 0x55U,
           "MJPEG part preserves in-place boundary");
    expect(!arc::net::Mjpeg::part(
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), part.size()},
               std::span(payload),
               "cam"),
           "MJPEG part rejects null output span");
    expect(!arc::net::Mjpeg::part(
               std::span(part),
               std::span(payload),
               std::string_view{static_cast<const char*>(nullptr), 3U}),
           "MJPEG part rejects null boundary view");
    expect(!arc::net::Mjpeg::part(
               std::span(part),
               std::span<const std::uint8_t>{
                   payload.data(),
                   std::numeric_limits<std::size_t>::max(),
               },
               "cam"),
           "MJPEG part length overflow rejects");

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
    expect(!arc::usb::Fifo<decltype(fifo)>::write(
               fifo,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}),
           "USB FIFO rejects null write span");
    expect(!arc::usb::Fifo<decltype(fifo)>::read(
               fifo,
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U}),
           "USB FIFO rejects null read span");

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
    constexpr auto wake_prog = arc::ulp::Builder<4>{}.read_adc(2U).if_greater(100U).wake_main().build();
    static_assert(wake_prog.valid());
    static_assert(wake_prog.used == 3U);
    ulp_policy_woke = false;
    expect(wake_prog.run<UlpPolicy>().has_value() && ulp_policy_woke, "ULP builder wakes from ADC threshold");
    constexpr auto quiet_prog = arc::ulp::Builder<4>{}.read_adc(2U).if_greater(200U).wake_main().build();
    ulp_policy_woke = false;
    expect(quiet_prog.run<UlpPolicy>().has_value() && !ulp_policy_woke, "ULP builder skips failed guard");
    constexpr auto overflow_prog = arc::ulp::Builder<2>{}.read_adc(2U).if_greater(100U).wake_main().build();
    static_assert(!overflow_prog.valid());
    expect(!overflow_prog.run<UlpPolicy>(), "ULP builder reports fixed budget overflow");
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
    const auto null_debayer = arc::isp::Debayer::to_rgb565(
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), raw.size()},
        4U,
        4U,
        std::span(rgb),
        arc::isp::Bayer::rggb);
    expect(!null_debayer && null_debayer.error() == ESP_ERR_INVALID_ARG, "ISP debayer rejects null raw span");

    std::array<std::uint8_t, 1> tiny_raw{};
    std::array<std::uint16_t, 1> tiny_rgb{};
    const auto oversized = arc::isp::Debayer::to_rgb565(
        std::span(tiny_raw),
        (std::numeric_limits<std::size_t>::max() / 2U) + 1U,
        2U,
        std::span(tiny_rgb),
        arc::isp::Bayer::rggb);
    expect(!oversized && oversized.error() == ESP_ERR_INVALID_ARG, "ISP rejects oversized frame");

    const auto stats = arc::isp::AecAwb::measure_rgb565(std::span(rgb));
    const auto null_stats = arc::isp::AecAwb::measure_rgb565(
        std::span<const std::uint16_t>{static_cast<const std::uint16_t*>(nullptr), 1U});
    const auto tuning = arc::isp::AecAwb::tune(stats);
    expect(stats.pixels == rgb.size() && null_stats.pixels == 0U && tuning.red_gain_q8 != 0U, "ISP AEC/AWB tuning");

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
    const auto null_edges_in = arc::vision::Sobel::edges(
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), gray.size()},
        4U,
        4U,
        edges,
        0U);
    const auto null_edges_out = arc::vision::Sobel::edges(
        gray,
        4U,
        4U,
        std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), edges.size()},
        0U);
    expect(!null_edges_in && !null_edges_out, "Vision Sobel rejects null spans");

    std::array<std::uint8_t, 4> tiny_gray{};
    std::array<std::uint8_t, 4> tiny_edges{};
    const auto huge_width = (std::numeric_limits<std::size_t>::max() / 3U) + 1U;
    const auto oversized_edges = arc::vision::Sobel::edges(
        std::span(tiny_gray),
        huge_width,
        3U,
        std::span(tiny_edges),
        0U);
    expect(!oversized_edges && oversized_edges.error() == ESP_ERR_INVALID_ARG, "Vision Sobel rejects oversized frame");

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
    const auto null_flow_previous = arc::vision::OpticalFlow::lucas_kanade(
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), gray.size()},
        shifted,
        4U,
        4U);
    const auto null_flow_current = arc::vision::OpticalFlow::lucas_kanade(
        gray,
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), shifted.size()},
        4U,
        4U);
    expect(!null_flow_previous && !null_flow_current, "Vision optical flow rejects null spans");
    static std::array<std::uint8_t, 32U * 32U> rich_texture{};
    for (std::size_t y = 0U; y < 32U; ++y) {
        for (std::size_t x = 0U; x < 32U; ++x) {
            rich_texture[(y * 32U) + x] = static_cast<std::uint8_t>((x * 17U) + (y * 31U) + (x * y));
        }
    }
    const auto rich_flow = arc::vision::OpticalFlow::lucas_kanade(
        std::span<const std::uint8_t>{rich_texture},
        std::span<const std::uint8_t>{rich_texture},
        32U,
        32U);
    expect(rich_flow.has_value() && rich_flow->dx_q8 == 0 && rich_flow->dy_q8 == 0 &&
               rich_flow->confidence == std::numeric_limits<std::uint32_t>::max(),
           "Vision optical flow saturates large determinant confidence");
    const auto oversized_flow =
        arc::vision::OpticalFlow::lucas_kanade(std::span(tiny_gray), std::span(tiny_edges), huge_width, 3U);
    expect(!oversized_flow && oversized_flow.error() == ESP_ERR_INVALID_ARG,
           "Vision optical flow rejects oversized frame");
    const auto target = arc::vision::VisualServo::flow_target({.dx_q8 = 256, .confidence = 1U}, 0.25F, 12.0F);
    expect(target.q == -0.25F && target.bus == 12.0F, "Vision flow to FOC target");

    const auto yuv420_size = arc::vision::frame_bytes(4U, 4U, arc::vision::Pixel::yuv420);
    expect(yuv420_size.has_value() && *yuv420_size == 24U &&
               !arc::vision::frame_bytes(3U, 4U, arc::vision::Pixel::yuv420),
           "Vision accelerator sizes YUV420 frames");

    std::array<std::uint8_t, 32> rgb565_in_bytes{};
    std::array<std::uint8_t, 8> rgb565_out_bytes{};
    const arc::vision::InFrame rgb565_in{
        .data = std::span<const std::uint8_t>{rgb565_in_bytes},
        .width = 4U,
        .height = 4U,
        .format = arc::vision::Pixel::rgb565,
    };
    const arc::vision::OutFrame rgb565_out{
        .data = std::span<std::uint8_t>{rgb565_out_bytes},
        .width = 2U,
        .height = 2U,
        .format = arc::vision::Pixel::rgb565,
    };

    struct VisionAccelPolicy {
        int srm_calls{};
        int fill_calls{};
        int blend_calls{};
        int jpeg_calls{};
        int h264_calls{};

        [[nodiscard]] esp_err_t srm(const arc::vision::PpaSrm& plan) noexcept
        {
            ++srm_calls;
            return plan.source.w == 2U ? ESP_OK : ESP_FAIL;
        }

        [[nodiscard]] arc::Status fill(const arc::vision::PpaFill&) noexcept
        {
            ++fill_calls;
            return arc::ok();
        }

        [[nodiscard]] esp_err_t blend(const arc::vision::PpaBlend&) noexcept
        {
            ++blend_calls;
            return ESP_OK;
        }

        [[nodiscard]] arc::Status jpeg(const arc::vision::JpegEncode&) noexcept
        {
            ++jpeg_calls;
            return arc::ok();
        }

        [[nodiscard]] esp_err_t h264(const arc::vision::H264Encode&) noexcept
        {
            ++h264_calls;
            return ESP_OK;
        }
    };

    VisionAccelPolicy vision_policy{};
    const arc::vision::PpaSrm srm{
        .in = rgb565_in,
        .source = {.x = 1U, .y = 1U, .w = 2U, .h = 2U},
        .out = rgb565_out,
        .target = arc::vision::full_frame(2U, 2U),
        .sx_q8 = 128U,
        .sy_q8 = 128U,
    };
    expect(srm.validate().has_value() && srm.submit(vision_policy).has_value() && vision_policy.srm_calls == 1,
           "Vision accelerator validates and submits PPA SRM");
    auto bad_srm = srm;
    bad_srm.source = {.x = 3U, .y = 3U, .w = 2U, .h = 2U};
    expect(!bad_srm.validate(), "Vision accelerator rejects out-of-frame PPA source");

    const arc::vision::PpaFill fill{
        .out = rgb565_out,
        .target = arc::vision::full_frame(2U, 2U),
        .argb8888 = 0xff00ff00U,
    };
    expect(fill.submit(vision_policy).has_value() && vision_policy.fill_calls == 1,
           "Vision accelerator submits PPA fill");

    std::array<std::uint8_t, 32> blend_out_bytes{};
    const arc::vision::OutFrame blend_out{
        .data = std::span<std::uint8_t>{blend_out_bytes},
        .width = 4U,
        .height = 4U,
        .format = arc::vision::Pixel::rgb565,
    };
    const arc::vision::PpaBlend blend{
        .bg = rgb565_in,
        .fg = rgb565_in,
        .out = blend_out,
        .target = arc::vision::full_frame(4U, 4U),
        .alpha_q8 = 128U,
    };
    expect(blend.submit(vision_policy).has_value() && vision_policy.blend_calls == 1,
           "Vision accelerator submits PPA blend");

    std::array<std::uint8_t, 64> jpeg_bytes{};
    const auto jpeg = arc::vision::JpegEncoder<4U, 4U, 90U>::frame(rgb565_in, std::span(jpeg_bytes));
    expect(jpeg.validate().has_value() &&
               arc::vision::JpegEncoder<4U, 4U, 90U>::encode(vision_policy, rgb565_in, std::span(jpeg_bytes))
                   .has_value() &&
               vision_policy.jpeg_calls == 1,
           "Vision accelerator validates JPEG encode plans");
    const arc::vision::JpegEncode bad_jpeg{
        .raw = rgb565_in,
        .out = std::span<std::uint8_t>{jpeg_bytes},
        .quality = 0U,
    };
    expect(!bad_jpeg.validate(), "Vision accelerator rejects invalid JPEG quality");

    std::array<std::uint8_t, 24> yuv_bytes{};
    std::array<std::uint8_t, 80> h264_bytes{};
    const arc::vision::InFrame yuv_frame{
        .data = std::span<const std::uint8_t>{yuv_bytes},
        .width = 4U,
        .height = 4U,
        .format = arc::vision::Pixel::yuv420,
    };
    const auto h264 = arc::vision::H264Encoder<4U, 4U, 400'000U>::frame(yuv_frame, std::span(h264_bytes));
    expect(h264.validate().has_value() &&
               arc::vision::H264Encoder<4U, 4U, 400'000U>::encode(vision_policy, yuv_frame, std::span(h264_bytes))
                   .has_value() &&
               vision_policy.h264_calls == 1,
           "Vision accelerator validates H264 encode plans");
    const arc::vision::H264Encode bad_h264{
        .raw = rgb565_in,
        .out = std::span<std::uint8_t>{h264_bytes},
        .bitrate = 400'000U,
    };
    expect(!bad_h264.validate(), "Vision accelerator rejects non-YUV H264 input");
    struct EmptyVisionPolicy {};
    EmptyVisionPolicy empty_vision_policy{};
    const auto unsupported_srm = srm.submit(empty_vision_policy);
    expect(!unsupported_srm && unsupported_srm.error() == ESP_ERR_NOT_SUPPORTED,
           "Vision accelerator reports unsupported policy hooks");
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
    expect(!arc::vm::BPF<>::decode(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), sizeof(Insn)},
               std::span(decoded)),
           "BPF decode rejects null bytecode span");

    std::array<std::uint8_t, 8> memory{};
    arc::vm::BPF<> vm{};
    const std::array<std::int64_t, 2> args{7, 0};
    const auto result = vm.run(*bytecode, std::span(memory), std::span<const std::int64_t>{args});
    std::uint32_t stored{};
    std::memcpy(&stored, memory.data(), sizeof(stored));
    expect(result.has_value() && result->value == 12 && stored == 12U, "BPF VM executes and stores");
    expect(!vm.run(*bytecode, std::span(memory), std::span<const std::int64_t>{args}, {.writable_memory = false}),
           "BPF VM rejects store into read-only memory");
    const std::array add_wrap_program{
        Insn::make(Op::mov_reg, 0U, 1U),
        Insn::make(Op::add_imm, 0U, 0U, 0, 1),
        Insn::make(Op::exit),
    };
    const std::array max_arg{std::numeric_limits<std::int64_t>::max()};
    const auto add_wrap = vm.run(std::span<const Insn>{add_wrap_program}, {}, std::span<const std::int64_t>{max_arg});
    expect(add_wrap.has_value() && add_wrap->value == std::numeric_limits<std::int64_t>::min(),
           "BPF VM add uses defined 64-bit wrap");
    const std::array sub_wrap_program{
        Insn::make(Op::mov_reg, 0U, 1U),
        Insn::make(Op::sub_imm, 0U, 0U, 0, 1),
        Insn::make(Op::exit),
    };
    const std::array min_arg{std::numeric_limits<std::int64_t>::min()};
    const auto sub_wrap = vm.run(std::span<const Insn>{sub_wrap_program}, {}, std::span<const std::int64_t>{min_arg});
    expect(sub_wrap.has_value() && sub_wrap->value == std::numeric_limits<std::int64_t>::max(),
           "BPF VM subtract uses defined 64-bit wrap");
    const std::array mul_wrap_program{
        Insn::make(Op::mov_reg, 0U, 1U),
        Insn::make(Op::mul_imm, 0U, 0U, 0, 4),
        Insn::make(Op::exit),
    };
    const std::array mul_arg{std::int64_t{1} << 62U};
    const auto mul_wrap = vm.run(std::span<const Insn>{mul_wrap_program}, {}, std::span<const std::int64_t>{mul_arg});
    expect(mul_wrap.has_value() && mul_wrap->value == 0, "BPF VM multiply uses defined 64-bit wrap");
    const std::array lsh_program{
        Insn::make(Op::mov_reg, 0U, 1U),
        Insn::make(Op::lsh_imm, 0U, 0U, 0, 1),
        Insn::make(Op::exit),
    };
    const std::array negative_arg{std::int64_t{-1}};
    const auto lsh = vm.run(std::span<const Insn>{lsh_program}, {}, std::span<const std::int64_t>{negative_arg});
    expect(lsh.has_value() && lsh->value == -2, "BPF VM left shift uses defined bit wrap");
    const std::array div_overflow_program{
        Insn::make(Op::mov_reg, 0U, 1U),
        Insn::make(Op::div_imm, 0U, 0U, 0, -1),
        Insn::make(Op::exit),
    };
    expect(!vm.run(std::span<const Insn>{div_overflow_program}, {}, std::span<const std::int64_t>{min_arg}),
           "BPF VM rejects signed division overflow");
    const std::array address_overflow_program{
        Insn::make(Op::load32, 0U, 1U, 1, 0),
        Insn::make(Op::exit),
    };
    expect(!vm.run(std::span<const Insn>{address_overflow_program}, std::span(memory), std::span<const std::int64_t>{max_arg}),
           "BPF VM rejects checked address overflow");
    expect(
        !vm.run(
            std::span<const Insn>{static_cast<const Insn*>(nullptr), 1U},
            std::span(memory),
            std::span<const std::int64_t>{args}),
        "BPF VM rejects null program span");
    expect(
        !vm.run(
            *bytecode,
            std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 8U},
            std::span<const std::int64_t>{args}),
        "BPF VM rejects null memory span");
    expect(
        !vm.run(
            *bytecode,
            std::span(memory),
            std::span<const std::int64_t>{static_cast<const std::int64_t*>(nullptr), 1U}),
        "BPF VM rejects null args span");
    expect(!vm.run(*bytecode, std::span(memory), std::span<const std::int64_t>{args}, {.max_steps = 0U}),
           "BPF VM rejects zero step limit");

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
    expect(arc::ulp::ml::requantize(3, {.shift = 1}) == 2 &&
               arc::ulp::ml::requantize(-3, {.shift = 1}) == -2,
           "ULP ML requantize rounds symmetrically");
    expect(arc::ulp::ml::requantize(std::numeric_limits<std::int64_t>::max(), {.multiplier = 2}) == 127 &&
               arc::ulp::ml::requantize(std::numeric_limits<std::int64_t>::min(), {.multiplier = 2}) == -128,
           "ULP ML requantize saturates overflow paths");

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
    const auto null_aggregate = arc::crypto::Paillier<ToyInt>::aggregate(
        key,
        std::span<const ToyInt>{static_cast<const ToyInt*>(nullptr), 1U});
    expect(!null_aggregate && null_aggregate.error() == ESP_ERR_INVALID_ARG, "Paillier aggregate rejects null span");

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
    expect(!arc::covert::Fsk::plan(
                std::span<arc::covert::FskSymbol, 8>{static_cast<arc::covert::FskSymbol*>(nullptr), symbols.size()},
                bits,
                fsk)
                   .has_value() &&
               !arc::covert::Fsk::plan(
                    std::span<arc::covert::FskSymbol, 8>{symbols},
                    std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), bits.size()},
                    fsk)
                    .has_value(),
           "Covert FSK rejects null spans");

    expect(arc::covert::EmTx<PwmCarrier>::bit(true, fsk).has_value(), "Covert EM PWM mark");
    expect(covert_hz == fsk.mark_hz && covert_duty == fsk.duty_permille, "Covert EM PWM retune");
    expect(arc::covert::EmTx<SigmaCarrier>::bit(false, fsk).has_value(), "Covert EM Sigma space");
    expect(covert_density == fsk.space_density, "Covert Sigma density");
    expect(arc::covert::SonicTx<PwmCarrier>::bit(false, fsk).has_value(), "Covert sonic PWM space");
    expect(covert_hz == fsk.space_hz &&
               !arc::covert::EmTx<PwmCarrier>::symbols(
                    std::span<const arc::covert::FskSymbol>{static_cast<const arc::covert::FskSymbol*>(nullptr), 1U})
                    .has_value(),
           "Covert sonic frequency");
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
    expect(!Kem::ntt(std::span<std::int16_t, Kem::n>{static_cast<std::int16_t*>(nullptr), Kem::n}),
           "Kyber NTT rejects null polynomial span");

    Kem::Poly product{};
    expect(Kem::pointwise(
               std::span<std::int16_t, Kem::n>{product},
               std::span<const std::int16_t, Kem::n>{original},
               std::span<const std::int16_t, Kem::n>{original})
               .has_value(),
           "Kyber SIMD pointwise product");
    expect(product[5] == Kem::mul(5, 5), "Kyber pointwise coefficient");
    expect(!Kem::pointwise(
               std::span<std::int16_t, Kem::n>{product},
               std::span<const std::int16_t, Kem::n>{static_cast<const std::int16_t*>(nullptr), Kem::n},
               std::span<const std::int16_t, Kem::n>{original}),
           "Kyber pointwise rejects null input span");

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
    expect(!Kem::keypair(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), seed.size()},
               public_key,
               secret_key),
           "Kyber keypair rejects null seed span");
    expect(!Kem::encapsulate(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), public_key.size()},
               coins,
               ciphertext,
               shared),
           "Kyber encapsulate rejects null public key span");
    expect(!Kem::decapsulate(
               secret_key,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), ciphertext.size()},
               opened),
           "Kyber decapsulate rejects null ciphertext span");

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
    expect(!arc::sdr::PulseSynth::fm(
               std::span<std::uint16_t>{static_cast<std::uint16_t*>(nullptr), audio.size()},
               audio,
               sdr_cfg),
           "SDR FM rejects null output span");
    const std::array<std::uint8_t, 1> ook_bits{0b1010'0000U};
    const auto ook = arc::sdr::PulseSynth::ook(rf_words, ook_bits, sdr_cfg);
    expect(ook.has_value() && rf_words[0] == sdr_cfg.one_word && rf_words[1] == 0U, "SDR OOK pulse synthesis");
    expect(!arc::sdr::PulseSynth::ook(
               rf_words,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
               sdr_cfg),
           "SDR OOK rejects null input span");
    constexpr arc::sdr::TxConfig wide_sdr_cfg{
        .sample_hz = 4'000'000'000U,
        .carrier_hz = 1'900'000'000U,
        .gpio = 4U,
        .one_word = 0x10U,
        .zero_word = 0x20U,
    };
    std::array<std::uint16_t, 1> wide_rf{};
    const std::array<std::uint16_t, 1> low_envelope{100U};
    const auto wide_am = arc::sdr::PulseSynth::am(wide_rf, low_envelope, wide_sdr_cfg);
    expect(wide_am.has_value() && wide_rf[0] == wide_sdr_cfg.zero_word, "SDR AM uses wide phase scaling");
    expect(!arc::sdr::PulseSynth::am(
               wide_rf,
               std::span<const std::uint16_t>{static_cast<const std::uint16_t*>(nullptr), 1U},
               wide_sdr_cfg),
           "SDR AM rejects null envelope span");
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
    expect(!arc::sdr::Tx<SdrPolicy>::start(
               sdr_cfg,
               std::span<const std::uint16_t>{static_cast<const std::uint16_t*>(nullptr), 1U}),
           "SDR LCD_CAM policy rejects null words span");

    std::array<std::int16_t, 64> chirp{};
    expect(arc::swarm::AcousticSlam::fmcw_chirp(chirp).has_value() && chirp[1] != 0, "Acoustic FMCW chirp");
    constexpr arc::swarm::ChirpConfig wrapped_chirp{
        .start_hz = 0x8000'0000U,
        .stop_hz = 1U,
        .sample_hz = 96'000U,
        .amplitude = 1,
    };
    static_assert(!arc::swarm::AcousticSlam::chirp_valid(wrapped_chirp));
    expect(!arc::swarm::AcousticSlam::fmcw_chirp(
               std::span<std::int16_t>{static_cast<std::int16_t*>(nullptr), 1U}),
           "Acoustic chirp rejects null output span");
    const std::array<std::int16_t, 6> ref{1, 2, 3, 4, 0, 0};
    const std::array<std::int16_t, 6> delayed{0, 1, 2, 3, 4, 0};
    const auto tdoa = arc::swarm::AcousticSlam::tdoa(ref, delayed, 1'000U, 3U);
    expect(tdoa.has_value() && tdoa->lag_samples == 1 && near(tdoa->delta_mm, 343.0F, 0.1F), "Acoustic TDoA");
    expect(!arc::swarm::AcousticSlam::tdoa(
               std::span<const std::int16_t>{static_cast<const std::int16_t*>(nullptr), 1U},
               delayed,
               1'000U,
               3U),
           "Acoustic TDoA rejects null reference span");
    expect(!arc::swarm::AcousticSlam::tdoa(
               ref,
               delayed,
               1'000U,
               3U,
               std::numeric_limits<float>::quiet_NaN()),
           "Acoustic TDoA rejects non-finite sound speed");
    expect(!arc::swarm::AcousticSlam::tdoa(
               ref,
               delayed,
               1'000U,
               static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())),
           "Acoustic TDoA rejects oversized lag windows");
    std::array<arc::simd::ComplexF32, 8> fft_ref{};
    std::array<arc::simd::ComplexF32, 8> fft_delayed{};
    std::array<arc::simd::ComplexF32, 8> fft_scratch{};
    fft_ref[1].re = 1.0F;
    fft_delayed[2].re = 1.0F;
    const auto phat = arc::swarm::AcousticSlam::gcc_tdoa(fft_ref, fft_delayed, fft_scratch, 3U, 1'000.0F);
    expect(phat.has_value(), "Acoustic SIMD FFT GCC-PHAT");
    expect(!arc::swarm::AcousticSlam::gcc_tdoa(
               fft_ref,
               fft_delayed,
               fft_scratch,
               3U,
               std::numeric_limits<float>::infinity()),
           "Acoustic GCC-PHAT rejects non-finite sample rate");

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
    auto invalid_ranges = ranges;
    invalid_ranges[0] = std::numeric_limits<float>::quiet_NaN();
    expect(!arc::swarm::AcousticSlam::solve(
               std::span<const arc::swarm::AcousticAnchor, 4>{anchors},
               std::span<const float, 4>{invalid_ranges}),
           "Acoustic trilateration rejects non-finite ranges");
    const std::array<arc::swarm::AcousticAnchor, 4> far_anchors{
        arc::swarm::AcousticAnchor{.node_id = 1U, .position = {.x_mm = 1'000'000.0F, .y_mm = 1'000'000.0F, .z_mm = 1'000'000.0F}},
        arc::swarm::AcousticAnchor{.node_id = 2U, .position = {.x_mm = 1'005'000.0F, .y_mm = 1'000'000.0F, .z_mm = 1'000'000.0F}},
        arc::swarm::AcousticAnchor{.node_id = 3U, .position = {.x_mm = 1'000'000.0F, .y_mm = 1'005'000.0F, .z_mm = 1'000'000.0F}},
        arc::swarm::AcousticAnchor{.node_id = 4U, .position = {.x_mm = 1'000'000.0F, .y_mm = 1'000'000.0F, .z_mm = 1'005'000.0F}},
    };
    const arc::swarm::Position3 far_target{.x_mm = 1'001'234.5F, .y_mm = 1'002'345.25F, .z_mm = 1'003'456.75F};
    std::array<float, 4> far_ranges{};
    for (std::size_t i = 0U; i < far_anchors.size(); ++i) {
        const auto dx = static_cast<double>(far_target.x_mm) - static_cast<double>(far_anchors[i].position.x_mm);
        const auto dy = static_cast<double>(far_target.y_mm) - static_cast<double>(far_anchors[i].position.y_mm);
        const auto dz = static_cast<double>(far_target.z_mm) - static_cast<double>(far_anchors[i].position.z_mm);
        far_ranges[i] = static_cast<float>(std::sqrt((dx * dx) + (dy * dy) + (dz * dz)));
    }
    const auto far_solved = arc::swarm::AcousticSlam::solve(
        std::span<const arc::swarm::AcousticAnchor, 4>{far_anchors},
        std::span<const float, 4>{far_ranges});
    expect(far_solved.has_value() &&
               near(far_solved->x_mm, far_target.x_mm, 0.25F) &&
               near(far_solved->y_mm, far_target.y_mm, 0.25F) &&
               near(far_solved->z_mm, far_target.z_mm, 0.25F),
           "Acoustic trilateration keeps precision with large coordinates");
    arc::dsp::Kalman<float, 6, 3> slam_filter{};
    expect(arc::swarm::AcousticSlam::correct_filter(slam_filter, *solved).has_value(), "Acoustic Kalman correction");
    expect(!arc::swarm::AcousticSlam::correct_filter(
               slam_filter,
               *solved,
               std::numeric_limits<float>::quiet_NaN()),
           "Acoustic Kalman correction rejects invalid noise");

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
    expect(!arc::vm::Hypervisor::apply<HyperPolicy>({
               .code = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
               .ram = guest_ram,
           }),
           "Hypervisor rejects null code span");
    expect(!arc::vm::Hypervisor::apply<HyperPolicy>({
               .code = guest_code,
               .ram = std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U},
           }),
           "Hypervisor rejects null RAM span");
    expect(!arc::vm::Hypervisor::apply<HyperPolicy>({
               .code = guest_code,
               .ram = guest_ram,
               .trusted_peripherals = std::span<const std::uint32_t>{static_cast<const std::uint32_t*>(nullptr), 1U},
           }),
           "Hypervisor rejects null trusted peripheral span");
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
    expect(!Intermittent::save(
               {.pc = 0x1234U, .sp = 0x5678U},
               std::span<const std::byte>{static_cast<const std::byte*>(nullptr), 1U},
               rcu_state),
           "Intermittent rejects null stack span");
    expect(!Intermittent::save(
               {.pc = 0x1234U, .sp = 0x5678U},
               stack,
               std::span<const std::byte>{static_cast<const std::byte*>(nullptr), 1U}),
           "Intermittent rejects null RCU span");
    const std::span<const std::byte> huge_stack{
        static_cast<const std::byte*>(nullptr),
        std::numeric_limits<std::size_t>::max(),
    };
    expect(!Intermittent::save({.pc = 0x1234U, .sp = 0x5678U}, huge_stack, rcu_state),
           "Intermittent rejects checkpoint byte overflow");
    expect(Intermittent::resume<IntermittentPolicy>().has_value() && intermittent_restored,
           "Intermittent restore hook");

    struct FramPolicy {
        static esp_err_t write(
            const std::size_t offset,
            const std::span<const std::byte> bytes) noexcept
        {
            if (!bytes.empty() && bytes.data() == nullptr) {
                return ESP_ERR_INVALID_ARG;
            }
            if (offset > fram_storage.size() || bytes.size() > fram_storage.size() - offset) {
                return ESP_ERR_NO_MEM;
            }
            for (std::size_t i = 0U; i < bytes.size(); ++i) {
                fram_storage[offset + i] = bytes[i];
            }
            ++fram_writes;
            return ESP_OK;
        }

        static esp_err_t read(
            const std::size_t offset,
            const std::span<std::byte> bytes) noexcept
        {
            if (!bytes.empty() && bytes.data() == nullptr) {
                return ESP_ERR_INVALID_ARG;
            }
            if (offset > fram_storage.size() || bytes.size() > fram_storage.size() - offset) {
                return ESP_ERR_NO_MEM;
            }
            for (std::size_t i = 0U; i < bytes.size(); ++i) {
                bytes[i] = fram_storage[offset + i];
            }
            ++fram_reads;
            return ESP_OK;
        }
    };

    struct FramState {
        std::uint32_t boot{};
        std::int32_t trim{};
    };

    fram_storage.fill(std::byte{});
    fram_reads = 0U;
    fram_writes = 0U;
    arc::FramAlloc<32, 8> fram{};
    const auto fram_state = fram.make<FramState>();
    const auto fram_counter = fram.make<std::uint16_t>();
    expect(fram_state.has_value() &&
               fram_counter.has_value() &&
               fram_state->span.offset == 0U &&
               fram_state->span.bytes == sizeof(FramState) &&
               fram_counter->span.offset == 8U &&
               fram.used == 10U,
           "FRAM allocator carves aligned persistent slots");
    const FramState stored{.boot = 42U, .trim = -7};
    const auto bad_fram_ref = arc::FramRef<std::uint16_t>{.span = fram_state->span};
    const auto range_fram_ref = arc::FramRef<FramState>{.span = {.offset = 60U, .bytes = sizeof(FramState)}};
    const auto loaded_fram = fram_state->store<FramPolicy>(stored).has_value()
        ? fram_state->load<FramPolicy>()
        : arc::Result<FramState>{arc::fail(ESP_FAIL)};
    expect(loaded_fram.has_value() &&
               loaded_fram->boot == 42U &&
               loaded_fram->trim == -7 &&
               fram_writes == 1U &&
               fram_reads == 1U,
           "FRAM ref stores and loads typed state through policy");
    expect(!bad_fram_ref.load<FramPolicy>(), "FRAM ref rejects mismatched typed span");
    expect(!range_fram_ref.store<FramPolicy>(stored), "FRAM ref propagates policy range failures");
    expect(!fram.alloc(1U, 3U), "FRAM allocator rejects non-power alignment");
    expect(!fram.alloc(64U, 8U), "FRAM allocator rejects exhausted storage");

    const std::array entropy_sram{std::byte{0xA5}, std::byte{0x3C}, std::byte{0x5A}};
    std::array<std::uint8_t, 4> raw_entropy{};
    expect(arc::crypto::Puf::sample_sram(entropy_sram, raw_entropy).has_value() &&
               raw_entropy[0] == 0xA5U && raw_entropy[2] == 0x5AU,
           "PUF SRAM decay sample");
    expect(!arc::crypto::Puf::sample_sram(
               std::span<const std::byte>{static_cast<const std::byte*>(nullptr), 1U},
               raw_entropy),
           "PUF SRAM rejects null source span");
    expect(!arc::crypto::Puf::sample_sram(
               entropy_sram,
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), entropy_sram.size()}),
           "PUF SRAM rejects null output span");
    const std::array<std::uint8_t, 1> raw_pairs{0b0001'1011U};
    std::array<std::uint8_t, 1> stable_bits{};
    const auto puf_stats = arc::crypto::Puf::von_neumann(raw_pairs, stable_bits);
    expect(puf_stats.raw_bits == 8U && puf_stats.stable_bits == 2U && puf_stats.ones == 1U && stable_bits[0] == 0b10U,
           "PUF von Neumann extractor");
    const auto null_puf_stats = arc::crypto::Puf::von_neumann(
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
        stable_bits);
    expect(null_puf_stats.raw_bits == 0U && null_puf_stats.stable_bits == 0U,
           "PUF von Neumann rejects null raw span");
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
    expect(!arc::crypto::Puf::sample_adc<PufAdc>(
               std::span<std::uint16_t>{static_cast<std::uint16_t*>(nullptr), 1U},
               puf_filter),
           "PUF ADC rejects null output span");
    struct PufAdcMin {
        static arc::Result<std::int32_t> read() noexcept
        {
            return std::numeric_limits<std::int32_t>::min();
        }
    };
    arc::dsp::Biquad<std::int32_t>::State puf_min_filter{};
    std::array<std::uint16_t, 1> adc_min_noise{};
    expect(arc::crypto::Puf::sample_adc<PufAdcMin>(adc_min_noise, puf_min_filter).has_value() &&
               adc_min_noise[0] == std::numeric_limits<std::uint16_t>::max(),
           "PUF ADC magnitude saturates signed minimum");
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
    expect(!arc::crypto::Puf::derive_with<PufHash>(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}),
           "PUF key derivation rejects null stable span");

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
    expect(!arc::matrix::FlexRoute::failed(
               {.expected_hz = std::numeric_limits<std::uint32_t>::max() - 5U,
                .measured_hz = std::numeric_limits<std::uint32_t>::max(),
                .tolerance_hz = 10U}),
           "FlexRoute high threshold saturates on overflow");
    expect(arc::matrix::FlexRoute::failed(
               {.expected_hz = 100U, .measured_hz = 88U, .tolerance_hz = 10U}),
           "FlexRoute low threshold still fails outside tolerance");

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
    const arc::dsp::WavefrontPlan<2> tiny_wave_plan{};
    std::array<std::int16_t, 2> tiny_wave_samples{};
    const auto overflowing_wave = arc::dsp::Wavefront::synthesize<2>(
        tiny_wave_plan,
        tiny_wave_samples,
        (std::numeric_limits<std::size_t>::max() / 2U) + 1U);
    expect(!overflowing_wave.has_value() && overflowing_wave.error() == ESP_ERR_INVALID_ARG,
           "Wavefront rejects interleaved sample-count overflow");
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
    expect(!arc::vision::StarTracker::threshold(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), stars_frame.size()},
               stars_mask,
               200U),
           "StarTracker rejects null threshold input span");
    expect(!arc::vision::StarTracker::threshold(
               stars_frame,
               std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), stars_mask.size()},
               200U),
           "StarTracker rejects null threshold output span");
    std::array<arc::vision::StarPoint, 4> stars{};
    const auto star_count = arc::vision::StarTracker::centroids(stars_frame, 5U, 5U, 200U, stars);
    expect(star_count.has_value() && *star_count == 3U && stars[0].x_q4 == 16U && stars[2].y_q4 == 48U,
           "StarTracker sub-pixel centroids");
    expect(!arc::vision::StarTracker::centroids(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), stars_frame.size()},
               5U,
               5U,
               200U,
               stars),
           "StarTracker rejects null centroid input span");
    expect(!arc::vision::StarTracker::centroids(
               stars_frame,
               5U,
               5U,
               200U,
               std::span<arc::vision::StarPoint>{static_cast<arc::vision::StarPoint*>(nullptr), stars.size()}),
           "StarTracker rejects null centroid output span");
    const auto too_wide_stars = arc::vision::StarTracker::centroids(
        stars_frame,
        (std::numeric_limits<std::uint16_t>::max() / 16U) + 2U,
        5U,
        200U,
        stars);
    expect(!too_wide_stars && too_wide_stars.error() == ESP_ERR_INVALID_ARG,
           "StarTracker rejects q4 coordinate overflow");
    std::array<std::uint8_t, 4> tiny_stars_frame{};
    const auto oversized_stars = arc::vision::StarTracker::centroids(
        std::span(tiny_stars_frame),
        (std::numeric_limits<std::size_t>::max() / 3U) + 1U,
        3U,
        200U,
        stars);
    expect(!oversized_stars && oversized_stars.error() == ESP_ERR_INVALID_ARG,
           "StarTracker rejects oversized frame");
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
    expect(!arc::vision::StarTracker::match(
               std::span<const arc::vision::StarPoint>{static_cast<const arc::vision::StarPoint*>(nullptr), 3U},
               std::span<const arc::vision::StarTriangle>{catalog}),
           "StarTracker rejects null observed span");
    expect(!arc::vision::StarTracker::match(
               std::span<const arc::vision::StarPoint>{stars.data(), *star_count},
               std::span<const arc::vision::StarTriangle>{static_cast<const arc::vision::StarTriangle*>(nullptr), 1U}),
           "StarTracker rejects null catalog span");
}

void test_current_goal_surfaces()
{
    using HostBurst = arc::Burst<1, 1'000'000>;
    std::array<rmt_symbol_word_t, 2> burst_symbols{
        HostBurst::symbol(1U, true, 1U, false),
        HostBurst::symbol(2U, false, 2U, true),
    };
    arc_host_rmt_transmit_calls = 0;
    arc_host_rmt_last_bytes = 0U;
    expect(HostBurst::send(nullptr, 1U) == ESP_ERR_INVALID_ARG && arc_host_rmt_transmit_calls == 0,
           "Burst send rejects null symbols before transmit");
    expect(HostBurst::send(burst_symbols.data(), 0U) == ESP_ERR_INVALID_ARG && arc_host_rmt_transmit_calls == 0,
           "Burst send rejects empty symbol spans before transmit");
    expect(HostBurst::send(
               burst_symbols.data(),
               (std::numeric_limits<std::size_t>::max() / sizeof(rmt_symbol_word_t)) + 1U) == ESP_ERR_INVALID_ARG &&
               arc_host_rmt_transmit_calls == 0,
           "Burst send rejects byte-size overflow before transmit");
    expect(HostBurst::send(burst_symbols) == ESP_OK && arc_host_rmt_transmit_calls == 1 &&
               arc_host_rmt_last_bytes == burst_symbols.size() * sizeof(rmt_symbol_word_t),
           "Burst send forwards checked symbol bytes");

    using HostSpiBus = arc::SpiBus<SPI2_HOST, 1, 2, 3, 16>;
    using HostSpi = arc::Spi<HostSpiBus, -1, 1'000'000U, 0, 1>;
    static_assert(HostSpi::bytes_fit(16U));
    static_assert(!HostSpi::bytes_fit(17U));
    static_assert(!HostSpi::bytes_fit((std::numeric_limits<std::size_t>::max() / 8U) + 1U));
    std::array<std::uint8_t, 4> spi_tx{};
    auto spi_job = HostSpi::job(spi_tx.data(), nullptr, spi_tx.size());
    expect(spi_job.length == 32U && spi_job.rxlength == 0U && spi_job.tx_buffer == spi_tx.data(),
           "SPI master job uses checked bit length");
    auto spi_oversize_job = HostSpi::job(spi_tx.data(), nullptr, 17U);
    expect(spi_oversize_job.length == 0U && spi_oversize_job.rxlength == 0U,
           "SPI master job rejects oversize bit length");
    arc_host_spi_transmit_calls = 0U;
    expect(HostSpi::send(spi_tx.data(), 17U) == ESP_ERR_INVALID_ARG && arc_host_spi_transmit_calls == 0U,
           "SPI master send rejects oversize before driver call");
    arc_host_spi_queue_calls = 0U;
    expect(HostSpi::queue(spi_oversize_job, 0U) == ESP_ERR_INVALID_ARG && arc_host_spi_queue_calls == 0U,
           "SPI master queue rejects invalid transaction before driver call");

    using HostSpiSlave = arc::SpiSlave<SPI3_HOST, 5, 6, 7, 8, 2, 0, 16>;
    static_assert(HostSpiSlave::bytes_fit(16U));
    static_assert(!HostSpiSlave::bytes_fit(17U));
    static_assert(!HostSpiSlave::bytes_fit((std::numeric_limits<std::size_t>::max() / 8U) + 1U));
    auto spi_slave_job = HostSpiSlave::job(spi_tx.data(), nullptr, spi_tx.size());
    expect(spi_slave_job.length == 32U && spi_slave_job.tx_buffer == spi_tx.data(),
           "SPI slave job uses checked bit length");
    auto spi_slave_oversize_job = HostSpiSlave::job(spi_tx.data(), nullptr, 17U);
    expect(spi_slave_oversize_job.length == 0U, "SPI slave job rejects oversize bit length");
    arc_host_spi_slave_transmit_calls = 0U;
    expect(HostSpiSlave::send(spi_tx.data(), 17U, 0U) == ESP_ERR_INVALID_ARG &&
               arc_host_spi_slave_transmit_calls == 0U,
           "SPI slave send rejects oversize before driver call");
    arc_host_spi_slave_queue_calls = 0U;
    expect(HostSpiSlave::queue(spi_slave_oversize_job, 0U) == ESP_ERR_INVALID_ARG &&
               arc_host_spi_slave_queue_calls == 0U,
           "SPI slave queue rejects invalid transaction before driver call");

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

    auto owned_src_dma = arc::dmabuf<std::uint8_t>(65U);
    auto owned_dst_dma = arc::dmabuf<std::uint8_t>(65U);
    expect(static_cast<bool>(owned_src_dma) && static_cast<bool>(owned_dst_dma),
           "Copy owned lease buffers allocate");
    owned_src_dma[0] = 55U;
    owned_src_dma[64] = 77U;
    auto owned_lease = arc::Copy<>::lease_coherent(std::move(owned_dst_dma), std::move(owned_src_dma));
    // NOLINTBEGIN(bugprone-use-after-move): verifies moved-from buffers are inert after lease transfer.
    expect(owned_lease.has_value() &&
               owned_lease->active() &&
               !static_cast<bool>(owned_src_dma) &&
               !static_cast<bool>(owned_dst_dma),
           "Copy owned lease moves buffers");
    // NOLINTEND(bugprone-use-after-move)
    expect(owned_lease->finish().has_value() &&
               !owned_lease->active() &&
               owned_lease->dst()[0] == 55U &&
               owned_lease->dst()[64] == 77U,
           "Copy owned lease finishes before reads");
    auto returned_dst = owned_lease->take_dst();
    expect(returned_dst.has_value() && (*returned_dst)[0] == 55U && (*returned_dst)[64] == 77U,
           "Copy owned lease returns destination");

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
    const auto null_cloak_stats = arc::crypto::Cloak::scramble<CloakPolicy>(
        {.stall_mask = 0x03U, .dummy_reads = 3U},
        std::span<const std::byte>{static_cast<const std::byte*>(nullptr), 1U});
    expect(null_cloak_stats.stalls == 4U && null_cloak_stats.dummy_reads == 0U,
           "Cloak skips null dummy span");
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
    expect(!snn.step(
               std::span<const std::int8_t, 16>{static_cast<const std::int8_t*>(nullptr), snn_input.size()},
               std::span<std::uint8_t, 2>{snn_spikes}),
           "SNN rejects null input span");
    arc::ml::Snn<16, 2> null_snn{
        .weights = std::span<const std::int8_t, 32>{static_cast<const std::int8_t*>(nullptr), snn_weights.size()},
        .params = {.threshold = 64, .leak = 0, .reset = 0},
    };
    expect(!null_snn.step(std::span<const std::int8_t, 16>{snn_input}, std::span<std::uint8_t, 2>{snn_spikes}),
           "SNN rejects null weights span");

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
    auto forged_fabric_packet = arc::net::FabricPacket<8>{
        .src = 1U,
        .dst = 4U,
        .ttl = 1U,
        .bytes = 9U,
    };
    expect(!arc::net::Fabric::valid_packet(forged_fabric_packet) &&
               !arc::net::Fabric::forward<FabricPolicy>(fabric_routes, forged_fabric_packet, mesh_schedule, 150U),
           "Fabric rejects forged packet length");
    const arc::net::FabricTable<1> zero_fabric_route{.routes = {arc::net::FabricRoute{.dst = 0U, .next_hop = 2U, .ttl = 1U}}};
    expect(!arc::net::Fabric::lookup(zero_fabric_route, 0U),
           "Fabric lookup rejects zero destination route");
    expect(!arc::net::Fabric::make<8>(
               1U,
               4U,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
               2U),
           "Fabric rejects null payload span");

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
    expect(!arc::ulp::ml::AudioSignature<4, 2>::bins(
               std::span<const std::int16_t, 4>{static_cast<const std::int16_t*>(nullptr), 4U},
               std::span<std::int8_t, 2>{audio_bins},
               10),
           "ULP audio signature rejects null samples span");
    expect(!arc::ulp::ml::AudioSignature<4, 2>::bins(
               std::span<const std::int16_t, 4>{audio_samples},
               std::span<std::int8_t, 2>{static_cast<std::int8_t*>(nullptr), 2U},
               10),
           "ULP audio signature rejects null output span");

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
    expect(!arc::usb::Host::submit<UsbHostPolicy>({
               .address = 2U,
               .endpoint = 1U,
               .tx = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
           }),
           "USB host rejects null TX span");
    expect(!arc::usb::Host::submit<UsbHostPolicy>({
               .address = 2U,
               .endpoint = 1U,
               .rx = std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), 1U},
           }),
           "USB host rejects null RX span");
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
    const arc::net::RdmaWindow wrapping_rdma_window{
        .node = 2U,
        .base = std::numeric_limits<std::uintptr_t>::max() - 7U,
        .bytes = 16U,
        .alignment = 1U,
    };
    expect(!arc::net::Rdma::range_fit(wrapping_rdma_window) &&
               !arc::net::Rdma::write<8>(wrapping_rdma_window, 1U, rdma_payload, 0U),
           "RDMA write rejects wrapping remote windows");
    expect(!arc::net::Rdma::write<8>(
               rdma_window,
               1U,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
               0U),
           "RDMA write rejects null payload span");
    arc::net::RdmaFrame<8> wrapping_rdma_frame{
        .header = {.src = 1U, .dst = 2U, .offset = 0U, .bytes = 3U},
    };
    expect(!arc::net::Rdma::apply<RdmaPolicy>(wrapping_rdma_window, wrapping_rdma_frame),
           "RDMA apply rejects wrapping local windows");
    arc::net::RdmaFrame<8> zero_src_rdma_frame{
        .header = {.src = 0U, .dst = 2U, .offset = 0U, .bytes = 1U},
    };
    expect(!arc::net::Rdma::apply<RdmaPolicy>(rdma_window, zero_src_rdma_frame),
           "RDMA apply rejects zero source node");
    arc::net::RdmaFrame<8> zero_bytes_rdma_frame{
        .header = {.src = 1U, .dst = 2U, .offset = 0U, .bytes = 0U},
    };
    expect(!arc::net::Rdma::apply<RdmaPolicy>(rdma_window, zero_bytes_rdma_frame),
           "RDMA apply rejects empty writes");
    const arc::net::RdmaWindow zero_node_rdma_window{
        .node = 0U,
        .base = reinterpret_cast<std::uintptr_t>(rdma_dst.data()),
        .bytes = rdma_dst.size(),
    };
    arc::net::RdmaFrame<8> zero_dst_rdma_frame{
        .header = {.src = 1U, .dst = 0U, .offset = 0U, .bytes = 1U},
    };
    expect(!arc::net::Rdma::apply<RdmaPolicy>(zero_node_rdma_window, zero_dst_rdma_frame),
           "RDMA apply rejects zero local node");

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
    const arc::mmu::DistributedSpan<std::uint16_t> huge_distributed_span{
        .base = 0x1000U,
        .count = (std::numeric_limits<std::size_t>::max() / sizeof(std::uint16_t)) + 1U,
        .node = 1U,
        .line_bytes = 32U,
    };
    expect(!huge_distributed_span.bytes_fit() && !huge_distributed_span.contains(0x1000U) &&
               !huge_distributed_span.fault(0x1000U),
           "DistributedSpan rejects byte-size overflow");
    const auto max_address = std::numeric_limits<std::uintptr_t>::max();
    const arc::mmu::DistributedSpan<std::uint32_t> wrapping_distributed_span{
        .base = max_address - 7U,
        .count = 3U,
        .node = 1U,
        .line_bytes = 4U,
    };
    expect(wrapping_distributed_span.bytes_fit() && !wrapping_distributed_span.range_fit() &&
               !wrapping_distributed_span.contains(max_address) && !wrapping_distributed_span.fault(max_address),
           "DistributedSpan rejects wrapping address ranges");

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
    struct NullTraceLivePolicy {
        static arc::Result<arc::trace::LiveChunk> trace_half(std::size_t) noexcept
        {
            return arc::trace::LiveChunk{
                .bytes = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
                .bank = 1U,
            };
        }

        static esp_err_t trace_swap() noexcept
        {
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
    expect(!arc::trace::LiveStream::arm<TraceLivePolicy>({.bank_bytes = 64U, .watermark_bytes = 0U}) &&
               !arc::trace::LiveStream::half_isr<TraceLivePolicy>({.bank_bytes = 64U, .watermark_bytes = 0U}) &&
               !arc::trace::LiveStream::half_isr<TraceLivePolicy>({.bank_bytes = 64U, .watermark_bytes = 65U}),
           "LiveStream rejects invalid trace bank geometry");
    expect(!arc::trace::LiveStream::half_isr<NullTraceLivePolicy>({.bank_bytes = 64U, .watermark_bytes = 32U}),
           "LiveStream rejects null trace chunk span");
    expect(!arc::trace::LiveStream::exfiltrate(
               {.bytes = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}},
               trace_sink),
           "LiveStream rejects null exfiltration span");

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
    arc::power::Governor<float> saturated_governor{};
    saturated_governor.seeded = true;
    saturated_governor.slack_filter.x(0, 0) = 1.0e30F;
    const auto saturated_decision = saturated_governor.decide(
        100U,
        100U,
        {
            .guard_cycles = 0,
            .min_samples = 1U,
            .process_noise = std::numeric_limits<float>::quiet_NaN(),
            .measurement_noise = -1.0F,
        });
    expect(saturated_decision.predicted_slack == std::numeric_limits<std::int32_t>::max() &&
               saturated_decision.action == arc::power::GovernorAction::release,
           "Governor clamps huge filtered slack and sanitizes invalid noise");

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
    const auto null_captured_gray = arc::vision::VSlam::capture_gray<VslamCamera>(
        std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), gray.size()});
    expect(captured_gray.has_value() && (*captured_gray)[7] == 7U && !null_captured_gray.has_value(),
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
    const auto null_fast_gray = arc::vision::VSlam::fast_corners(
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), gray.size()},
        10U,
        10U,
        corners,
        40U);
    const auto null_fast_out = arc::vision::VSlam::fast_corners(
        gray,
        10U,
        10U,
        std::span<arc::vision::Corner>{static_cast<arc::vision::Corner*>(nullptr), corners.size()},
        40U);
    const auto too_wide_fast = arc::vision::VSlam::fast_corners(
        gray,
        static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U,
        10U,
        corners,
        40U);
    expect(!too_wide_fast && too_wide_fast.error() == ESP_ERR_INVALID_ARG,
           "VSlam rejects corner coordinate overflow");
    std::array<std::uint8_t, 4> tiny_vslam_gray{};
    const auto oversized_fast = arc::vision::VSlam::fast_corners(
        std::span(tiny_vslam_gray),
        (std::numeric_limits<std::size_t>::max() / 7U) + 1U,
        7U,
        corners,
        40U);
    expect(!oversized_fast && oversized_fast.error() == ESP_ERR_INVALID_ARG, "VSlam rejects oversized frame");
    const std::array<arc::vision::Corner, 2> prev_corners{
        arc::vision::Corner{.x = 10U, .y = 10U, .score = 12U},
        arc::vision::Corner{.x = 20U, .y = 12U, .score = 12U},
    };
    const std::array<arc::vision::Corner, 2> curr_corners{
        arc::vision::Corner{.x = 12U, .y = 11U, .score = 12U},
        arc::vision::Corner{.x = 22U, .y = 13U, .score = 12U},
    };
    const auto essential = arc::vision::VSlam::from_flow(prev_corners, curr_corners, 2U, 100.0F);
    const auto null_essential_previous = arc::vision::VSlam::from_flow(
        std::span<const arc::vision::Corner>{static_cast<const arc::vision::Corner*>(nullptr), prev_corners.size()},
        curr_corners,
        2U,
        100.0F);
    const auto null_essential_current = arc::vision::VSlam::from_flow(
        prev_corners,
        std::span<const arc::vision::Corner>{static_cast<const arc::vision::Corner*>(nullptr), curr_corners.size()},
        2U,
        100.0F);
    arc::nav::Eskf<float>::State vision_state{};
    expect(fast.has_value() && !fast->empty() && !null_fast_gray.has_value() &&
               !null_fast_out.has_value() && essential.has_value() &&
               !null_essential_previous.has_value() && !null_essential_current.has_value() &&
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
    const std::array<arc::net::FtmPeer, 4> far_ftm_peers{
        arc::net::FtmPeer{.node_id = 1U, .position = {.x_mm = 1'000'000.0F, .y_mm = 1'000'000.0F, .z_mm = 1'000'000.0F}, .channel = 6U},
        arc::net::FtmPeer{.node_id = 2U, .position = {.x_mm = 1'005'000.0F, .y_mm = 1'000'000.0F, .z_mm = 1'000'000.0F}, .channel = 6U},
        arc::net::FtmPeer{.node_id = 3U, .position = {.x_mm = 1'000'000.0F, .y_mm = 1'005'000.0F, .z_mm = 1'000'000.0F}, .channel = 6U},
        arc::net::FtmPeer{.node_id = 4U, .position = {.x_mm = 1'000'000.0F, .y_mm = 1'000'000.0F, .z_mm = 1'005'000.0F}, .channel = 6U},
    };
    const arc::swarm::Position3 far_rf_point{.x_mm = 1'001'234.5F, .y_mm = 1'002'345.25F, .z_mm = 1'003'456.75F};
    const std::array<arc::net::FtmMeasurement, 4> far_ftm_ranges{
        arc::net::FtmMeasurement{.node_id = 1U, .distance_mm = rf_range(far_ftm_peers[0].position, far_rf_point)},
        arc::net::FtmMeasurement{.node_id = 2U, .distance_mm = rf_range(far_ftm_peers[1].position, far_rf_point)},
        arc::net::FtmMeasurement{.node_id = 3U, .distance_mm = rf_range(far_ftm_peers[2].position, far_rf_point)},
        arc::net::FtmMeasurement{.node_id = 4U, .distance_mm = rf_range(far_ftm_peers[3].position, far_rf_point)},
    };
    const auto far_ftm_position = arc::net::Ftm::trilaterate(
        std::span<const arc::net::FtmPeer, 4>{far_ftm_peers},
        std::span<const arc::net::FtmMeasurement, 4>{far_ftm_ranges});
    expect(far_ftm_position.has_value() &&
               near(far_ftm_position->x_mm, far_rf_point.x_mm, 0.25F) &&
               near(far_ftm_position->y_mm, far_rf_point.y_mm, 0.25F) &&
               near(far_ftm_position->z_mm, far_rf_point.z_mm, 0.25F),
           "FTM trilateration keeps precision with large coordinates");

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
    const std::array<arc::swarm::FleetPeer, 4> fleet_peers{
        arc::swarm::FleetPeer{.node = 11U, .slack = 4, .free_cycles = 64U, .overruns = 0U, .online = true},
        arc::swarm::FleetPeer{.node = 77U, .slack = 20, .free_cycles = 256U, .overruns = 0U, .online = true},
        arc::swarm::FleetPeer{.node = 88U, .slack = 40, .free_cycles = 512U, .overruns = 1U, .online = true},
        arc::swarm::FleetPeer{.node = 99U, .slack = 80, .free_cycles = 1024U, .overruns = 0U, .online = false},
    };
    const auto idle_core = arc::swarm::AnyIdleCore::select(
        std::span<const arc::swarm::FleetPeer, fleet_peers.size()>{fleet_peers},
        0,
        128U);
    expect(idle_core.has_value() && idle_core->peer == 77U && idle_core->free_cycles == 256U,
           "AnyIdleCore selects the best online non-overrun peer");
    expect(!arc::swarm::AnyIdleCore::select(
               std::span<const arc::swarm::FleetPeer, fleet_peers.size()>{fleet_peers},
               64,
               1U),
           "AnyIdleCore rejects fleets without an eligible peer");
    expect(!arc::swarm::AnyIdleCore::select(
               std::span<const arc::swarm::FleetPeer, 1>{static_cast<const arc::swarm::FleetPeer*>(nullptr), 1U}),
           "AnyIdleCore rejects null fleet spans");
    const auto migrate_decision = arc::swarm::Migrator::from_governor(
        {.action = arc::power::GovernorAction::boost, .predicted_slack = -5, .overrun_risk = true},
        idle_core.value());
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
               migrate_decision.slack == idle_core->slack &&
               migration_process == 3U && migration_ip == 0x200U && migration_sp == 0x100U &&
               resumed_memory[0] == 1U && resumed_memory[3] == 4U,
           "Migrator snapshots, teleports, and resumes WASM process state");
    expect(!arc::swarm::Migrator::snapshot<16>(
               3U,
               {
                   .linear_memory = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
                   .stack_pointer = 0x100U,
                   .instruction_pointer = 0x200U,
                   .fuel = 9U,
               }),
           "Migrator rejects null snapshot memory span");
    expect(migration_frame.has_value() &&
               !arc::swarm::Migrator::resume<MigrationPolicy>(
                   *migration_frame,
                   std::span<std::uint8_t>{static_cast<std::uint8_t*>(nullptr), wasm_memory.size()}),
           "Migrator rejects null resume memory span");
    auto forged_migration_frame = arc::swarm::MigrationFrame<16>{
        .process_id = 3U,
        .memory_bytes = 17U,
    };
    expect(!arc::swarm::Migrator::valid_frame(forged_migration_frame) &&
               arc::swarm::Migrator::bytes(forged_migration_frame).empty() &&
               !arc::swarm::Migrator::teleport<MigrationPolicy>(migrate_decision.peer, forged_migration_frame) &&
               !arc::swarm::Migrator::resume<MigrationPolicy>(forged_migration_frame, resumed_memory),
           "Migrator rejects forged payload length");

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
    std::array<arc::power::PowerSample, 1> saturated_power_sample{};
    const std::array<std::uint16_t, 1> high_current_lsb{std::numeric_limits<std::uint16_t>::max()};
    const std::array<std::uint32_t, 1> high_pc{0x30U};
    const auto saturated_power = arc::power::Profiler::interleave(
        high_current_lsb,
        high_pc,
        saturated_power_sample,
        {.sample_hz = 1U, .milliamps_per_lsb = 1.0e9F});
    const auto invalid_scale_power = arc::power::Profiler::interleave(
        high_current_lsb,
        high_pc,
        saturated_power_sample,
        {.sample_hz = 1U, .milliamps_per_lsb = std::numeric_limits<float>::infinity()});
    const auto wrapped_ticks = arc::power::Profiler::interleave(
        std::span<const std::uint16_t>{current_lsb.data(), std::numeric_limits<std::uint32_t>::max()},
        std::span<const std::uint32_t>{pcs.data(), std::numeric_limits<std::uint32_t>::max()},
        std::span<arc::power::PowerSample>{power_samples.data(), std::numeric_limits<std::uint32_t>::max()},
        {.sample_hz = 1U, .milliamps_per_lsb = 1.0F});
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
               saturated_power.has_value() &&
               (*saturated_power)[0].milliamps == std::numeric_limits<std::uint16_t>::max() &&
               !invalid_scale_power.has_value() && !wrapped_ticks.has_value() &&
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
    const auto null_lifi_in = arc::optical::LiFi::encode(
        std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), lifi_bytes.size()},
        lifi_symbols,
        {.half_ticks = 3U});
    const auto null_lifi_out = arc::optical::LiFi::encode(
        lifi_bytes,
        std::span<arc::optical::LiFiSymbol>{static_cast<arc::optical::LiFiSymbol*>(nullptr), lifi_symbols.size()},
        {.half_ticks = 3U});
    expect(lifi_encoded.has_value() && lifi_encoded->size() == 8U &&
               (*lifi_encoded)[0].first_high && !(*lifi_encoded)[0].second_high &&
               !(*lifi_encoded)[1].first_high && (*lifi_encoded)[1].second_high &&
               arc::optical::LiFi::transmit<LiFiPolicy>(*lifi_encoded).has_value() &&
               arc::optical::LiFi::arm_receiver<LiFiPolicy>().has_value() &&
               lifi_sent_symbols == 8U && lifi_armed == 1U &&
               !null_lifi_in.has_value() && !null_lifi_out.has_value() &&
               !arc::optical::LiFi::transmit<LiFiPolicy>(
                    std::span<const arc::optical::LiFiSymbol>{
                        static_cast<const arc::optical::LiFiSymbol*>(nullptr),
                        1U})
                    .has_value(),
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
    expect(!arc::vm::Jit::translate<JitPolicy>(
               std::span<const arc::vm::BpfInsn>{static_cast<const arc::vm::BpfInsn*>(nullptr), 1U},
               jit_image),
           "BPF JIT rejects null program span");
    expect(!arc::vm::Jit::translate<JitPolicy>(
               jit_program,
               std::span<std::uint32_t>{static_cast<std::uint32_t*>(nullptr), jit_program.size()}),
           "BPF JIT rejects null output span");

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
    expect(!arc::vm::WasmAot::translate<WasmPolicy>(
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), wasm_code.size()},
               wasm_image),
           "WasmAot rejects null bytecode span");
    expect(!arc::vm::WasmAot::translate<WasmPolicy>(
               wasm_code,
               std::span<std::uint32_t>{static_cast<std::uint32_t*>(nullptr), wasm_image.size()}),
           "WasmAot rejects null output span");
    expect(!arc::vm::WasmSandbox::protect<WasmGuardPolicy>(
               std::span<const std::uint32_t>{static_cast<const std::uint32_t*>(nullptr), 1U}),
           "WasmSandbox rejects null image span");

    struct HotSwapPolicy {
        static esp_err_t verify(const arc::vm::HotSwapPlan plan) noexcept
        {
            ++hotswap_verified;
            return plan.version == 7U && !plan.signature.empty() ? ESP_OK : ESP_FAIL;
        }

        static esp_err_t protect(const arc::vm::HotSwapImage image) noexcept
        {
            hotswap_protected_words = image.words.size();
            return image.entry == image.words.data() ? ESP_OK : ESP_FAIL;
        }

        static esp_err_t activate(const arc::vm::HotSwapImage image) noexcept
        {
            hotswap_activated_version = image.version;
            return image.entry == image.words.data() ? ESP_OK : ESP_FAIL;
        }
    };

    const std::array<std::uint8_t, 4> hotswap_signature{0x41U, 0x52U, 0x43U, 0x21U};
    hotswap_verified = 0U;
    hotswap_protected_words = 0U;
    hotswap_activated_version = 0U;
    std::array<std::uint32_t, 8> hotswap_wasm_image{};
    const arc::vm::HotSwapPlan hotswap_wasm{
        .kind = arc::vm::HotSwapKind::wasm,
        .version = 7U,
        .payload = wasm_code,
        .signature = hotswap_signature,
    };
    arc::vm::HotSwapGate hotswap_gate{};
    expect(hotswap_gate.accept(hotswap_wasm).has_value(), "HotSwapGate accepts signed monotonic plan");
    const auto staged_wasm = arc::vm::HotSwap::wasm<HotSwapPolicy, WasmPolicy>(hotswap_wasm, hotswap_wasm_image);
    expect(staged_wasm.has_value() &&
               staged_wasm->kind == arc::vm::HotSwapKind::wasm &&
               staged_wasm->words.size() == 4U &&
               hotswap_verified == 1U &&
               hotswap_protected_words == 4U &&
               arc::vm::HotSwap::activate<HotSwapPolicy>(*staged_wasm).has_value() &&
               hotswap_gate.commit(*staged_wasm).has_value() &&
               hotswap_gate.active == 7U &&
               hotswap_activated_version == 7U,
           "HotSwap verifies, protects, and activates signed WASM image");
    expect(!hotswap_gate.accept(hotswap_wasm), "HotSwapGate rejects replayed active version");

    std::array<arc::vm::BpfInsn, 4> hotswap_bpf_decoded{};
    std::array<std::uint32_t, 4> hotswap_bpf_image{};
    const std::span<const std::uint8_t> hotswap_bpf_payload{
        reinterpret_cast<const std::uint8_t*>(jit_program.data()),
        jit_program.size() * sizeof(arc::vm::BpfInsn),
    };
    const arc::vm::HotSwapPlan hotswap_bpf{
        .kind = arc::vm::HotSwapKind::bpf,
        .version = 7U,
        .payload = hotswap_bpf_payload,
        .signature = hotswap_signature,
    };
    const auto staged_bpf = arc::vm::HotSwap::bpf<HotSwapPolicy, JitPolicy>(
        hotswap_bpf,
        hotswap_bpf_decoded,
        hotswap_bpf_image);
    expect(staged_bpf.has_value() &&
               staged_bpf->kind == arc::vm::HotSwapKind::bpf &&
               staged_bpf->words.size() == jit_program.size() &&
               hotswap_verified == 2U &&
               hotswap_protected_words == jit_program.size(),
           "HotSwap stages signed BPF bytecode through JIT policy");
    expect(!arc::vm::HotSwap::wasm<HotSwapPolicy, WasmPolicy>(hotswap_bpf, hotswap_wasm_image),
           "HotSwap rejects payload kind mismatch");
    expect(!arc::vm::HotSwap::verify<HotSwapPolicy>({
               .kind = arc::vm::HotSwapKind::wasm,
               .version = 7U,
               .payload = wasm_code,
               .signature = {},
           }),
           "HotSwap rejects unsigned payload");
    expect(!arc::vm::HotSwap::verify<HotSwapPolicy>({
               .kind = arc::vm::HotSwapKind::wasm,
               .version = 0U,
               .payload = wasm_code,
               .signature = hotswap_signature,
           }),
           "HotSwap rejects unversioned payload");
    arc::vm::HotSwapGate mismatch_gate{};
    expect(mismatch_gate.accept(hotswap_wasm).has_value(), "HotSwapGate accepts staged mismatch fixture");
    const arc::vm::HotSwapImage wrong_image{
        .kind = arc::vm::HotSwapKind::bpf,
        .version = hotswap_wasm.version,
        .words = std::span<const std::uint32_t>{hotswap_wasm_image}.first(1U),
        .entry = hotswap_wasm_image.data(),
    };
    expect(!mismatch_gate.commit(wrong_image), "HotSwapGate rejects mismatched staged image");

    const std::array<std::uint8_t, 1> sleb_minus_one_bytes{0x7fU};
    arc::vm::WasmCursor sleb_minus_one{.bytes = sleb_minus_one_bytes};
    const auto sleb_minus_one_value = sleb_minus_one.sleb();
    expect(sleb_minus_one_value.has_value() && *sleb_minus_one_value == -1,
           "WasmCursor decodes negative one without signed shift");
    const std::array<std::uint8_t, 5> uleb_max_bytes{0xffU, 0xffU, 0xffU, 0xffU, 0x0fU};
    arc::vm::WasmCursor uleb_max{.bytes = uleb_max_bytes};
    const auto uleb_max_value = uleb_max.uleb();
    expect(uleb_max_value.has_value() && *uleb_max_value == std::numeric_limits<std::uint32_t>::max(),
           "WasmCursor decodes max u32 ULEB");
    const std::array<std::uint8_t, 5> uleb_overflow_bytes{0xffU, 0xffU, 0xffU, 0xffU, 0x10U};
    arc::vm::WasmCursor uleb_overflow{.bytes = uleb_overflow_bytes};
    expect(!uleb_overflow.uleb(), "WasmCursor rejects overflowing u32 ULEB");
    const std::array<std::uint8_t, 5> sleb_min_bytes{0x80U, 0x80U, 0x80U, 0x80U, 0x78U};
    arc::vm::WasmCursor sleb_min{.bytes = sleb_min_bytes};
    const auto sleb_min_value = sleb_min.sleb();
    expect(sleb_min_value.has_value() && *sleb_min_value == std::numeric_limits<std::int32_t>::min(),
           "WasmCursor decodes INT32_MIN without signed shift overflow");
    arc::vm::WasmCursor null_wasm_cursor{
        .bytes = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U},
    };
    expect(!null_wasm_cursor.u8(), "WasmCursor rejects null byte span");

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
    expect(!arc::covert::BlackBox::seal<BlackBoxHash, 4>(
               9U,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), flight_record.size()},
               std::span<const std::uint8_t, 4>{previous_root},
               die_key),
           "BlackBox seal rejects null payload span");
    expect(!arc::covert::BlackBox::seal<BlackBoxHash, 4>(
               9U,
               flight_record,
               std::span<const std::uint8_t, 4>{static_cast<const std::uint8_t*>(nullptr), previous_root.size()},
               die_key),
           "BlackBox seal rejects null previous root span");
    expect(sealed.has_value() &&
               !arc::covert::BlackBox::append<BlackBoxStore>(
                   32U,
                   *sealed,
                   std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), flight_record.size()}),
           "BlackBox append rejects null payload span");
    blackbox_write_bytes = 0U;
    expect(sealed.has_value() &&
               !arc::covert::BlackBox::append<BlackBoxStore>(
                   std::numeric_limits<std::uint32_t>::max() - static_cast<std::uint32_t>(sizeof(*sealed)) + 1U,
                   *sealed,
                   flight_record) &&
               blackbox_write_bytes == 0U,
           "BlackBox append rejects wrapping storage offset");

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
    Twin::State forecast_state{};
    forecast_state.x[0] = 1.0F;
    Twin::Model forecast_model{};
    forecast_model.a[0][0] = 1.0F;
    forecast_model.b[0][0] = 1.0F;
    forecast_model.c[0][0] = 1.0F;
    const std::array<Twin::InputVec, 3> forecast_inputs{Twin::InputVec{1.0F}, Twin::InputVec{2.0F}, Twin::InputVec{3.0F}};
    std::array<Twin::OutputVec, 3> forecast_outputs{};
    const auto forecast = Twin::forecast<forecast_inputs.size()>(
        forecast_state,
        forecast_model,
        std::span<const Twin::InputVec, forecast_inputs.size()>{forecast_inputs},
        std::span<Twin::OutputVec, forecast_outputs.size()>{forecast_outputs});
    expect(forecast.has_value() && *forecast == forecast_outputs.size() &&
               near(forecast_outputs[0][0], 1.0F) &&
               near(forecast_outputs[1][0], 2.0F) &&
               near(forecast_outputs[2][0], 4.0F) &&
               near(forecast_state.x[0], 1.0F),
           "DigitalTwin forecasts fixed horizon without mutating live state");
    expect(!Twin::forecast<1U>(
               forecast_state,
               forecast_model,
               std::span<const Twin::InputVec, 1U>{static_cast<const Twin::InputVec*>(nullptr), 1U},
               std::span<Twin::OutputVec, 1U>{forecast_outputs.data(), 1U}),
           "DigitalTwin forecast rejects null input span");

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
    expect(!cli.parse(std::span<const char>{static_cast<const char*>(nullptr), 1U}),
           "Cli rejects null input span");

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

        static arc::Result<arc::SdioSlaveTransfer> sdio_finish(const std::uint32_t timeout_ms) noexcept
        {
            sdio_finish_timeout = timeout_ms;
            return arc::SdioSlaveTransfer{.address = 0x1000U, .bytes = sdio_queued_bytes};
        }

        static esp_err_t sdio_interrupt(const std::uint32_t bits) noexcept
        {
            sdio_irq_bits = bits;
            return ESP_OK;
        }
    };
    std::array<std::uint8_t, 32> sdio_small{};
    expect(!arc::SdioSlave::queue_coherent<SdioPolicy>(sdio_small), "SdioSlave rejects non-cache-line DMA buffer");
    alignas(arc::cache_line) std::array<std::uint8_t, arc::cache_line> sdio_buffer{};
    expect(arc::SdioSlave::start<SdioPolicy>().has_value() &&
               arc::SdioSlave::queue_coherent<SdioPolicy>(sdio_buffer).has_value(),
           "SdioSlave queues coherent DMA");
    const auto sdio_done = arc::SdioSlave::finish_coherent<SdioPolicy>(10U);
    expect(
        sdio_done.has_value() &&
            arc::SdioSlave::interrupt_host<SdioPolicy>(0x2U).has_value() &&
            sdio_done->bytes == sdio_buffer.size() && sdio_queued_bytes == sdio_buffer.size() && sdio_irq_bits == 0x2U,
        "SdioSlave finishes coherent DMA and host interrupt");
    auto sdio_lease = arc::SdioSlave::lease_coherent<SdioPolicy>(sdio_buffer);
    expect(sdio_lease.has_value() && sdio_lease->active(), "SdioSlave coherent lease queues");
    const auto leased_done = sdio_lease->finish(10U);
    expect(leased_done.has_value() && leased_done->bytes == sdio_buffer.size() && !sdio_lease->active(),
           "SdioSlave coherent lease invalidates on finish");
    auto sdio_default_lease = arc::SdioSlave::lease_coherent<SdioPolicy>(sdio_buffer);
    expect(sdio_default_lease.has_value(), "SdioSlave coherent lease can use default finish timeout");
    if (sdio_default_lease) {
        const auto default_done = sdio_default_lease->finish();
        expect(default_done.has_value() && sdio_finish_timeout == arc::sdio_wait_forever,
               "SdioSlave coherent lease waits by default");
    }

    struct UsbDevicePolicy {
        static esp_err_t ep0_write(const std::span<const std::uint8_t> bytes) noexcept
        {
            usb_ep0_bytes = bytes.size();
            usb_ep0_first = bytes.empty() ? 0U : bytes[0];
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
    expect(usb_ch9.setup({.request_type = 0x80U,
                          .request = static_cast<std::uint8_t>(arc::usb::StandardRequest::get_configuration),
                          .length = 1U})
                   .has_value() &&
               usb_ep0_bytes == 1U && usb_ep0_first == 1U &&
               usb_ch9.setup({.request_type = 0x80U,
                              .request = static_cast<std::uint8_t>(arc::usb::StandardRequest::get_status),
                              .length = 2U})
                   .has_value() &&
               usb_ep0_bytes == 2U && usb_ep0_first == 0U,
           "USB Device reports Chapter 9 status/configuration");
    expect(!usb_ch9.setup({.request_type = 0x40U,
                           .request = static_cast<std::uint8_t>(arc::usb::StandardRequest::set_address),
                           .value = 9U})
                   .has_value() &&
               usb_device_address == 5U,
           "USB Device does not treat class requests as standard requests");

    struct ClassSetup {
        [[nodiscard]] static consteval auto descriptors() noexcept
        {
            return arc::usb::Bulk<0x01U, 0x82U>::descriptors();
        }

        [[nodiscard]] static arc::Status setup(const arc::usb::SetupPacket packet) noexcept
        {
            usb_class_request = packet.request;
            return arc::ok();
        }
    };
    arc::usb::Device<ClassSetup, UsbDevicePolicy> usb_class{};
    expect(usb_class.setup({.request_type = 0x40U, .request = 0x22U}).has_value() && usb_class_request == 0x22U,
           "USB Device dispatches class setup hook");

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
    expect(!arc::net::Thread::send<ThreadPolicy>(
               thread_peer,
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}),
           "Thread send rejects null payload span");

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
    expect(!arc::ble::Mesh::publish<BleMeshPolicy>(
               mesh_address,
               {.company = 0x02E5U, .model = 1U},
               std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}),
           "BLE Mesh publish rejects null payload span");

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
    expect(!arc::x509::Bundle::attach<CertPolicy>(
               {.der = std::span<const std::uint8_t>{static_cast<const std::uint8_t*>(nullptr), 1U}, .certificates = 1U}),
           "x509 bundle rejects null DER span");

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
    expect(!arc::NvsCrypto::mount<NvsPolicy>(
               {.partition = std::string_view{static_cast<const char*>(nullptr), 1U}, .key_partition = "keys"}),
           "NVS crypto rejects null partition string");
    expect(!arc::NvsCrypto::mount<NvsPolicy>(
               {.partition = "cfg", .key_partition = std::string_view{static_cast<const char*>(nullptr), 1U}}),
           "NVS crypto rejects null key partition string");

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

struct ScrubPolicy {
    static inline arc::ScrubFault last{};
    static inline std::size_t captures{};
    static inline std::size_t halts{};

    static void capture(const arc::ScrubFault& fault) noexcept
    {
        last = fault;
        ++captures;
    }

    static void halt() noexcept
    {
        ++halts;
    }
};

void test_memory_scrub()
{
    std::array<std::byte, 4> iram{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    std::array<std::byte, 4> dram{std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}};
    arc::Scrub<3> scrub{};

    expect(!scrub.watch(0U, {}, 1U), "Scrub rejects empty region");
    expect(!scrub.watch(3U, std::as_bytes(std::span{iram}), 1U), "Scrub rejects out-of-range region");
    expect(scrub.watch(0U, std::as_bytes(std::span{iram}), 0x10U).has_value(), "Scrub seals IRAM span");
    expect(scrub.watch(1U, std::as_bytes(std::span{dram}), 0x20U).has_value(), "Scrub seals DRAM span");

    const auto expected_iram = arc::Scrub<3>::crc32(std::as_bytes(std::span{iram}));
    const auto first = scrub.scan();
    expect(first.has_value() && first->index == 0U && first->tag == 0x10U && first->crc == expected_iram,
           "Scrub scans sealed region");
    const auto scanned = scrub.step(2U);
    expect(scanned.has_value() && *scanned == 2U, "Scrub step obeys bounded budget");
    const auto zero_budget = scrub.step(0U);
    expect(!zero_budget && zero_budget.error() == ESP_ERR_INVALID_ARG, "Scrub rejects zero budget");

    dram[2] = std::byte{0xAA};
    ScrubPolicy::last = {};
    ScrubPolicy::captures = 0U;
    ScrubPolicy::halts = 0U;
    const auto fault = scrub.scan<ScrubPolicy>();
    expect(!fault && fault.error() == ESP_ERR_INVALID_STATE, "Scrub rejects changed memory");
    expect(ScrubPolicy::captures == 1U && ScrubPolicy::halts == 1U && ScrubPolicy::last.index == 1U &&
               ScrubPolicy::last.tag == 0x20U && ScrubPolicy::last.expected != ScrubPolicy::last.actual,
           "Scrub reports mismatch to capture and halt hooks");

    expect(scrub.refresh(1U).has_value(), "Scrub refresh reseals legitimate writes");
    scrub.cursor = 1U;
    const auto recovered = scrub.scan();
    expect(recovered.has_value() && recovered->index == 1U && recovered->crc == scrub.regions[1].seal,
           "Scrub scans refreshed region");
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
    auto null_seeded_hyper = Matrix::seeded(std::span<const arc::swarm::Pose6, Matrix::cells>{
        static_cast<const arc::swarm::Pose6*>(nullptr),
        Matrix::cells,
    });
    const auto null_seeded_observe = null_seeded_hyper.observe(Matrix::from_vslam({}, 10, 1.0F));
    const auto null_seeded_pose = null_seeded_hyper.estimate();
    const auto hyper_pose = hyper.estimate();
    auto poisoned_hyper = hyper;
    poisoned_hyper.probability[0] = std::numeric_limits<float>::quiet_NaN();
    expect(!hyper.observe({.pose = {.x_mm = std::numeric_limits<float>::quiet_NaN()}, .variance = 1.0F}) &&
               !hyper.observe({.pose = {}, .variance = std::numeric_limits<float>::infinity()}) &&
               null_seeded_observe.has_value() && null_seeded_pose.has_value() &&
               std::isfinite(null_seeded_pose->x_mm) &&
               !poisoned_hyper.estimate(),
           "HyperMatrix rejects non-finite observations and estimates");
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
    expect(arc::cnc::Kinematics::round_i32(std::numeric_limits<float>::infinity()) == std::numeric_limits<std::int32_t>::max() &&
               arc::cnc::Kinematics::round_i32(-std::numeric_limits<float>::infinity()) == std::numeric_limits<std::int32_t>::min() &&
               arc::cnc::Kinematics::round_i32(std::numeric_limits<float>::quiet_NaN()) == 0,
           "CNC round_i32 handles non-finite inputs without narrowing UB");
    expect(!arc::cnc::Kinematics::delta(
               {0.0F, 0.0F, std::numeric_limits<float>::quiet_NaN()},
               {.arm_mm = 120.0F, .radius_mm = 30.0F, .steps_mm = 10.0F}),
           "CNC delta rejects non-finite coordinates");
    const std::string_view line{"N7 G1 X10 Y5 F1200"};
    const auto block = arc::cnc::GCode::parse_line(std::span<const char>{line.data(), line.size()});
    const std::string_view bad_line{"N-1 G1 X0"};
    const auto bad_block = arc::cnc::GCode::parse_line(std::span<const char>{bad_line.data(), bad_line.size()});
    const std::string_view huge_g{"G2147483648 X0"};
    const auto huge_g_block = arc::cnc::GCode::parse_line(std::span<const char>{huge_g.data(), huge_g.size()});
    const auto null_g_block = arc::cnc::GCode::parse_line(std::span<const char>{static_cast<const char*>(nullptr), 1U});
    std::array<arc::MotionStep<2>, 128> cnc_steps{};
    const auto planned = block.has_value()
        ? arc::cnc::GCode::plan_linear<2>(*block, {0.0F, 0.0F}, 10.0F, cnc_steps)
        : arc::Result<std::span<const arc::MotionStep<2>>>{arc::fail(ESP_ERR_INVALID_STATE)};
    expect(corexy.delta[0] == 1200 && corexy.delta[1] == 400 && corexy.ticks_step == 2U &&
               delta.has_value() && delta->delta[0] > 0 &&
               five.delta[0] == 10 && five.delta[4] == 10 &&
               block.has_value() && block->command == arc::cnc::Command::linear &&
               block->line == 7U && block->has_axis[0] && block->axis[0] == 10.0F &&
               !bad_block.has_value() && !huge_g_block.has_value() && !null_g_block.has_value() &&
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
    const auto null_fir = arc::hls::fir<int, 4>(
        std::span<const int, 4>{static_cast<const int*>(nullptr), coeffs.size()},
        std::span<const int, 4>{samples});
    const auto null_dot = arc::hls::dot<int, 4>(
        std::span<const int, 4>{coeffs},
        std::span<const int, 4>{static_cast<const int*>(nullptr), samples.size()});
    constexpr auto dense_spec = arc::hls::DenseSpec<4, 2>::hls_spec();
    constexpr auto silicon_plan = arc::hls::silicon_plan<
        arc::hls::DenseSpec<4, 2>,
        arc::hls::DenseSpec<2, 1>>(arc::hls::SiliconTarget::efpga);
    expect(fir.has_value() && *fir == 20 && dot.has_value() && *dot == 20 &&
               !null_fir.has_value() && !null_dot.has_value() &&
               dense_spec.static_bounds && dense_spec.heapless &&
               dense_spec.latency_cycles == 8U &&
               silicon_plan.target == arc::hls::SiliconTarget::efpga &&
               silicon_plan.kernels == 2U && silicon_plan.latency_cycles == 10U &&
               silicon_plan.synthesizable,
           "HLS helpers expose fixed-bound heapless kernel metadata and silicon plans");
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
        // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from gate state is inert.
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
        // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from gate state is inert.
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
    // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from init state is inert.
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
    // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from init state is inert.
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
        // NOLINTNEXTLINE(bugprone-use-after-move): verifies moved-from lease state is inert.
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

void test_task_arena()
{
    alignas(16) std::array<std::byte, 64> storage{};
    arc::TaskArena arena{.data = storage.data(), .bytes = storage.size()};

    expect(arena.allocate(1U, 0U) == nullptr && arena.used == 0U, "TaskArena rejects zero alignment");
    expect(arena.allocate(1U, 3U) == nullptr && arena.used == 0U, "TaskArena rejects non-power alignment");
    const auto* first = static_cast<const std::byte*>(arena.allocate(1U, 8U));
    expect(first == storage.data() && arena.used == 1U, "TaskArena first allocation");
    const auto* second = static_cast<const std::byte*>(arena.allocate(1U, 16U));
    expect(second == storage.data() + 16U && arena.used == 17U, "TaskArena aligns second allocation");

    arena.used = storage.size() + 1U;
    expect(arena.allocate(1U, 8U) == nullptr && arena.used == storage.size() + 1U,
           "TaskArena rejects used past capacity");

    arc::TaskArena wrapped{
        .data = reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max() - 3U),
        .bytes = 16U,
        .used = 3U,
    };
    expect(wrapped.allocate(8U, 8U) == nullptr && wrapped.used == 3U,
           "TaskArena rejects alignment overflow");
}

}  // namespace

int main()
{
    test_result();
    test_flow();
    test_any_io();
    test_spsc();
    test_core_local();
    test_static_borrow();
    test_lockstep();
    test_sim();
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
    test_fs();
    test_store();
    test_stream();
    test_pipeline_usb_ulp();
    test_pru_isp_vision();
    test_vm_bpf();
    test_ulp_ml_paillier_covert();
    test_goal_wave_surfaces();
    test_resilient_edge_goal_surfaces();
    test_current_goal_surfaces();
    test_memory_scrub();
    test_hive_goal_surfaces();
    test_task_arena();
    test_refinit();
    std::puts("arc host tests: OK");
}
