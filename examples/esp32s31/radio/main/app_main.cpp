#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

#include "arc/ble_mesh.hpp"
#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"
#include "arc/thread.hpp"

#if __has_include("esp_now.h") && __has_include("esp_wifi.h") && __has_include("esp_event.h")
#include "arc/espnow.hpp"
#define ARC_S31_KORVO_ESPNOW_CONTRACT 1
#else
#define ARC_S31_KORVO_ESPNOW_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

inline constexpr auto mesh_address = arc::ble::MeshAddress{.unicast = 1U, .group = 0xc000U};
inline constexpr auto mesh_model = arc::ble::MeshModel{.company = 0x02e5U, .model = 0x1000U};
inline constexpr auto mesh_payload = std::array<std::uint8_t, 4>{0x31U, 0x01U, 0x06U, 0x54U};

inline constexpr auto thread_dataset = arc::net::ThreadDataset{
    .network_key = {0x31U, 0x01U, 0x14U},
    .pan_id = 0x5031U,
    .channel = 15U,
};
inline constexpr auto thread_peer = arc::net::ThreadPeer{.mesh_local = {0xfdU}, .rloc16 = 0x1234U};
inline constexpr auto thread_payload = std::array<std::uint8_t, 3>{0x15U, 0x04U, 0x31U};

struct MeshPolicy {
    static esp_err_t mesh_provision(const arc::ble::MeshAddress address) noexcept
    {
        return address.unicast == mesh_address.unicast ? ESP_OK : ESP_ERR_INVALID_ARG;
    }

    static esp_err_t mesh_publish(
        const arc::ble::MeshAddress address,
        const arc::ble::MeshModel model,
        const std::span<const std::uint8_t> payload) noexcept
    {
        return address.group == mesh_address.group && model.model == mesh_model.model && payload.size() == mesh_payload.size()
            ? ESP_OK
            : ESP_ERR_INVALID_ARG;
    }
};

struct ThreadPolicy {
    static esp_err_t thread_attach(const arc::net::ThreadDataset& dataset) noexcept
    {
        return dataset.pan_id == thread_dataset.pan_id && dataset.channel == thread_dataset.channel ? ESP_OK : ESP_ERR_INVALID_ARG;
    }

    static esp_err_t thread_send(
        const arc::net::ThreadPeer& peer,
        const std::span<const std::uint8_t> payload) noexcept
    {
        return peer.rloc16 == thread_peer.rloc16 && payload.size() == thread_payload.size() ? ESP_OK : ESP_ERR_INVALID_ARG;
    }
};

#if ARC_S31_KORVO_ESPNOW_CONTRACT
struct EspNowEvent {
    std::uint32_t serial{};
};

struct EspNowBus {
    using event_type = EspNowEvent;
};

struct EspNowPolicy {
    static constexpr auto peer = std::array<std::uint8_t, 6>{0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU};
    static constexpr std::uint8_t channel = 6U;
};

using EspNowPlane = arc::net::EspNow<EspNowPolicy, EspNowBus>;
static_assert(sizeof(EspNowPlane) > 0U);
#endif

static_assert(Board::Wireless::wifi && Board::Wireless::wifi6);
static_assert(Board::Wireless::ble && Board::Wireless::bt54 && Board::Wireless::bt_classic);
static_assert(Board::Wireless::ieee802154 && Board::Wireless::zigbee3 && Board::Wireless::thread14);
static_assert(Board::Wireless::pcb_antenna == Board::Module::pcb_antenna);
static_assert(Board::Wireless::wifi6 == arc::soc::Target::wifi6);
static_assert(Board::Wireless::ble == arc::soc::Target::ble);
static_assert(Board::Wireless::bt54 == arc::soc::Target::bt54);
static_assert(Board::Wireless::bt_classic == arc::soc::Target::bt_classic);
static_assert(Board::Wireless::ieee802154 == arc::soc::Target::ieee802154);
static_assert(arc::Soc::wifi && arc::Soc::ble && arc::Soc::ble_mesh);
static_assert(thread_dataset.pan_id != 0U && thread_dataset.channel >= 11U && thread_dataset.channel <= 26U);
static_assert(thread_peer.rloc16 != 0U);
static_assert(mesh_address.unicast != 0U && mesh_address.group != 0U);
static_assert(mesh_model.model != 0U);

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "radio requires ARC_TARGET=esp32s31");
    static_assert(arc::Topology<Board>);

    const auto provision = arc::ble::Mesh::provision<MeshPolicy>(mesh_address);
    const auto publish = arc::ble::Mesh::publish<MeshPolicy>(mesh_address, mesh_model, mesh_payload);
    const auto attach = arc::net::Thread::attach<ThreadPolicy>(thread_dataset);
    const auto send = arc::net::Thread::send<ThreadPolicy>(thread_peer, thread_payload);

    std::printf(
        "arc-s31-radio target=%s board=%s arch=%s wifi6=%d ble=%d bt54=%d bt_classic=%d ieee802154=%d zigbee3=%d thread14=%d pcb_antenna=%d espnow_contract=%d mesh_provision=%d mesh_publish=%d thread_attach=%d thread_send=%d ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::Wireless::wifi6 ? 1 : 0,
        Board::Wireless::ble ? 1 : 0,
        Board::Wireless::bt54 ? 1 : 0,
        Board::Wireless::bt_classic ? 1 : 0,
        Board::Wireless::ieee802154 ? 1 : 0,
        Board::Wireless::zigbee3 ? 1 : 0,
        Board::Wireless::thread14 ? 1 : 0,
        Board::Wireless::pcb_antenna ? 1 : 0,
        ARC_S31_KORVO_ESPNOW_CONTRACT,
        static_cast<int>(arc::status_code(provision)),
        static_cast<int>(arc::status_code(publish)),
        static_cast<int>(arc::status_code(attach)),
        static_cast<int>(arc::status_code(send)),
        Board::Wireless::wifi6 && Board::Wireless::ble && Board::Wireless::bt54 && Board::Wireless::bt_classic &&
                Board::Wireless::ieee802154
            ? 1
            : 0);
}
