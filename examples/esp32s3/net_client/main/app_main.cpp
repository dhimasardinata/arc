#include <array>

#include "arc.hpp"
#include "arc/http.hpp"
#include "arc/tcp.hpp"
#include "arc/tls.hpp"
#include "arc/uri.hpp"

#include "esp_log.h"

namespace app {

inline constexpr char tag[] = "arc-net-client";
inline constexpr char url[] = "https://example.com:443/api/v1/status?unit=ph";

void log_span(const char* const label, const std::span<const char> text)
{
    ESP_LOGI(tag, "%s=%.*s", label, static_cast<int>(text.size()), text.data());
}

void boot()
{
    const auto uri = arc::net::Uri::parse(url);
    if (!uri) {
        ESP_LOGE(tag, "uri parse failed: 0x%x", static_cast<unsigned>(uri.error()));
        return;
    }

    std::array<char, 64> host{};
    std::array<char, 96> target{};
    const auto endpoint = arc::net::Uri::endpoint(*uri, 443U);
    const auto host_text = arc::net::Uri::copy_host(std::span(host), *uri);
    const auto target_text = arc::net::Uri::path_query(std::span(target), *uri);
    if (!endpoint || !host_text || !target_text) {
        ESP_LOGE(
            tag,
            "uri endpoint failed: endpoint=0x%x host=0x%x target=0x%x",
            endpoint ? ESP_OK : endpoint.error(),
            host_text ? ESP_OK : host_text.error(),
            target_text ? ESP_OK : target_text.error());
        return;
    }

    ESP_LOGI(tag, "scheme=https host=%s port=%u", host.data(), static_cast<unsigned>(endpoint->port));
    log_span("target", *target_text);

    auto http = arc::net::Http::https(url);
    ESP_LOGI(tag, "https client configured=%s", http ? "yes" : "no");
    if (http) {
        static_cast<void>(http->close());
    }

    esp_tls_cfg_t cfg{};
    const auto bad_tcp_nul = arc::net::Tcp::dial("tcp://node%00.local/path", std::span(host), 443U);
    const auto bad_tls_nul = arc::net::Tls::dial("https://node%00.local/path", std::span(host), cfg);
    const auto bad_http_nul = arc::net::Http::https("https://node%00.local/path");
    const auto bad_tcp_delim = arc::net::Tcp::dial("tcp://node%2Fadmin.local/path", std::span(host), 443U);
    const auto bad_tls_delim = arc::net::Tls::dial("https://node%2Fadmin.local/path", std::span(host), cfg);
    const auto bad_http_delim = arc::net::Http::https("https://node%2Fadmin.local/path");
    ESP_LOGI(
        tag,
        "unsafe host rejected nul=%s delimiter=%s",
        !bad_tcp_nul && !bad_tls_nul && !bad_http_nul ? "yes" : "no",
        !bad_tcp_delim && !bad_tls_delim && !bad_http_delim ? "yes" : "no");
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
