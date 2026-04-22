#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_err.h"

#include "arc/result.hpp"

namespace arc {

template <typename T>
concept FileBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

struct File {
    constexpr File() noexcept = default;

    explicit File(std::FILE* const raw) noexcept
        : raw_(raw)
    {
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept
        : raw_(std::exchange(other.raw_, nullptr))
    {
    }

    File& operator=(File&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(close());
            raw_ = std::exchange(other.raw_, nullptr);
        }
        return *this;
    }

    ~File()
    {
        static_cast<void>(close());
    }

    [[nodiscard]] static Result<File> open(const char* const path, const char* const mode) noexcept
    {
        if (path == nullptr || mode == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::FILE* const file = std::fopen(path, mode);
        if (file == nullptr) {
            return fail(last_error());
        }

        return File{file};
    }

    [[nodiscard]] Result<std::size_t> read(void* const data, const std::size_t bytes) noexcept
    {
        if (raw_ == nullptr || data == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto got = std::fread(data, 1U, bytes, raw_);
        if (got != bytes && std::ferror(raw_) != 0) {
            return fail(last_error());
        }

        return got;
    }

    template <typename T>
        requires(FileBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] Result<std::size_t> read(const std::span<T> data) noexcept
    {
        return read(data.data(), data.size_bytes());
    }

    [[nodiscard]] Result<std::size_t> write(const void* const data, const std::size_t bytes) noexcept
    {
        if (raw_ == nullptr || data == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto sent = std::fwrite(data, 1U, bytes, raw_);
        if (sent != bytes && std::ferror(raw_) != 0) {
            return fail(last_error());
        }

        return sent;
    }

    template <typename T>
        requires FileBytes<T>
    [[nodiscard]] Result<std::size_t> write(const std::span<T> data) noexcept
    {
        return write(data.data(), data.size_bytes());
    }

    [[nodiscard]] esp_err_t flush() noexcept
    {
        if (raw_ == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return std::fflush(raw_) == 0 ? ESP_OK : last_error();
    }

    [[nodiscard]] esp_err_t seek(const long offset, const int origin = SEEK_SET) noexcept
    {
        if (raw_ == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return std::fseek(raw_, offset, origin) == 0 ? ESP_OK : last_error();
    }

    [[nodiscard]] Result<long> tell() noexcept
    {
        if (raw_ == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto pos = std::ftell(raw_);
        if (pos < 0) {
            return fail(last_error());
        }
        return pos;
    }

    [[nodiscard]] esp_err_t close() noexcept
    {
        if (raw_ == nullptr) {
            return ESP_OK;
        }

        auto* const file = std::exchange(raw_, nullptr);
        return std::fclose(file) == 0 ? ESP_OK : last_error();
    }

    [[nodiscard]] std::FILE* native() const noexcept
    {
        return raw_;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return raw_ != nullptr;
    }

private:
    [[nodiscard]] static esp_err_t last_error() noexcept
    {
        switch (errno) {
        case 0:
            return ESP_FAIL;
        case EINVAL:
            return ESP_ERR_INVALID_ARG;
        case ENOENT:
            return ESP_ERR_NOT_FOUND;
        case ENOMEM:
            return ESP_ERR_NO_MEM;
        default:
            return ESP_FAIL;
        }
    }

    std::FILE* raw_{nullptr};
};

}  // namespace arc
