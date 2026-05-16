#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/coap.hpp"
#include "arc/http_server.hpp"
#include "arc/mqtt.hpp"
#include "arc/uri.hpp"
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

    const std::span<const char> text{reinterpret_cast<const char*>(data), size};
    std::array<char, 256> scratch{};
    const auto uri = arc::net::Uri::parse(text);
    if (uri) {
        static_cast<void>(arc::net::Uri::path_query(std::span<char>{scratch}, *uri));
        static_cast<void>(arc::net::Uri::copy_host(std::span<char>{scratch}, *uri));
        std::size_t offset{};
        while (arc::net::Uri::next(uri->query, offset)) {}
    }
    static_cast<void>(arc::net::Uri::decode(std::span<char>{scratch}, text, true));
    static_cast<void>(arc::net::Uri::encode(std::span<char>{scratch}, text, true));
    return 0;
}
