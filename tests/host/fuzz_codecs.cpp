#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/coap.hpp"
#include "arc/http_server.hpp"
#include "arc/mqtt.hpp"
#include "arc/ws.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* const data, const std::size_t size)
{
    if (data == nullptr && size != 0U) {
        return 0;
    }

    const std::span<const std::uint8_t> bytes{data, size};
    std::array<arc::net::HttpHeaderView, 16> headers{};
    static_cast<void>(arc::net::HttpServer::parse(
        std::span<const char>{reinterpret_cast<const char*>(data), size},
        std::span<arc::net::HttpHeaderView>{headers}));
    static_cast<void>(arc::net::Mqtt::parse(bytes));
    static_cast<void>(arc::net::Ws::parse(bytes));
    static_cast<void>(arc::net::Coap::parse(bytes));
    return 0;
}
