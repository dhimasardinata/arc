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

using BoundPlane = arc::Plane<Bound, 1024, Shared>;
using BarePlane = arc::Plane<Bare, 1024>;
using App = arc::App<Loop, 1024>;

static_assert(requires { BarePlane::boot("arc"); });
static_assert(requires { BoundPlane::template boot<&shared>("arc"); });
static_assert(requires { App::boot("arc"); });

}  // namespace
