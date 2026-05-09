#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::usb {

enum class DescriptorType : std::uint8_t {
    device = 1U,
    configuration = 2U,
    string = 3U,
    interface = 4U,
    endpoint = 5U,
};

enum class Class : std::uint8_t {
    communications = 0x02U,
    audio = 0x01U,
    cdc_data = 0x0aU,
    video = 0x0eU,
    vendor = 0xffU,
};

enum class EndpointAttr : std::uint8_t {
    control = 0x00U,
    isochronous = 0x01U,
    bulk = 0x02U,
    interrupt = 0x03U,
};

enum class StandardRequest : std::uint8_t {
    set_address = 5U,
    get_descriptor = 6U,
    set_configuration = 9U,
};

enum class DeviceState : std::uint8_t {
    detached,
    powered,
    addressed,
    configured,
};

struct SetupPacket {
    std::uint8_t request_type{};
    std::uint8_t request{};
    std::uint16_t value{};
    std::uint16_t index{};
    std::uint16_t length{};
};

struct DeviceDescriptor {
    std::uint16_t usb{0x0200U};
    Class klass{Class::vendor};
    std::uint8_t subclass{};
    std::uint8_t protocol{};
    std::uint8_t max_packet0{64U};
    std::uint16_t vendor{};
    std::uint16_t product{};
    std::uint16_t device{0x0100U};
    std::uint8_t manufacturer{};
    std::uint8_t product_string{};
    std::uint8_t serial{};
    std::uint8_t configurations{1U};

    [[nodiscard]] constexpr std::array<std::uint8_t, 18> bytes() const noexcept
    {
        return {
            18U,
            static_cast<std::uint8_t>(DescriptorType::device),
            lo(usb),
            hi(usb),
            static_cast<std::uint8_t>(klass),
            subclass,
            protocol,
            max_packet0,
            lo(vendor),
            hi(vendor),
            lo(product),
            hi(product),
            lo(device),
            hi(device),
            manufacturer,
            product_string,
            serial,
            configurations,
        };
    }

private:
    [[nodiscard]] static constexpr std::uint8_t lo(const std::uint16_t value) noexcept
    {
        return static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] static constexpr std::uint8_t hi(const std::uint16_t value) noexcept
    {
        return static_cast<std::uint8_t>(value >> 8U);
    }
};

struct ConfigurationDescriptor {
    std::uint16_t total_length{};
    std::uint8_t interfaces{1U};
    std::uint8_t configuration{1U};
    std::uint8_t string{};
    std::uint8_t attributes{0x80U};
    std::uint8_t max_power_ma{100U};

    [[nodiscard]] constexpr std::array<std::uint8_t, 9> bytes() const noexcept
    {
        return {
            9U,
            static_cast<std::uint8_t>(DescriptorType::configuration),
            lo(total_length),
            hi(total_length),
            interfaces,
            configuration,
            string,
            attributes,
            static_cast<std::uint8_t>(max_power_ma / 2U),
        };
    }

private:
    [[nodiscard]] static constexpr std::uint8_t lo(const std::uint16_t value) noexcept
    {
        return static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] static constexpr std::uint8_t hi(const std::uint16_t value) noexcept
    {
        return static_cast<std::uint8_t>(value >> 8U);
    }
};

struct InterfaceDescriptor {
    std::uint8_t number{};
    std::uint8_t alternate{};
    std::uint8_t endpoints{};
    Class klass{Class::vendor};
    std::uint8_t subclass{};
    std::uint8_t protocol{};
    std::uint8_t string{};

    [[nodiscard]] constexpr std::array<std::uint8_t, 9> bytes() const noexcept
    {
        return {
            9U,
            static_cast<std::uint8_t>(DescriptorType::interface),
            number,
            alternate,
            endpoints,
            static_cast<std::uint8_t>(klass),
            subclass,
            protocol,
            string,
        };
    }
};

struct EndpointDescriptor {
    std::uint8_t address{};
    EndpointAttr attributes{EndpointAttr::bulk};
    std::uint16_t max_packet{64U};
    std::uint8_t interval{};

    [[nodiscard]] constexpr std::array<std::uint8_t, 7> bytes() const noexcept
    {
        return {
            7U,
            static_cast<std::uint8_t>(DescriptorType::endpoint),
            address,
            static_cast<std::uint8_t>(attributes),
            lo(max_packet),
            hi(max_packet),
            interval,
        };
    }

private:
    [[nodiscard]] static constexpr std::uint8_t lo(const std::uint16_t value) noexcept
    {
        return static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] static constexpr std::uint8_t hi(const std::uint16_t value) noexcept
    {
        return static_cast<std::uint8_t>(value >> 8U);
    }
};

template <std::size_t A, std::size_t B>
[[nodiscard]] consteval auto concat(
    const std::array<std::uint8_t, A> lhs,
    const std::array<std::uint8_t, B> rhs) noexcept
{
    std::array<std::uint8_t, A + B> out{};
    for (std::size_t i = 0; i < A; ++i) {
        out[i] = lhs[i];
    }
    for (std::size_t i = 0; i < B; ++i) {
        out[A + i] = rhs[i];
    }
    return out;
}

template <std::uint8_t OutEndpoint, std::uint8_t InEndpoint, std::uint16_t PacketBytes = 64U>
struct Bulk {
    static_assert((OutEndpoint & 0x80U) == 0U, "bulk OUT endpoint must not set the IN bit");
    static_assert((InEndpoint & 0x80U) != 0U, "bulk IN endpoint must set the IN bit");
    static_assert(PacketBytes != 0U, "bulk packet size must be non-zero");

    [[nodiscard]] static consteval auto descriptors(const std::uint8_t interface = 0U) noexcept
    {
        constexpr auto total = 9U + 9U + 7U + 7U;
        const auto cfg = ConfigurationDescriptor{
            .total_length = total,
            .interfaces = 1U,
        }
                             .bytes();
        const auto ifc = InterfaceDescriptor{
            .number = interface,
            .endpoints = 2U,
            .klass = Class::vendor,
        }
                             .bytes();
        const auto out = EndpointDescriptor{
            .address = OutEndpoint,
            .attributes = EndpointAttr::bulk,
            .max_packet = PacketBytes,
        }
                             .bytes();
        const auto in = EndpointDescriptor{
            .address = InEndpoint,
            .attributes = EndpointAttr::bulk,
            .max_packet = PacketBytes,
        }
                            .bytes();
        return concat(concat(concat(cfg, ifc), out), in);
    }
};

template <std::uint8_t NotifyEndpoint, std::uint8_t OutEndpoint, std::uint8_t InEndpoint, std::uint16_t PacketBytes = 64U>
struct Cdc {
    static_assert((NotifyEndpoint & 0x80U) != 0U, "CDC notify endpoint must be IN");
    static_assert((OutEndpoint & 0x80U) == 0U, "CDC OUT endpoint must not set the IN bit");
    static_assert((InEndpoint & 0x80U) != 0U, "CDC IN endpoint must set the IN bit");

    [[nodiscard]] static consteval auto descriptors(const std::uint8_t control = 0U, const std::uint8_t data = 1U) noexcept
    {
        constexpr auto total = 9U + 9U + 7U + 9U + 7U + 7U;
        const auto cfg = ConfigurationDescriptor{
            .total_length = total,
            .interfaces = 2U,
        }
                             .bytes();
        const auto ctrl = InterfaceDescriptor{
            .number = control,
            .endpoints = 1U,
            .klass = Class::communications,
            .subclass = 2U,
            .protocol = 1U,
        }
                              .bytes();
        const auto notify = EndpointDescriptor{
            .address = NotifyEndpoint,
            .attributes = EndpointAttr::interrupt,
            .max_packet = 16U,
            .interval = 8U,
        }
                                .bytes();
        const auto data_ifc = InterfaceDescriptor{
            .number = data,
            .endpoints = 2U,
            .klass = Class::cdc_data,
        }
                                  .bytes();
        const auto out = EndpointDescriptor{
            .address = OutEndpoint,
            .attributes = EndpointAttr::bulk,
            .max_packet = PacketBytes,
        }
                             .bytes();
        const auto in = EndpointDescriptor{
            .address = InEndpoint,
            .attributes = EndpointAttr::bulk,
            .max_packet = PacketBytes,
        }
                            .bytes();
        return concat(concat(concat(concat(concat(cfg, ctrl), notify), data_ifc), out), in);
    }
};

template <std::uint8_t InEndpoint, std::uint16_t PacketBytes = 512U>
struct Uvc {
    static_assert((InEndpoint & 0x80U) != 0U, "UVC endpoint must be IN");

    [[nodiscard]] static consteval auto descriptors(const std::uint8_t interface = 0U) noexcept
    {
        constexpr auto total = 9U + 9U + 7U;
        const auto cfg = ConfigurationDescriptor{
            .total_length = total,
            .interfaces = 1U,
            .max_power_ma = 250U,
        }
                             .bytes();
        const auto ifc = InterfaceDescriptor{
            .number = interface,
            .endpoints = 1U,
            .klass = Class::video,
            .subclass = 2U,
        }
                             .bytes();
        const auto in = EndpointDescriptor{
            .address = InEndpoint,
            .attributes = EndpointAttr::isochronous,
            .max_packet = PacketBytes,
            .interval = 1U,
        }
                            .bytes();
        return concat(concat(cfg, ifc), in);
    }
};

template <std::uint8_t InEndpoint, std::uint16_t PacketBytes = 192U>
struct Uac {
    static_assert((InEndpoint & 0x80U) != 0U, "UAC endpoint must be IN");

    [[nodiscard]] static consteval auto descriptors(const std::uint8_t interface = 0U) noexcept
    {
        constexpr auto total = 9U + 9U + 7U;
        const auto cfg = ConfigurationDescriptor{
            .total_length = total,
            .interfaces = 1U,
            .max_power_ma = 100U,
        }
                             .bytes();
        const auto ifc = InterfaceDescriptor{
            .number = interface,
            .endpoints = 1U,
            .klass = Class::audio,
            .subclass = 2U,
        }
                             .bytes();
        const auto in = EndpointDescriptor{
            .address = InEndpoint,
            .attributes = EndpointAttr::isochronous,
            .max_packet = PacketBytes,
            .interval = 1U,
        }
                            .bytes();
        return concat(concat(cfg, ifc), in);
    }
};

template <typename T>
concept FifoLane = requires(T& lane, std::uint8_t value) {
    { lane.try_push(value) } -> std::same_as<bool>;
    { lane.try_pop(value) } -> std::same_as<bool>;
};

template <FifoLane Lane>
struct Fifo {
    [[nodiscard]] static Status write(Lane& lane, const std::span<const std::uint8_t> data) noexcept
    {
        for (const auto byte : data) {
            if (!lane.try_push(byte)) {
                return Status{fail(ESP_ERR_NO_MEM)};
            }
        }
        return ok();
    }

    [[nodiscard]] static Result<std::size_t> read(Lane& lane, const std::span<std::uint8_t> out) noexcept
    {
        std::size_t count{};
        for (auto& byte : out) {
            if (!lane.try_pop(byte)) {
                break;
            }
            ++count;
        }
        return count;
    }
};

template <typename ClassDriver, typename Policy>
struct Device {
    DeviceState state{DeviceState::powered};
    std::uint8_t address{};
    std::uint8_t configuration{};
    DeviceDescriptor device{};

    [[nodiscard]] Status setup(const SetupPacket packet) noexcept
    {
        const auto request = static_cast<StandardRequest>(packet.request);
        switch (request) {
            case StandardRequest::get_descriptor:
                return descriptor(packet.value, packet.length);
            case StandardRequest::set_address:
                address = static_cast<std::uint8_t>(packet.value & 0x7fU);
                state = address == 0U ? DeviceState::powered : DeviceState::addressed;
                return status(Policy::set_address(address));
            case StandardRequest::set_configuration:
                configuration = static_cast<std::uint8_t>(packet.value & 0xffU);
                state = configuration == 0U ? DeviceState::addressed : DeviceState::configured;
                return status(Policy::configured(configuration));
            default:
                return Status{fail(ESP_ERR_NOT_SUPPORTED)};
        }
    }

private:
    [[nodiscard]] Status descriptor(const std::uint16_t value, const std::uint16_t length) noexcept
    {
        const auto type = static_cast<DescriptorType>(value >> 8U);
        if (type == DescriptorType::device) {
            const auto bytes = device.bytes();
            const auto count = length < bytes.size() ? length : bytes.size();
            return status(Policy::ep0_write(std::span<const std::uint8_t>{bytes.data(), count}));
        }
        if (type == DescriptorType::configuration) {
            constexpr auto bytes = ClassDriver::descriptors();
            const auto count = length < bytes.size() ? length : bytes.size();
            return status(Policy::ep0_write(std::span<const std::uint8_t>{bytes.data(), count}));
        }
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
};

}  // namespace arc::usb
