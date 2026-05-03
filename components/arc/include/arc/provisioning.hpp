#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

namespace arc {

enum class ProvisioningState : std::uint8_t {
    idle,
    hello,
    keys,
    credentials,
    done,
    failed,
};

template <std::size_t SsidBytes, std::size_t PassBytes, std::size_t CertBytes>
struct Provisioning {
    static_assert(SsidBytes > 0U && PassBytes > 0U && CertBytes > 0U, "provisioning buffers must be non-zero");

    [[nodiscard]] esp_err_t begin(std::span<const std::uint8_t> nonce) noexcept
    {
        if (nonce.empty() || nonce.size() > session_nonce.size()) {
            state = ProvisioningState::failed;
            return ESP_ERR_INVALID_ARG;
        }
        nonce_size = nonce.size();
        copy(session_nonce, nonce);
        state = ProvisioningState::hello;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t keys(std::span<const std::uint8_t> peer_key) noexcept
    {
        if (state != ProvisioningState::hello || peer_key.empty() || peer_key.size() > public_key.size()) {
            state = ProvisioningState::failed;
            return ESP_ERR_INVALID_STATE;
        }
        key_size = peer_key.size();
        copy(public_key, peer_key);
        state = ProvisioningState::keys;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t credentials(
        std::span<const std::uint8_t> ssid_in,
        std::span<const std::uint8_t> pass_in,
        std::span<const std::uint8_t> cert_in = {}) noexcept
    {
        if (state != ProvisioningState::keys || ssid_in.empty() || ssid_in.size() > ssid.size() ||
            pass_in.size() > pass.size() || cert_in.size() > cert.size()) {
            state = ProvisioningState::failed;
            return ESP_ERR_INVALID_ARG;
        }
        ssid_size = ssid_in.size();
        pass_size = pass_in.size();
        cert_size = cert_in.size();
        copy(ssid, ssid_in);
        copy(pass, pass_in);
        copy(cert, cert_in);
        state = ProvisioningState::done;
        return ESP_OK;
    }

    [[nodiscard]] bool ready() const noexcept
    {
        return state == ProvisioningState::done;
    }

    ProvisioningState state{ProvisioningState::idle};
    std::array<std::uint8_t, 32> session_nonce{};
    std::array<std::uint8_t, 96> public_key{};
    std::array<std::uint8_t, SsidBytes> ssid{};
    std::array<std::uint8_t, PassBytes> pass{};
    std::array<std::uint8_t, CertBytes> cert{};
    std::size_t nonce_size{};
    std::size_t key_size{};
    std::size_t ssid_size{};
    std::size_t pass_size{};
    std::size_t cert_size{};

private:
    template <std::size_t N>
    static void copy(std::array<std::uint8_t, N>& out, std::span<const std::uint8_t> in) noexcept
    {
        for (std::size_t i = 0; i < in.size(); ++i) {
            out[i] = in[i];
        }
    }
};

}  // namespace arc
