#include "arc/concepts.hpp"
#include "arc/plane.hpp"
#include "arc/sketch.hpp"

namespace {

struct Shared {
    std::uint32_t value;
};

constinit Shared shared{};

struct Bound {
    static void setup(Shared&) {}
    static void run(Shared&) noexcept {}
};

struct Bare {
    static void setup() {}
    static void run() noexcept {}
};

struct Loop {
    static void setup() {}
    IRAM_ATTR static void loop() noexcept {}
};

struct MockWave {
    struct Config {
        std::uint32_t hz;
        std::uint32_t duty;
    };

    static constexpr Config defaults() noexcept
    {
        return Config{1'000U, 500U};
    }

    static Config config() noexcept
    {
        return defaults();
    }

    static std::uint32_t hz() noexcept
    {
        return 1'000U;
    }

    static esp_err_t hz(std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    static std::uint32_t permille() noexcept
    {
        return 500U;
    }

    static esp_err_t set(const Config&) noexcept
    {
        return ESP_OK;
    }

    static void start() noexcept {}
    static void start(const Config&) noexcept {}

    static esp_err_t duty(std::uint32_t) noexcept
    {
        return ESP_OK;
    }
};

using BoundPlane = arc::Plane<Bound, 1024, Shared>;
using BarePlane = arc::Plane<Bare, 1024>;
using App = arc::App<Loop, 1024>;

static_assert(requires { BarePlane::boot("arc"); });
static_assert(requires { BoundPlane::template boot<&shared>("arc"); });
static_assert(requires { App::boot("arc"); });
static_assert(arc::WaveConfig<MockWave>);
static_assert(arc::ConfigWave<MockWave>);
static_assert(arc::DutyWave<MockWave>);
static_assert(arc::RateWave<MockWave>);

}  // namespace
