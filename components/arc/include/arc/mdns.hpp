#pragma once

#include <cstdint>
#include <utility>

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "lwip/apps/mdns.h"
#include "lwip/err.h"
#include "lwip/netif.h"

#include "arc/init.hpp"
#include "arc/net.hpp"
#include "arc/result.hpp"

#if defined(LWIP_MDNS_RESPONDER) && LWIP_MDNS_RESPONDER

namespace arc::net {

struct Mdns {
    struct Service {
        constexpr Service() noexcept = default;

        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        Service(Service&& other) noexcept
            : iface_(std::exchange(other.iface_, nullptr))
            , slot_(std::exchange(other.slot_, invalid_slot()))
        {
        }

        Service& operator=(Service&& other) noexcept
        {
            if (this != &other) {
                close();
                iface_ = std::exchange(other.iface_, nullptr);
                slot_ = std::exchange(other.slot_, invalid_slot());
            }
            return *this;
        }

        ~Service()
        {
            close();
        }

        [[nodiscard]] static Result<Service> make(
            esp_netif_t* const iface,
            const char* const name,
            const char* const service,
            const mdns_sd_proto proto,
            const std::uint16_t port,
            service_get_txt_fn_t const txt = nullptr,
            void* const txt_userdata = nullptr) noexcept
        {
            if (iface == nullptr || name == nullptr || service == nullptr || port == 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            const auto ready = Mdns::init();
            if (ready != ESP_OK) {
                return fail(ready);
            }

            Gate guard(state.gate);
            auto* const netif = raw(iface);
            if (netif == nullptr) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            const auto slot = mdns_resp_add_service(netif, name, service, proto, port, txt, txt_userdata);
            if (slot < 0) {
                return fail(map_lwip(static_cast<err_t>(slot)));
            }
            return Service{iface, static_cast<std::uint8_t>(slot)};
        }

        [[nodiscard]] static Result<Service> make(
            const char* const name,
            const char* const service,
            const mdns_sd_proto proto,
            const std::uint16_t port,
            service_get_txt_fn_t const txt = nullptr,
            void* const txt_userdata = nullptr) noexcept
        {
            return make(Radio::sta(), name, service, proto, port, txt, txt_userdata);
        }

        [[nodiscard]] static Result<Service> make_ap(
            const char* const name,
            const char* const service,
            const mdns_sd_proto proto,
            const std::uint16_t port,
            service_get_txt_fn_t const txt = nullptr,
            void* const txt_userdata = nullptr) noexcept
        {
            return ap(name, service, proto, port, txt, txt_userdata);
        }

        [[nodiscard]] static Result<Service> ap(
            const char* const name,
            const char* const service,
            const mdns_sd_proto proto,
            const std::uint16_t port,
            service_get_txt_fn_t const txt = nullptr,
            void* const txt_userdata = nullptr) noexcept
        {
            return make(Radio::ap(), name, service, proto, port, txt, txt_userdata);
        }

        [[nodiscard]] esp_err_t rename(const char* const name) noexcept
        {
            if (iface_ == nullptr || slot_ == invalid_slot() || name == nullptr) {
                return ESP_ERR_INVALID_STATE;
            }

            const auto ready = Mdns::init();
            if (ready != ESP_OK) {
                return ready;
            }

            Gate guard(state.gate);
            auto* const netif = raw(iface_);
            if (netif == nullptr) {
                return ESP_ERR_INVALID_ARG;
            }
            return map_lwip(mdns_resp_rename_service(netif, slot_, name));
        }

        void close() noexcept
        {
            if (iface_ == nullptr || slot_ == invalid_slot()) {
                return;
            }

            if (Mdns::init() != ESP_OK) {
                iface_ = nullptr;
                slot_ = invalid_slot();
                return;
            }

            Gate guard(state.gate);
            if (auto* const netif = raw(iface_); netif != nullptr) {
                static_cast<void>(mdns_resp_del_service(netif, slot_));
            }
            iface_ = nullptr;
            slot_ = invalid_slot();
        }

        [[nodiscard]] std::uint8_t slot() const noexcept
        {
            return slot_;
        }

        [[nodiscard]] esp_netif_t* netif() const noexcept
        {
            return iface_;
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return iface_ != nullptr && slot_ != invalid_slot();
        }

    private:
        explicit Service(esp_netif_t* const iface, const std::uint8_t slot) noexcept
            : iface_(iface)
            , slot_(slot)
        {
        }

        [[nodiscard]] static constexpr std::uint8_t invalid_slot() noexcept
        {
            return 0xffU;
        }

        esp_netif_t* iface_{nullptr};
        std::uint8_t slot_{invalid_slot()};
    };

    static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        mdns_resp_init();
        Init::pass(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t host(
        esp_netif_t* const iface,
        const char* const name) noexcept
    {
        if (iface == nullptr || name == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        Gate guard(state.gate);
        auto* const netif = raw(iface);
        if (netif == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        if (mdns_resp_netif_active(netif) != 0) {
            return map_lwip(mdns_resp_rename_netif(netif, name));
        }
        return map_lwip(mdns_resp_add_netif(netif, name));
    }

    [[nodiscard]] static esp_err_t host(const char* const name) noexcept
    {
        return host(Radio::sta(), name);
    }

    [[nodiscard]] static esp_err_t host_ap(const char* const name) noexcept
    {
        return ap(name);
    }

    [[nodiscard]] static esp_err_t ap(const char* const name) noexcept
    {
        return host(Radio::ap(), name);
    }

    [[nodiscard]] static esp_err_t off(esp_netif_t* const iface) noexcept
    {
        if (iface == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        Gate guard(state.gate);
        auto* const netif = raw(iface);
        if (netif == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return map_lwip(mdns_resp_remove_netif(netif));
    }

    [[nodiscard]] static bool active(esp_netif_t* const iface) noexcept
    {
        if (iface == nullptr || init() != ESP_OK) {
            return false;
        }

        Gate guard(state.gate);
        auto* const netif = raw(iface);
        return netif != nullptr && mdns_resp_netif_active(netif) != 0;
    }

    static esp_err_t announce(esp_netif_t* const iface) noexcept
    {
        if (iface == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        Gate guard(state.gate);
        auto* const netif = raw(iface);
        if (netif == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        mdns_resp_announce(netif);
        return ESP_OK;
    }

    static esp_err_t restart(esp_netif_t* const iface) noexcept
    {
        if (iface == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        Gate guard(state.gate);
        auto* const netif = raw(iface);
        if (netif == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        mdns_resp_restart(netif);
        return ESP_OK;
    }

private:
    struct State {
        std::uint32_t init{};
        std::uint32_t gate{};
    };

    [[nodiscard]] static struct netif* raw(esp_netif_t* const iface) noexcept
    {
        return static_cast<struct netif*>(esp_netif_get_netif_impl(iface));
    }

    [[nodiscard]] static esp_err_t map_lwip(const err_t err) noexcept
    {
        switch (err) {
            case ERR_OK:
                return ESP_OK;
            case ERR_MEM:
            case ERR_BUF:
                return ESP_ERR_NO_MEM;
            case ERR_TIMEOUT:
            case ERR_WOULDBLOCK:
                return ESP_ERR_TIMEOUT;
            case ERR_VAL:
            case ERR_ARG:
                return ESP_ERR_INVALID_ARG;
            case ERR_INPROGRESS:
            case ERR_ALREADY:
            case ERR_USE:
            case ERR_ISCONN:
            case ERR_CONN:
                return ESP_ERR_INVALID_STATE;
            case ERR_RTE:
                return ESP_ERR_NOT_FOUND;
            case ERR_IF:
            case ERR_ABRT:
            case ERR_RST:
            case ERR_CLSD:
            default:
                return ESP_FAIL;
        }
    }

    constinit static inline State state{};
};

}  // namespace arc::net

#endif
