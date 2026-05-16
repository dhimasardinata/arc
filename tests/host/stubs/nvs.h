#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "esp_err.h"

#define ESP_ERR_NVS_NO_FREE_PAGES 0x1101
#define ESP_ERR_NVS_NOT_FOUND 0x1102
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1110
#define ESP_ERR_NVS_INVALID_LENGTH 0x110C

using nvs_handle_t = std::uint32_t;

enum nvs_open_mode_t : std::uint8_t {
    NVS_READONLY = 0,
    NVS_READWRITE = 1,
};

namespace arc_host_nvs {

struct Handle {
    std::string ns{};
    nvs_open_mode_t mode{NVS_READONLY};
    bool active{};
};

inline constexpr std::size_t max_handles = 8U;

inline std::array<Handle, max_handles> handles{};
inline std::unordered_set<std::string> namespaces{};
inline std::unordered_map<std::string, std::string> strings{};
inline std::unordered_map<std::string, std::vector<std::uint8_t>> blobs{};

inline void reset()
{
    for (auto& handle : handles) {
        handle = {};
    }
    namespaces.clear();
    strings.clear();
    blobs.clear();
}

[[nodiscard]] inline Handle* find(const nvs_handle_t raw)
{
    if (raw == 0U || raw > handles.size()) {
        return nullptr;
    }

    auto& handle = handles[raw - 1U];
    return handle.active ? &handle : nullptr;
}

[[nodiscard]] inline std::string key(const Handle& handle, const char* const name)
{
    std::string out = handle.ns;
    out.push_back('\n');
    out.append(name);
    return out;
}

}  // namespace arc_host_nvs

inline esp_err_t nvs_open(const char* const ns, const nvs_open_mode_t mode, nvs_handle_t* const out)
{
    if (ns == nullptr || out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (mode == NVS_READONLY && !arc_host_nvs::namespaces.contains(ns)) {
        return ESP_ERR_NVS_NOT_FOUND;
    }

    for (std::size_t i = 0U; i < arc_host_nvs::handles.size(); ++i) {
        auto& handle = arc_host_nvs::handles[i];
        if (!handle.active) {
            if (mode == NVS_READWRITE) {
                arc_host_nvs::namespaces.insert(ns);
            }
            handle.ns = ns;
            handle.mode = mode;
            handle.active = true;
            *out = static_cast<nvs_handle_t>(i + 1U);
            return ESP_OK;
        }
    }

    return ESP_ERR_NO_MEM;
}

inline void nvs_close(const nvs_handle_t raw)
{
    if (auto* const handle = arc_host_nvs::find(raw); handle != nullptr) {
        *handle = {};
    }
}

inline esp_err_t nvs_commit(const nvs_handle_t raw)
{
    const auto* const handle = arc_host_nvs::find(raw);
    return handle != nullptr && handle->mode == NVS_READWRITE ? ESP_OK : ESP_ERR_INVALID_STATE;
}

inline esp_err_t nvs_set_str(const nvs_handle_t raw, const char* const name, const char* const value)
{
    auto* const handle = arc_host_nvs::find(raw);
    if (handle == nullptr || handle->mode != NVS_READWRITE || name == nullptr || value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    arc_host_nvs::strings[arc_host_nvs::key(*handle, name)] = value;
    return ESP_OK;
}

inline esp_err_t nvs_get_str(
    const nvs_handle_t raw,
    const char* const name,
    char* const out,
    std::size_t* const bytes)
{
    const auto* const handle = arc_host_nvs::find(raw);
    if (handle == nullptr || name == nullptr || bytes == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const auto found = arc_host_nvs::strings.find(arc_host_nvs::key(*handle, name));
    if (found == arc_host_nvs::strings.end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }

    const auto required = found->second.size() + 1U;
    if (out == nullptr) {
        *bytes = required;
        return ESP_OK;
    }
    if (*bytes < required) {
        *bytes = required;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    std::memcpy(out, found->second.c_str(), required);
    *bytes = required;
    return ESP_OK;
}

inline esp_err_t nvs_set_blob(
    const nvs_handle_t raw,
    const char* const name,
    const void* const value,
    const std::size_t bytes)
{
    auto* const handle = arc_host_nvs::find(raw);
    if (handle == nullptr || handle->mode != NVS_READWRITE || name == nullptr || (value == nullptr && bytes != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    auto& out = arc_host_nvs::blobs[arc_host_nvs::key(*handle, name)];
    out.resize(bytes);
    if (bytes != 0U) {
        const auto* const data = static_cast<const std::uint8_t*>(value);
        std::copy(data, data + bytes, out.begin());
    }
    return ESP_OK;
}

inline esp_err_t nvs_get_blob(
    const nvs_handle_t raw,
    const char* const name,
    void* const out,
    std::size_t* const bytes)
{
    const auto* const handle = arc_host_nvs::find(raw);
    if (handle == nullptr || name == nullptr || bytes == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const auto found = arc_host_nvs::blobs.find(arc_host_nvs::key(*handle, name));
    if (found == arc_host_nvs::blobs.end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }

    if (out == nullptr) {
        *bytes = found->second.size();
        return ESP_OK;
    }
    if (*bytes < found->second.size()) {
        *bytes = found->second.size();
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    std::memcpy(out, found->second.data(), found->second.size());
    *bytes = found->second.size();
    return ESP_OK;
}

inline esp_err_t nvs_erase_key(const nvs_handle_t raw, const char* const name)
{
    auto* const handle = arc_host_nvs::find(raw);
    if (handle == nullptr || handle->mode != NVS_READWRITE || name == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const auto full = arc_host_nvs::key(*handle, name);
    const auto erased = arc_host_nvs::strings.erase(full) + arc_host_nvs::blobs.erase(full);
    return erased > 0U ? ESP_OK : ESP_ERR_NVS_NOT_FOUND;
}
