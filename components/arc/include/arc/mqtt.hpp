#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept MqttBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

enum class MqttType : std::uint8_t {
    connect = 1U,
    connack = 2U,
    publish = 3U,
    puback = 4U,
    subscribe = 8U,
    suback = 9U,
    pingreq = 12U,
    pingresp = 13U,
    disconnect = 14U,
};

enum class MqttQos : std::uint8_t {
    at_most = 0U,
    at_least = 1U,
    exactly_once = 2U,
};

struct MqttPacket {
    MqttType type{};
    std::uint8_t flags{};
    std::span<const std::uint8_t> body{};
    std::size_t bytes{};

    [[nodiscard]] constexpr bool dup() const noexcept
    {
        return (flags & 0x08U) != 0U;
    }

    [[nodiscard]] constexpr bool retain() const noexcept
    {
        return (flags & 0x01U) != 0U;
    }

    [[nodiscard]] constexpr MqttQos qos() const noexcept
    {
        return static_cast<MqttQos>((flags >> 1U) & 0x03U);
    }
};

struct MqttConnect {
    const char* client{};
    const char* user{};
    const char* pass{};
    std::uint16_t keep_alive{60U};
    bool clean{true};
};

struct MqttPublish {
    const char* topic{};
    const void* data{};
    std::size_t bytes{};
    std::uint16_t packet{};
    MqttQos qos{MqttQos::at_most};
    bool dup{};
    bool retain{};
};

struct MqttSubscription {
    const char* filter{};
    MqttQos qos{MqttQos::at_most};
};

struct MqttPublishView {
    std::span<const std::uint8_t> topic{};
    std::span<const std::uint8_t> payload{};
    std::uint16_t packet{};
    MqttQos qos{MqttQos::at_most};
    bool dup{};
    bool retain{};
};

struct MqttConnAck {
    bool session{};
    std::uint8_t code{};
};

struct MqttSubAck {
    std::uint16_t packet{};
    std::span<const std::uint8_t> codes{};
};

enum class MqttSessionState : std::uint8_t {
    closed = 0U,
    connecting = 1U,
    online = 2U,
};

struct Mqtt {
    [[nodiscard]] static Result<std::span<const std::uint8_t>> connect(
        std::span<std::uint8_t> out,
        const MqttConnect& cfg) noexcept
    {
        if (cfg.client == nullptr || (cfg.pass != nullptr && cfg.user == nullptr)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto client = strlen16(cfg.client);
        if (!client) {
            return fail(client.error());
        }

        const auto user = cfg.user != nullptr ? strlen16(cfg.user) : Result<std::uint16_t>{0U};
        if (!user) {
            return fail(user.error());
        }

        const auto pass = cfg.pass != nullptr ? strlen16(cfg.pass) : Result<std::uint16_t>{0U};
        if (!pass) {
            return fail(pass.error());
        }

        const std::size_t remaining =
            10U +
            2U + *client +
            (cfg.user != nullptr ? 2U + *user : 0U) +
            (cfg.pass != nullptr ? 2U + *pass : 0U);

        auto packet = begin(out, static_cast<std::uint8_t>(MqttType::connect), 0U, remaining);
        if (!packet) {
            return fail(packet.error());
        }

        auto frame = *packet;
        auto pos = header_bytes(remaining);

        pos += write_string(frame, pos, "MQTT");
        frame[pos++] = 4U;

        std::uint8_t flags = 0U;
        if (cfg.clean) {
            flags |= 0x02U;
        }
        if (cfg.user != nullptr) {
            flags |= 0x80U;
        }
        if (cfg.pass != nullptr) {
            flags |= 0x40U;
        }
        frame[pos++] = flags;
        pos += write_u16(frame, pos, cfg.keep_alive);
        pos += write_string(frame, pos, cfg.client);

        if (cfg.user != nullptr) {
            pos += write_string(frame, pos, cfg.user);
        }
        if (cfg.pass != nullptr) {
            pos += write_string(frame, pos, cfg.pass);
        }
        return frame.first(pos);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> publish(
        std::span<std::uint8_t> out,
        const MqttPublish& cfg) noexcept
    {
        if (cfg.topic == nullptr || (cfg.bytes != 0U && cfg.data == nullptr)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto topic = strlen16(cfg.topic);
        if (!topic) {
            return fail(topic.error());
        }
        if (*topic == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto qos = static_cast<std::uint8_t>(cfg.qos);
        if (qos > 2U || (qos != 0U && cfg.packet == 0U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t remaining = 2U + *topic + (qos != 0U ? 2U : 0U);
        if (!add_remaining(remaining, cfg.bytes)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        std::uint8_t flags = static_cast<std::uint8_t>(qos << 1U);
        if (cfg.dup) {
            flags |= 0x08U;
        }
        if (cfg.retain) {
            flags |= 0x01U;
        }

        auto packet = begin(out, static_cast<std::uint8_t>(MqttType::publish), flags, remaining);
        if (!packet) {
            return fail(packet.error());
        }

        auto frame = *packet;
        auto pos = header_bytes(remaining);
        pos += write_string(frame, pos, cfg.topic);

        if (qos != 0U) {
            pos += write_u16(frame, pos, cfg.packet);
        }
        if (cfg.bytes != 0U) {
            std::memcpy(frame.data() + pos, cfg.data, cfg.bytes);
            pos += cfg.bytes;
        }
        return frame.first(pos);
    }

    template <typename T, std::size_t Extent>
        requires MqttBytes<T>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> publish(
        std::span<std::uint8_t> out,
        const char* const topic,
        const std::span<T, Extent> data,
        const std::uint16_t packet = 0U,
        const MqttQos qos = MqttQos::at_most,
        const bool dup = false,
        const bool retain = false) noexcept
    {
        return publish(out, MqttPublish{
                                .topic = topic,
                                .data = data.data(),
                                .bytes = data.size_bytes(),
                                .packet = packet,
                                .qos = qos,
                                .dup = dup,
                                .retain = retain,
                            });
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> subscribe(
        std::span<std::uint8_t> out,
        const std::uint16_t packet,
        const std::span<const MqttSubscription> topics) noexcept
    {
        if (packet == 0U || topics.empty() || topics.data() == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t remaining = 2U;
        for (const auto& topic : topics) {
            if (topic.filter == nullptr || static_cast<std::uint8_t>(topic.qos) > 2U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto bytes = strlen16(topic.filter);
            if (!bytes) {
                return fail(bytes.error());
            }
            if (*bytes == 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (!add_remaining(remaining, 2U + *bytes + 1U)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
        }

        auto frame = begin(out, static_cast<std::uint8_t>(MqttType::subscribe), 0x02U, remaining);
        if (!frame) {
            return fail(frame.error());
        }

        auto packet_bytes = *frame;
        auto pos = header_bytes(remaining);
        pos += write_u16(packet_bytes, pos, packet);
        for (const auto& topic : topics) {
            pos += write_string(packet_bytes, pos, topic.filter);
            packet_bytes[pos++] = static_cast<std::uint8_t>(topic.qos);
        }
        return packet_bytes.first(pos);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> ping(
        std::span<std::uint8_t> out) noexcept
    {
        return fixed(out, static_cast<std::uint8_t>(MqttType::pingreq), 0U);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> disconnect(
        std::span<std::uint8_t> out) noexcept
    {
        return fixed(out, static_cast<std::uint8_t>(MqttType::disconnect), 0U);
    }

    [[nodiscard]] static Result<MqttPacket> parse(const std::span<const std::uint8_t> frame) noexcept
    {
        if (!valid_span(frame) || frame.size() < 2U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t remaining = 0U;
        std::size_t used = 0U;
        const auto err = decode_remaining(frame.subspan(1U), remaining, used);
        if (err != ESP_OK) {
            return fail(err);
        }

        const std::size_t header = 1U + used;
        const std::size_t total = header + remaining;
        if (frame.size() < total) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto byte0 = frame[0];
        const auto type = static_cast<std::uint8_t>(byte0 >> 4U);
        const auto flags = static_cast<std::uint8_t>(byte0 & 0x0fU);
        if (!valid_header(type, flags)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        return MqttPacket{
            .type = static_cast<MqttType>(type),
            .flags = flags,
            .body = frame.subspan(header, remaining),
            .bytes = total,
        };
    }

    [[nodiscard]] static Result<MqttPublishView> view(const MqttPacket& packet) noexcept
    {
        if (packet.type != MqttType::publish ||
            packet.body.size() < 2U ||
            (packet.body.size() != 0U && packet.body.data() == nullptr)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t pos = 0U;
        const auto topic = slice(packet.body, pos);
        if (!topic) {
            return fail(topic.error());
        }
        if (topic->empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        pos += 2U + topic->size();

        const auto qos = packet.qos();
        std::uint16_t id = 0U;
        if (qos != MqttQos::at_most) {
            if (pos > packet.body.size() || packet.body.size() - pos < 2U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            id = read_u16(packet.body, pos);
            pos += 2U;
        }

        return MqttPublishView{
            .topic = *topic,
            .payload = packet.body.subspan(pos),
            .packet = id,
            .qos = qos,
            .dup = packet.dup(),
            .retain = packet.retain(),
        };
    }

    [[nodiscard]] static Result<MqttConnAck> connack(const MqttPacket& packet) noexcept
    {
        if (packet.type != MqttType::connack || packet.body.size() != 2U || !valid_span(packet.body)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return MqttConnAck{
            .session = (packet.body[0] & 0x01U) != 0U,
            .code = packet.body[1],
        };
    }

    [[nodiscard]] static Result<MqttSubAck> suback(const MqttPacket& packet) noexcept
    {
        if (packet.type != MqttType::suback || packet.body.size() < 3U || !valid_span(packet.body)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return MqttSubAck{
            .packet = read_u16(packet.body, 0U),
            .codes = packet.body.subspan(2U),
        };
    }

    [[nodiscard]] static bool match(const char* filter, const char* topic) noexcept
    {
        if (filter == nullptr || topic == nullptr || filter[0] == '\0' || topic[0] == '\0') {
            return false;
        }
        if (topic[0] == '$' && filter[0] != '$') {
            return false;
        }

        const auto* const begin = filter;
        for (;;) {
            if (*filter == '#') {
                return (filter == begin || filter[-1] == '/') && filter[1] == '\0';
            }

            if (*filter == '+') {
                if ((filter != begin && filter[-1] != '/') || (filter[1] != '\0' && filter[1] != '/')) {
                    return false;
                }
                ++filter;
                while (*topic != '\0' && *topic != '/') {
                    ++topic;
                }
            } else if (*filter == *topic) {
                if (*filter == '\0') {
                    return true;
                }
                ++filter;
                ++topic;
            } else {
                return false;
            }

            if (*filter == '\0' && *topic == '\0') {
                return true;
            }
            if (*filter == '/' && filter[1] == '#' && filter[2] == '\0' && *topic == '\0') {
                return true;
            }
            if (*filter == '/' && *topic == '/') {
                ++filter;
                ++topic;
                continue;
            }
            if (*filter == '\0' || *topic == '\0') {
                return *filter == '\0' && *topic == '\0';
            }
        }
    }

private:
    [[nodiscard]] static Result<std::span<const std::uint8_t>> fixed(
        std::span<std::uint8_t> out,
        const std::uint8_t type,
        const std::uint8_t flags) noexcept
    {
        auto frame = begin(out, type, flags, 0U);
        if (!frame) {
            return fail(frame.error());
        }
        return frame->first(2U);
    }

    [[nodiscard]] static Result<std::span<std::uint8_t>> begin(
        std::span<std::uint8_t> out,
        const std::uint8_t type,
        const std::uint8_t flags,
        const std::size_t remaining) noexcept
    {
        if (!valid_span(out) || remaining > max_remaining()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto header = header_bytes(remaining);
        if (out.size() < header + remaining) {
            return fail(ESP_ERR_NO_MEM);
        }

        out[0] = static_cast<std::uint8_t>((type << 4U) | (flags & 0x0fU));
        encode_remaining(out.subspan(1U), remaining);
        return out;
    }

    [[nodiscard]] static constexpr std::size_t max_remaining() noexcept
    {
        return 268435455U;
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T, Extent> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static constexpr bool add_remaining(
        std::size_t& remaining,
        const std::size_t bytes) noexcept
    {
        if (remaining > max_remaining() || bytes > max_remaining() - remaining) {
            return false;
        }
        remaining += bytes;
        return true;
    }

    [[nodiscard]] static constexpr bool valid_header(
        const std::uint8_t type,
        const std::uint8_t flags) noexcept
    {
        switch (type) {
            case 1U:
            case 2U:
            case 4U:
            case 5U:
            case 7U:
            case 9U:
            case 11U:
            case 12U:
            case 13U:
            case 14U:
                return flags == 0U;
            case 3U:
                return ((flags >> 1U) & 0x03U) != 0x03U;
            case 6U:
            case 8U:
            case 10U:
                return flags == 0x02U;
            default:
                return false;
        }
    }

    [[nodiscard]] static constexpr std::size_t header_bytes(const std::size_t remaining) noexcept
    {
        if (remaining < 128U) {
            return 2U;
        }
        if (remaining < 16384U) {
            return 3U;
        }
        if (remaining < 2097152U) {
            return 4U;
        }
        return 5U;
    }

    static void encode_remaining(std::span<std::uint8_t> out, std::size_t remaining) noexcept
    {
        std::size_t pos = 0U;
        do {
            auto byte = static_cast<std::uint8_t>(remaining % 128U);
            remaining /= 128U;
            if (remaining != 0U) {
                byte |= 0x80U;
            }
            out[pos++] = byte;
        } while (remaining != 0U);
    }

    [[nodiscard]] static esp_err_t decode_remaining(
        const std::span<const std::uint8_t> in,
        std::size_t& remaining,
        std::size_t& used) noexcept
    {
        remaining = 0U;
        used = 0U;

        std::size_t factor = 1U;
        for (std::size_t i = 0U; i < in.size() && i < 4U; ++i) {
            const auto byte = in[i];
            remaining += static_cast<std::size_t>(byte & 0x7fU) * factor;
            used = i + 1U;

            if ((byte & 0x80U) == 0U) {
                return ESP_OK;
            }
            factor *= 128U;
        }
        return ESP_ERR_INVALID_ARG;
    }

    [[nodiscard]] static Result<std::uint16_t> strlen16(const char* const text) noexcept
    {
        if (text == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto bytes = std::strlen(text);
        if (bytes > 0xffffU) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return static_cast<std::uint16_t>(bytes);
    }

    static std::size_t write_u16(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const std::uint16_t value) noexcept
    {
        out[pos] = static_cast<std::uint8_t>(value >> 8U);
        out[pos + 1U] = static_cast<std::uint8_t>(value & 0xffU);
        return 2U;
    }

    static std::size_t write_string(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const char* const text) noexcept
    {
        const auto bytes = static_cast<std::uint16_t>(std::strlen(text));
        write_u16(out, pos, bytes);
        std::memcpy(out.data() + pos + 2U, text, bytes);
        return 2U + bytes;
    }

    [[nodiscard]] static std::uint16_t read_u16(
        const std::span<const std::uint8_t> in,
        const std::size_t pos) noexcept
    {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(in[pos]) << 8U) | in[pos + 1U]);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> slice(
        const std::span<const std::uint8_t> in,
        const std::size_t pos) noexcept
    {
        if (pos > in.size() || in.size() - pos < 2U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto bytes = read_u16(in, pos);
        const auto begin = pos + 2U;
        if (bytes > in.size() - begin) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return in.subspan(begin, bytes);
    }
};

template <typename Codec>
concept MqttAdapter = requires(
    Codec& codec,
    std::span<std::uint8_t> out,
    std::span<const std::uint8_t> frame,
    const MqttConnect& connect_cfg,
    const MqttPublish& publish_cfg,
    std::uint16_t packet,
    std::span<const MqttSubscription> topics,
    const MqttPacket& parsed,
    const char* filter,
    const char* topic) {
    { codec.connect(out, connect_cfg) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.publish(out, publish_cfg) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.subscribe(out, packet, topics) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.ping(out) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.disconnect(out) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.parse(frame) } -> std::same_as<Result<MqttPacket>>;
    { codec.view(parsed) } -> std::same_as<Result<MqttPublishView>>;
    { codec.connack(parsed) } -> std::same_as<Result<MqttConnAck>>;
    { codec.suback(parsed) } -> std::same_as<Result<MqttSubAck>>;
    { codec.match(filter, topic) } -> std::same_as<bool>;
};

struct AnyMqtt {
    using FrameFn = Result<std::span<const std::uint8_t>> (*)(void*, std::span<std::uint8_t>) noexcept;
    using ConnectFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        const MqttConnect&) noexcept;
    using PublishFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        const MqttPublish&) noexcept;
    using SubscribeFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        std::uint16_t,
        std::span<const MqttSubscription>) noexcept;
    using ParseFn = Result<MqttPacket> (*)(void*, std::span<const std::uint8_t>) noexcept;
    using ViewFn = Result<MqttPublishView> (*)(void*, const MqttPacket&) noexcept;
    using ConnAckFn = Result<MqttConnAck> (*)(void*, const MqttPacket&) noexcept;
    using SubAckFn = Result<MqttSubAck> (*)(void*, const MqttPacket&) noexcept;
    using MatchFn = bool (*)(void*, const char*, const char*) noexcept;

    void* ctx{};
    ConnectFn connect_fn{};
    PublishFn publish_fn{};
    SubscribeFn subscribe_fn{};
    FrameFn ping_fn{};
    FrameFn disconnect_fn{};
    ParseFn parse_fn{};
    ViewFn view_fn{};
    ConnAckFn connack_fn{};
    SubAckFn suback_fn{};
    MatchFn match_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return connect_fn != nullptr &&
            publish_fn != nullptr &&
            subscribe_fn != nullptr &&
            ping_fn != nullptr &&
            disconnect_fn != nullptr &&
            parse_fn != nullptr &&
            view_fn != nullptr &&
            connack_fn != nullptr &&
            suback_fn != nullptr &&
            match_fn != nullptr;
    }

    [[nodiscard]] static constexpr AnyMqtt arc() noexcept
    {
        return {
            .ctx = nullptr,
            .connect_fn = &arc_connect,
            .publish_fn = &arc_publish,
            .subscribe_fn = &arc_subscribe,
            .ping_fn = &arc_ping,
            .disconnect_fn = &arc_disconnect,
            .parse_fn = &arc_parse,
            .view_fn = &arc_view,
            .connack_fn = &arc_connack,
            .suback_fn = &arc_suback,
            .match_fn = &arc_match,
        };
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static constexpr AnyMqtt bind(Codec& codec) noexcept
    {
        return {
            .ctx = &codec,
            .connect_fn = &connect_object<Codec>,
            .publish_fn = &publish_object<Codec>,
            .subscribe_fn = &subscribe_object<Codec>,
            .ping_fn = &ping_object<Codec>,
            .disconnect_fn = &disconnect_object<Codec>,
            .parse_fn = &parse_object<Codec>,
            .view_fn = &view_object<Codec>,
            .connack_fn = &connack_object<Codec>,
            .suback_fn = &suback_object<Codec>,
            .match_fn = &match_object<Codec>,
        };
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> connect(
        std::span<std::uint8_t> out,
        const MqttConnect& cfg) noexcept
    {
        return connect_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : connect_fn(ctx, out, cfg);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> publish(
        std::span<std::uint8_t> out,
        const MqttPublish& cfg) noexcept
    {
        return publish_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : publish_fn(ctx, out, cfg);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> subscribe(
        std::span<std::uint8_t> out,
        const std::uint16_t packet,
        const std::span<const MqttSubscription> topics) noexcept
    {
        return subscribe_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : subscribe_fn(ctx, out, packet, topics);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> ping(std::span<std::uint8_t> out) noexcept
    {
        return ping_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : ping_fn(ctx, out);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> disconnect(std::span<std::uint8_t> out) noexcept
    {
        return disconnect_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : disconnect_fn(ctx, out);
    }

    [[nodiscard]] Result<MqttPacket> parse(const std::span<const std::uint8_t> frame) noexcept
    {
        return parse_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : parse_fn(ctx, frame);
    }

    [[nodiscard]] Result<MqttPublishView> view(const MqttPacket& packet) noexcept
    {
        return view_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : view_fn(ctx, packet);
    }

    [[nodiscard]] Result<MqttConnAck> connack(const MqttPacket& packet) noexcept
    {
        return connack_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : connack_fn(ctx, packet);
    }

    [[nodiscard]] Result<MqttSubAck> suback(const MqttPacket& packet) noexcept
    {
        return suback_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : suback_fn(ctx, packet);
    }

    [[nodiscard]] bool match(const char* const filter, const char* const topic) noexcept
    {
        return match_fn != nullptr && match_fn(ctx, filter, topic);
    }

private:
    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_connect(
        void*,
        const std::span<std::uint8_t> out,
        const MqttConnect& cfg) noexcept
    {
        return Mqtt::connect(out, cfg);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_publish(
        void*,
        const std::span<std::uint8_t> out,
        const MqttPublish& cfg) noexcept
    {
        return Mqtt::publish(out, cfg);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_subscribe(
        void*,
        const std::span<std::uint8_t> out,
        const std::uint16_t packet,
        const std::span<const MqttSubscription> topics) noexcept
    {
        return Mqtt::subscribe(out, packet, topics);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_ping(
        void*,
        const std::span<std::uint8_t> out) noexcept
    {
        return Mqtt::ping(out);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_disconnect(
        void*,
        const std::span<std::uint8_t> out) noexcept
    {
        return Mqtt::disconnect(out);
    }

    [[nodiscard]] static Result<MqttPacket> arc_parse(
        void*,
        const std::span<const std::uint8_t> frame) noexcept
    {
        return Mqtt::parse(frame);
    }

    [[nodiscard]] static Result<MqttPublishView> arc_view(
        void*,
        const MqttPacket& packet) noexcept
    {
        return Mqtt::view(packet);
    }

    [[nodiscard]] static Result<MqttConnAck> arc_connack(
        void*,
        const MqttPacket& packet) noexcept
    {
        return Mqtt::connack(packet);
    }

    [[nodiscard]] static Result<MqttSubAck> arc_suback(
        void*,
        const MqttPacket& packet) noexcept
    {
        return Mqtt::suback(packet);
    }

    [[nodiscard]] static bool arc_match(
        void*,
        const char* const filter,
        const char* const topic) noexcept
    {
        return Mqtt::match(filter, topic);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> connect_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const MqttConnect& cfg) noexcept
    {
        return static_cast<Codec*>(ctx)->connect(out, cfg);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> publish_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const MqttPublish& cfg) noexcept
    {
        return static_cast<Codec*>(ctx)->publish(out, cfg);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> subscribe_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const std::uint16_t packet,
        const std::span<const MqttSubscription> topics) noexcept
    {
        return static_cast<Codec*>(ctx)->subscribe(out, packet, topics);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> ping_object(
        void* const ctx,
        const std::span<std::uint8_t> out) noexcept
    {
        return static_cast<Codec*>(ctx)->ping(out);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> disconnect_object(
        void* const ctx,
        const std::span<std::uint8_t> out) noexcept
    {
        return static_cast<Codec*>(ctx)->disconnect(out);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<MqttPacket> parse_object(
        void* const ctx,
        const std::span<const std::uint8_t> frame) noexcept
    {
        return static_cast<Codec*>(ctx)->parse(frame);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<MqttPublishView> view_object(
        void* const ctx,
        const MqttPacket& packet) noexcept
    {
        return static_cast<Codec*>(ctx)->view(packet);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<MqttConnAck> connack_object(
        void* const ctx,
        const MqttPacket& packet) noexcept
    {
        return static_cast<Codec*>(ctx)->connack(packet);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static Result<MqttSubAck> suback_object(
        void* const ctx,
        const MqttPacket& packet) noexcept
    {
        return static_cast<Codec*>(ctx)->suback(packet);
    }

    template <MqttAdapter Codec>
    [[nodiscard]] static bool match_object(
        void* const ctx,
        const char* const filter,
        const char* const topic) noexcept
    {
        return static_cast<Codec*>(ctx)->match(filter, topic);
    }
};

class MqttSession {
public:
    constexpr explicit MqttSession(const std::uint16_t keep_alive_s = 60U) noexcept
        : keep_alive_s_(keep_alive_s)
    {
    }

    constexpr void reset() noexcept
    {
        state_ = MqttSessionState::closed;
        next_packet_ = 1U;
        last_tx_ms_ = 0U;
        last_rx_ms_ = 0U;
        awaiting_ping_ = false;
    }

    constexpr void connect_started(
        const std::uint64_t now_ms,
        const std::uint16_t keep_alive_s) noexcept
    {
        state_ = MqttSessionState::connecting;
        keep_alive_s_ = keep_alive_s;
        last_tx_ms_ = now_ms;
        last_rx_ms_ = now_ms;
        awaiting_ping_ = false;
    }

    constexpr void connect_started(
        const std::uint64_t now_ms,
        const MqttConnect& cfg) noexcept
    {
        connect_started(now_ms, cfg.keep_alive);
    }

    [[nodiscard]] constexpr MqttSessionState state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] constexpr bool online() const noexcept
    {
        return state_ == MqttSessionState::online;
    }

    [[nodiscard]] constexpr bool awaiting_ping() const noexcept
    {
        return awaiting_ping_;
    }

    [[nodiscard]] constexpr std::uint16_t keep_alive() const noexcept
    {
        return keep_alive_s_;
    }

    [[nodiscard]] constexpr std::uint16_t next_packet() noexcept
    {
        const auto out = next_packet_;
        next_packet_ = static_cast<std::uint16_t>(next_packet_ + 1U);
        if (next_packet_ == 0U) {
            next_packet_ = 1U;
        }
        return out;
    }

    constexpr void sent(const std::uint64_t now_ms) noexcept
    {
        last_tx_ms_ = now_ms;
    }

    constexpr void received(const std::uint64_t now_ms) noexcept
    {
        last_rx_ms_ = now_ms;
    }

    [[nodiscard]] constexpr bool ping_due(const std::uint64_t now_ms) const noexcept
    {
        return online() && keep_alive_s_ != 0U && !awaiting_ping_ &&
            elapsed(last_tx_ms_, now_ms) >= alive_ms();
    }

    [[nodiscard]] constexpr bool expired(const std::uint64_t now_ms) const noexcept
    {
        return online() && keep_alive_s_ != 0U &&
            elapsed(last_rx_ms_, now_ms) >= broker_ms();
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> ping(
        std::span<std::uint8_t> out,
        const std::uint64_t now_ms) noexcept
    {
        if (!ping_due(now_ms)) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        auto frame = Mqtt::ping(out);
        if (!frame) {
            return fail(frame.error());
        }
        sent(now_ms);
        awaiting_ping_ = true;
        return frame;
    }

    [[nodiscard]] esp_err_t observe(const MqttPacket& packet, const std::uint64_t now_ms) noexcept
    {
        received(now_ms);
        switch (packet.type) {
            case MqttType::connack: {
                const auto ack = Mqtt::connack(packet);
                if (!ack) {
                    state_ = MqttSessionState::closed;
                    return ack.error();
                }
                if (ack->code != 0U) {
                    state_ = MqttSessionState::closed;
                    return ESP_FAIL;
                }
                state_ = MqttSessionState::online;
                awaiting_ping_ = false;
                return ESP_OK;
            }
            case MqttType::pingresp:
                awaiting_ping_ = false;
                return ESP_OK;
            case MqttType::suback:
                return Mqtt::suback(packet).has_value() ? ESP_OK : ESP_ERR_INVALID_ARG;
            case MqttType::puback:
                return packet.body.size() == 2U ? ESP_OK : ESP_ERR_INVALID_ARG;
            default:
                return ESP_OK;
        }
    }

private:
    [[nodiscard]] constexpr std::uint64_t alive_ms() const noexcept
    {
        return static_cast<std::uint64_t>(keep_alive_s_) * 1000ULL;
    }

    [[nodiscard]] constexpr std::uint64_t broker_ms() const noexcept
    {
        return alive_ms() + (alive_ms() / 2U);
    }

    [[nodiscard]] static constexpr std::uint64_t elapsed(
        const std::uint64_t since_ms,
        const std::uint64_t now_ms) noexcept
    {
        return now_ms - since_ms;
    }

    MqttSessionState state_{MqttSessionState::closed};
    std::uint16_t next_packet_{1U};
    std::uint16_t keep_alive_s_{60U};
    std::uint64_t last_tx_ms_{};
    std::uint64_t last_rx_ms_{};
    bool awaiting_ping_{};
};

}  // namespace arc::net
