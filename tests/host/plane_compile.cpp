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

struct Budgeted {
    static constexpr std::size_t stack_bytes = arc::stack::budget<1536U>();

    static void setup() {}
    static void run() noexcept {}
};

struct Loop {
    static constexpr std::size_t stack_bytes = arc::stack::budget<512U>();

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

using BoundPlane = arc::Plane<Bound, 2048, Shared>;
using SharedCell = arc::StaticRef<&shared, arc::Core::core1>;
using StaticBoundPlane = arc::StaticPlane<Bound, SharedCell, 2048>;
using BarePlane = arc::Plane<Bare, 2048>;
using BudgetedPlane = arc::Plane<Budgeted, Budgeted::stack_bytes>;
using App = arc::App<Loop, arc::stack::required<Loop>()>;

static_assert(requires { BarePlane::boot("arc"); });
static_assert(requires { BoundPlane::template boot<&shared>("arc"); });
static_assert(requires { StaticBoundPlane::boot("arc"); });
static_assert(StaticBoundPlane::core == arc::Core::core1);
static_assert(StaticBoundPlane::stack_required == BoundPlane::stack_required);
static_assert(BudgetedPlane::stack_required == Budgeted::stack_bytes);
static_assert(App::stack_required == arc::stack::task_floor);
static_assert(arc::TaskMem<2048>::required == arc::stack::task_floor);
static_assert(!arc::stack::fits<1024, arc::stack::task_floor>());
static_assert(requires { App::boot("arc"); });
static_assert(arc::WaveConfig<MockWave>);
static_assert(arc::ConfigWave<MockWave>);
static_assert(arc::DutyWave<MockWave>);
static_assert(arc::RateWave<MockWave>);

}  // namespace
