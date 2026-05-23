#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "arc/pipeline.hpp"
#include "arc/result.hpp"

namespace arc {

struct AxiPort {
    DmaEndpoint dma{};
    std::uint32_t signal{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return static_cast<bool>(dma);
    }
};

struct AxiEdge {
    AxiPort from{};
    AxiPort to{};
    std::size_t bytes{};
};

template <std::size_t Edges>
struct AxiPlan {
    static_assert(Edges > 0U,
                  "[ARC ERROR] arc::AxiPlan needs at least one edge. Action: use AxiGraph with source and sink nodes.");

    std::array<AxiEdge, Edges> edges{};
    std::size_t bytes{};
};

template <typename T>
concept AxiNode = requires(T& node) {
    { node.input() } -> std::same_as<AxiPort>;
    { node.output() } -> std::same_as<AxiPort>;
};

template <AxiNode... Nodes>
struct AxiGraph {
    static_assert(sizeof...(Nodes) > 1U,
                  "[ARC ERROR] arc::AxiGraph needs at least source and sink nodes. Action: pass two or more node types.");

    static constexpr std::size_t nodes = sizeof...(Nodes);
    static constexpr std::size_t edges = nodes - 1U;

    using Plan = AxiPlan<edges>;

    [[nodiscard]] static Result<Plan> plan(Nodes&... items) noexcept
    {
        const std::array<AxiPort, nodes> inputs{items.input()...};
        const std::array<AxiPort, nodes> outputs{items.output()...};
        Plan out{};

        for (std::size_t i = 0; i < edges; ++i) {
            const auto from = outputs[i];
            const auto to = inputs[i + 1U];
            if (!from || !to) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto bytes = from.dma.bytes < to.dma.bytes ? from.dma.bytes : to.dma.bytes;
            if (bytes == 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (out.bytes > std::numeric_limits<std::size_t>::max() - bytes) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            out.edges[i] = AxiEdge{
                .from = from,
                .to = to,
                .bytes = bytes,
            };
            out.bytes += bytes;
        }

        return ok(out);
    }

    [[nodiscard]] static Status link(Nodes&... items) noexcept
    {
        auto built = plan(items...);
        if (!built) {
            return Status{fail(built.error())};
        }
        return link(*built);
    }

    template <typename Policy>
    [[nodiscard]] static Status boot(Policy& policy, Nodes&... items) noexcept
    {
        auto built = plan(items...);
        if (!built) {
            return Status{fail(built.error())};
        }
        auto ready = link(*built);
        if (!ready) {
            return ready;
        }

        for (std::size_t i = 0; i < edges; ++i) {
            if (const auto err = bind(policy, i, built->edges[i]); err != ESP_OK) {
                return Status{fail(err)};
            }
        }

        return arm(policy, items...);
    }

private:
    [[nodiscard]] static Status link(Plan& built) noexcept
    {
        for (auto& edge : built.edges) {
            if (edge.from.dma.tail == nullptr || edge.to.dma.head == nullptr) {
                return Status{fail(ESP_ERR_INVALID_ARG)};
            }
            edge.from.dma.tail->next = edge.to.dma.head;
        }
        return ok();
    }

    template <typename Policy>
    [[nodiscard]] static esp_err_t bind(Policy& policy, const std::size_t index, const AxiEdge& edge) noexcept
    {
        if constexpr (requires { { policy.link(index, edge) } -> std::same_as<esp_err_t>; }) {
            return policy.link(index, edge);
        } else {
            static_cast<void>(policy);
            static_cast<void>(index);
            static_cast<void>(edge);
            return ESP_OK;
        }
    }

    template <typename Policy, typename Node>
    [[nodiscard]] static Status arm_one(Policy& policy, const std::size_t index, Node& node) noexcept
    {
        if constexpr (requires { { policy.arm(index, node) } -> std::same_as<esp_err_t>; }) {
            return status(policy.arm(index, node));
        } else {
            static_cast<void>(policy);
            static_cast<void>(index);
            static_cast<void>(node);
            return ok();
        }
    }

    template <typename Policy>
    [[nodiscard]] static Status arm(Policy& policy, Nodes&... items) noexcept
    {
        Status ready = ok();
        std::size_t index{};
        auto one = [&](auto& item) noexcept {
            if (ready) {
                ready = arm_one(policy, index, item);
            }
            ++index;
        };
        (one(items), ...);
        return ready;
    }
};

}  // namespace arc
