#pragma once

#include <utility>

#include "esp_err.h"
#include "esp_etm.h"

#include "arc/result.hpp"

namespace arc {

class EtmEvent {
public:
    constexpr EtmEvent() noexcept = default;

    explicit constexpr EtmEvent(const esp_etm_event_handle_t event) noexcept
        : event_(event)
    {
    }

    EtmEvent(const EtmEvent&) = delete;
    EtmEvent& operator=(const EtmEvent&) = delete;

    constexpr EtmEvent(EtmEvent&& other) noexcept
        : event_(std::exchange(other.event_, nullptr))
    {
    }

    EtmEvent& operator=(EtmEvent&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(close());
            event_ = std::exchange(other.event_, nullptr);
        }
        return *this;
    }

    ~EtmEvent()
    {
        static_cast<void>(close());
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return event_ != nullptr;
    }

    [[nodiscard]] constexpr esp_etm_event_handle_t native() const noexcept
    {
        return event_;
    }

    [[nodiscard]] esp_err_t close() noexcept
    {
        if (event_ == nullptr) {
            return ESP_OK;
        }

        const auto err = esp_etm_del_event(event_);
        if (err == ESP_OK) {
            event_ = nullptr;
        }
        return err;
    }

private:
    esp_etm_event_handle_t event_{};
};

class EtmTask {
public:
    constexpr EtmTask() noexcept = default;

    explicit constexpr EtmTask(const esp_etm_task_handle_t task) noexcept
        : task_(task)
    {
    }

    EtmTask(const EtmTask&) = delete;
    EtmTask& operator=(const EtmTask&) = delete;

    constexpr EtmTask(EtmTask&& other) noexcept
        : task_(std::exchange(other.task_, nullptr))
    {
    }

    EtmTask& operator=(EtmTask&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(close());
            task_ = std::exchange(other.task_, nullptr);
        }
        return *this;
    }

    ~EtmTask()
    {
        static_cast<void>(close());
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return task_ != nullptr;
    }

    [[nodiscard]] constexpr esp_etm_task_handle_t native() const noexcept
    {
        return task_;
    }

    [[nodiscard]] esp_err_t close() noexcept
    {
        if (task_ == nullptr) {
            return ESP_OK;
        }

        const auto err = esp_etm_del_task(task_);
        if (err == ESP_OK) {
            task_ = nullptr;
        }
        return err;
    }

private:
    esp_etm_task_handle_t task_{};
};

class Etm {
public:
    constexpr Etm() noexcept = default;

    explicit constexpr Etm(const esp_etm_channel_handle_t channel) noexcept
        : channel_(channel)
    {
    }

    Etm(const Etm&) = delete;
    Etm& operator=(const Etm&) = delete;

    constexpr Etm(Etm&& other) noexcept
        : channel_(std::exchange(other.channel_, nullptr))
    {
    }

    Etm& operator=(Etm&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(close());
            channel_ = std::exchange(other.channel_, nullptr);
        }
        return *this;
    }

    ~Etm()
    {
        static_cast<void>(close());
    }

    [[nodiscard]] static Result<Etm> make(const esp_etm_channel_config_t& cfg = {}) noexcept
    {
        esp_etm_channel_handle_t channel{};
        const auto err = esp_etm_new_channel(&cfg, &channel);
        if (err != ESP_OK) {
            return fail(err);
        }
        return Etm{channel};
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return channel_ != nullptr;
    }

    [[nodiscard]] constexpr esp_etm_channel_handle_t native() const noexcept
    {
        return channel_;
    }

    [[nodiscard]] Status connect(
        const esp_etm_event_handle_t event,
        const esp_etm_task_handle_t task) noexcept
    {
        if (channel_ == nullptr || event == nullptr || task == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(esp_etm_channel_connect(channel_, event, task));
    }

    [[nodiscard]] Status connect(const EtmEvent& event, const EtmTask& task) noexcept
    {
        return connect(event.native(), task.native());
    }

    [[nodiscard]] Status disconnect() noexcept
    {
        if (channel_ == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(esp_etm_channel_connect(channel_, nullptr, nullptr));
    }

    [[nodiscard]] Status enable() noexcept
    {
        if (channel_ == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(esp_etm_channel_enable(channel_));
    }

    [[nodiscard]] Status disable() noexcept
    {
        if (channel_ == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(esp_etm_channel_disable(channel_));
    }

    [[nodiscard]] esp_err_t close() noexcept
    {
        if (channel_ == nullptr) {
            return ESP_OK;
        }

        static_cast<void>(esp_etm_channel_disable(channel_));
        static_cast<void>(esp_etm_channel_connect(channel_, nullptr, nullptr));
        const auto err = esp_etm_del_channel(channel_);
        if (err == ESP_OK) {
            channel_ = nullptr;
        }
        return err;
    }

private:
    esp_etm_channel_handle_t channel_{};
};

}  // namespace arc
