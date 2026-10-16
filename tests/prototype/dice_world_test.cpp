// The dice simulation (src/app/prototype/shake/sim) against physics that can be
// worked out on paper: free fall, a box that accelerates, a spinning frame, a
// die that slides to a stop, one that bounces, two that collide, a stack, a
// tipped die; and against the things a scene needs to be true: nothing leaves
// the box, nothing sinks into anything, a replay is the same replay, and the
// guide always brings every die home.

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

#include "dice_world.hpp"

namespace {

using namespace wjs::dice;
using Catch::Approx;

constexpr double kTwoPi = 2.0 * std::numbers::pi;

// The physics as it works out on paper: no two dice alike would only blur it.
Tuning plain() {
    Tuning t;
    t.variety = 0.0;
    return t;
}

// A box for one or a few dice, roomy, with the lid out of the way unless asked.
World roomy(int rows, int columns, double slack = 8.0, double lid = 100.0, std::uint32_t seed = 1) {
    Box box;
    box.rows = rows;
    box.columns = columns;
    box.slack = slack;
    box.lidHeight = lid;
    return World(box, plain(), seed);
}

Tuning airless() {
    Tuning t = plain();
    t.airDrag = 0.0;
    t.airSpinDrag = 0.0;
    return t;
}

World roomy(const Tuning& tuning, int rows = 1, int columns = 1, double slack = 8.0,
            double lid = 100.0) {
    Box box;
    box.rows = rows;
    box.columns = columns;
    box.slack = slack;
    box.lidHeight = lid;
    return World(box, tuning, 1);
}

void run(World& world, double seconds, const Reading& reading = Reading{}) {
    world.setReading(reading);
    world.advance(seconds);
}

Quat about(Vec3 axis, double angle) {
    const Vec3 a = normalized(axis);
    const double s = std::sin(angle / 2.0);
    return Quat{std::cos(angle / 2.0), a.x * s, a.y * s, a.z * s};
}

// How far apart two orientations are, in radians.
double angleBetween(Quat a, Quat b) {
    const Quat d = a * conjugate(b);
    return 2.0 * std::acos(std::min(1.0, std::abs(d.w)));
}

double lowestCorner(const Die& d) {
    double lowest = 1e9;
    const auto e = axesOf(d.q);
    for (int corner = 0; corner < 8; ++corner) {
        const Vec3 p = d.x + e[0] * ((corner & 1) != 0 ? 0.5 : -0.5) +
                       e[1] * ((corner & 2) != 0 ? 0.5 : -0.5) +
                       e[2] * ((corner & 4) != 0 ? 0.5 : -0.5);
        lowest = std::min(lowest, p.z);
    }
    return lowest;
}

// A hand shaking a box: strokes of 3 to 5 Hz up to 1.2 g sideways and 3 g up and
// down, with knocks. Not the prototype's stand-in, a different one, so that the
// simulation is not only tried on what it was tuned on.
Reading shaking(double t, std::uint32_t seed) {
    const auto phase = [seed](int k) {
        return kTwoPi * std::fmod(0.6180339887 * (seed * 7 + k * 13 + 1), 1.0);
    };
    const double burst = t < 1.0 ? 1.0 : std::max(0.0, 1.0 - (t - 1.0) / 0.12);
    const double stroke = std::sin(kTwoPi * 3.7 * t + phase(0));
    const double cross = std::sin(kTwoPi * 3.1 * t + phase(1));
    Reading r;
    r.force = Vec3{1.2 * burst * stroke, 1.2 * burst * cross,
                   1.0 + 2.6 * burst * std::sin(kTwoPi * 3.7 * t + phase(2))};
    r.spin = Vec3{1.4 * burst * cross, -1.4 * burst * stroke, 0.3 * burst * stroke};
    r.spinChange = Vec3{-1.4 * burst * kTwoPi * 3.1 * std::sin(kTwoPi * 3.1 * t + phase(1)),
                        1.4 * burst * kTwoPi * 3.7 * std::sin(kTwoPi * 3.7 * t + phase(0)), 0.0};
    return r;
}

bool finiteEverywhere(const World& world) {
    for (const Die& d : world.dice()) {
        for (double value : {d.x.x, d.x.y, d.x.z, d.q.w, d.q.x, d.q.y, d.q.z, d.v.x, d.v.y, d.v.z,
                             d.w.x, d.w.y, d.w.z}) {
            if (!std::isfinite(value)) {
                return false;
            }
        }
    }
    return true;
}

// How far any corner of any die is outside the box.
double outsideBox(const World& world) {
    double worst = 0.0;
    for (const Die& d : world.dice()) {
        const auto e = axesOf(d.q);
        for (int corner = 0; corner < 8; ++corner) {
            const Vec3 p = d.x + e[0] * ((corner & 1) != 0 ? 0.5 : -0.5) +
                           e[1] * ((corner & 2) != 0 ? 0.5 : -0.5) +
                           e[2] * ((corner & 4) != 0 ? 0.5 : -0.5);
            worst = std::max({worst, -p.z, std::abs(p.x) - world.halfWidth(),
                              std::abs(p.y) - world.halfDepth(), p.z - world.lidZ()});
        }
    }
    return worst;
}

}  // namespace

TEST_CASE("a die in free fall falls as s = g t^2 / 2", "[dice]") {
    World world = roomy(airless());
    world.dice()[0].x = Vec3{0.0, 0.0, 20.0};
    run(world, 0.1);
    const Tuning t;
    CHECK(world.dice()[0].x.z == Approx(20.0 - 0.5 * t.g * 0.01).margin(0.05));
    CHECK(world.dice()[0].v.z == Approx(-t.g * 0.1).margin(0.1));
    CHECK(world.dice()[0].x.x == Approx(0.0).margin(1e-9));
}

TEST_CASE("the box's acceleration is what the accelerometer reads, reversed", "[dice]") {
    World world = roomy(airless());
    world.dice()[0].x = Vec3{0.0, 0.0, 20.0};
    Reading r;
    r.force = Vec3{0.5, -0.25, 1.0};
    run(world, 0.1, r);
    const Tuning t;
    CHECK(world.dice()[0].x.x == Approx(-0.5 * 0.5 * t.g * 0.01).margin(0.03));
    CHECK(world.dice()[0].x.y == Approx(0.5 * 0.25 * t.g * 0.01).margin(0.03));
    CHECK(world.dice()[0].x.z == Approx(20.0 - 0.5 * t.g * 0.01).margin(0.05));
}

TEST_CASE("a spinning box throws a die outward (centrifugal) and sideways (Coriolis)", "[dice]") {
    // No gravity in the box's frame (the device in free fall) and a turn of 2 rad/s
    // about the vertical: a die 3 edges from the axis is accelerated by w^2 r = 12.
    World world = roomy(airless());
    world.dice()[0].x = Vec3{3.0, 0.0, 20.0};
    Reading r;
    r.force = Vec3{0.0, 0.0, 0.0};
    r.spin = Vec3{0.0, 0.0, 2.0};
    run(world, 0.01, r);
    CHECK(world.dice()[0].v.x == Approx(0.12).epsilon(0.03));
    // Moving outward at speed v, the Coriolis term -2 w x v pushes it to -y, w ~ 2: 4 v.
    run(world, 0.2, r);
    CHECK(world.dice()[0].v.y < 0.0);
    // A change of the turn pushes it round: -alpha x r, with alpha = (0, 0, 5), r = (x, 0): +y...
    // -5 x.
    World euler = roomy(airless());
    euler.dice()[0].x = Vec3{3.0, 0.0, 20.0};
    Reading e;
    e.force = Vec3{0.0, 0.0, 0.0};
    e.spinChange = Vec3{0.0, 0.0, 5.0};
    run(euler, 0.01, e);
    CHECK(euler.dice()[0].v.y == Approx(-5.0 * 3.0 * 0.01).epsilon(0.03));
}

TEST_CASE("a free die keeps its spin", "[dice]") {
    World world = roomy(airless());
    world.dice()[0].x = Vec3{0.0, 0.0, 20.0};
    const Vec3 w{1.0, 2.0, 3.0};
    world.dice()[0].w = w;
    Reading r;
    r.force = Vec3{0.0, 0.0, 0.0};
    run(world, 1.0, r);
    const Quat expected = about(w, length(w) * 1.0);
    CHECK(angleBetween(world.dice()[0].q, expected) < 1e-3);
    CHECK(length(world.dice()[0].w) == Approx(length(w)).epsilon(1e-6));
}

TEST_CASE("a die at rest on the floor stays at rest", "[dice]") {
    World world = roomy(1, 1, 3.0, 100.0);
    run(world, 1.5);
    const Die& d = world.dice()[0];
    CHECK(d.x.z == Approx(0.5).margin(5e-4));
    CHECK(d.x.x == Approx(0.0).margin(1e-3));
    CHECK(length(d.v) < 0.02);
    CHECK(length(d.w) < 0.02);
    CHECK(angleBetween(d.q, Quat{}) < 1e-3);
}

TEST_CASE("a die dropped flat rebounds with the coefficient of restitution", "[dice]") {
    Tuning t = airless();
    World world = roomy(t);
    const double drop = 2.0;
    world.dice()[0].x = Vec3{0.0, 0.0, 0.5 + drop};
    world.setReading(Reading{});
    bool bounced = false;
    double top = 0.0;
    double previous = world.dice()[0].v.z;
    for (int i = 0; i < 1500; ++i) {
        world.advance(0.001);
        const Die& d = world.dice()[0];
        if (!bounced && d.v.z > 0.0 && previous < 0.0) {
            bounced = true;
        }
        if (bounced) {
            top = std::max(top, d.x.z - 0.5);
        }
        previous = d.v.z;
        if (bounced && d.v.z < 0.0) {
            break;
        }
    }
    REQUIRE(bounced);
    const double expected = t.restitutionFloor * t.restitutionFloor * drop;
    CHECK(top == Approx(expected).epsilon(0.2));
    CHECK(top < drop);
}

TEST_CASE("a die sliding on the floor stops in v^2 / (2 mu g)", "[dice]") {
    const Tuning t = airless();
    World world = roomy(t);
    const double speed = 20.0;
    world.dice()[0].v = Vec3{speed, 0.0, 0.0};
    run(world, 1.0);
    const Die& d = world.dice()[0];
    const double expected = speed * speed / (2.0 * t.frictionFloor * t.g);
    CHECK(d.x.x == Approx(expected).epsilon(0.1));
    CHECK(length(d.v) < 0.05);
    CHECK(d.x.z == Approx(0.5).margin(1e-3));
}

TEST_CASE("friction holds a die on a slope that is gentler than the friction angle", "[dice]") {
    const Tuning t = airless();
    const double angle = 20.0 * std::numbers::pi / 180.0;  // tan 20 = 0.36 < mu
    World world = roomy(t);
    Reading r;
    r.force = Vec3{std::sin(angle), 0.0, std::cos(angle)};
    run(world, 1.5, r);
    CHECK(std::abs(world.dice()[0].x.x) < 0.02);
    CHECK(length(world.dice()[0].v) < 0.05);
}

TEST_CASE("a die slides down a slope that is steeper, at g (sin a - mu cos a)", "[dice]") {
    const Tuning t = airless();
    const double angle = 35.0 * std::numbers::pi / 180.0;
    World world = roomy(t, 1, 1, 20.0);
    Reading r;
    r.force = Vec3{std::sin(angle), 0.0, std::cos(angle)};
    run(world, 0.3, r);
    const double acceleration = t.g * (std::sin(angle) - t.frictionFloor * std::cos(angle));
    CHECK(world.dice()[0].x.x == Approx(-0.5 * acceleration * 0.09).epsilon(0.15));
}

TEST_CASE("two dice meeting head on share their momentum by the restitution", "[dice]") {
    const Tuning t = airless();
    World world = roomy(t, 1, 2, 8.0);
    Reading r;
    r.force = Vec3{0.0, 0.0, 0.0};
    world.setReading(r);
    world.dice()[0].x = Vec3{-3.0, 0.0, 10.0};
    world.dice()[1].x = Vec3{0.0, 0.0, 10.0};
    world.dice()[0].v = Vec3{10.0, 0.0, 0.0};
    world.advance(0.6);
    const Die& a = world.dice()[0];
    const Die& b = world.dice()[1];
    const double e = t.restitutionDie;
    CHECK(a.v.x + b.v.x == Approx(10.0).epsilon(0.01));
    CHECK(b.v.x - a.v.x == Approx(e * 10.0).epsilon(0.06));
    CHECK(length(a.w) < 0.3);
    CHECK(length(b.w) < 0.3);
}

TEST_CASE("an off-centre collision conserves momentum and does not make energy", "[dice]") {
    const Tuning t = airless();
    World world = roomy(t, 1, 2, 8.0);
    Reading r;
    r.force = Vec3{0.0, 0.0, 0.0};
    world.setReading(r);
    world.dice()[0].x = Vec3{-3.0, 0.3, 10.0};
    world.dice()[1].x = Vec3{0.0, 0.0, 10.0};
    world.dice()[0].v = Vec3{12.0, 0.0, 0.0};
    world.dice()[0].q = about(Vec3{0.0, 0.0, 1.0}, 0.3);
    const double before = world.kineticEnergy();
    world.advance(0.8);
    const Vec3 momentum = world.dice()[0].v + world.dice()[1].v;
    CHECK(momentum.x == Approx(12.0).epsilon(1e-3));
    CHECK(momentum.y == Approx(0.0).margin(0.02));
    CHECK(momentum.z == Approx(0.0).margin(0.02));
    CHECK(world.kineticEnergy() <= before + 1e-6);
    // It really did hit: the other die moves, and turns.
    CHECK(world.dice()[1].v.x > 1.0);
}

TEST_CASE("a stack of dice stands", "[dice]") {
    World stack = roomy(1, 3, 3.0, 100.0);
    // three on one cell, one on another
    for (int i = 0; i < 3; ++i) {
        stack.dice()[static_cast<std::size_t>(i)].x = Vec3{0.0, 0.0, 0.5 + i};
    }
    run(stack, 2.0);
    for (int i = 0; i < 3; ++i) {
        const Die& d = stack.dice()[static_cast<std::size_t>(i)];
        CHECK(d.x.z == Approx(0.5 + i).margin(0.03));
        CHECK(std::abs(d.x.x) < 0.03);
        CHECK(length(d.v) < 0.1);
    }
    // One set a third of an edge off its neighbour's centre is still held (the centre of
    // mass is over the support).
    World offset = roomy(1, 2, 3.0, 100.0);
    offset.dice()[0].x = Vec3{0.0, 0.0, 0.5};
    offset.dice()[1].x = Vec3{0.3, 0.0, 1.5};
    run(offset, 2.0);
    CHECK(offset.dice()[1].x.z == Approx(1.5).margin(0.03));
    CHECK(offset.dice()[1].x.x == Approx(0.3).margin(0.05));
    CHECK(angleBetween(offset.dice()[1].q, Quat{}) < 0.05);
}

TEST_CASE("a die tipped on an edge falls flat, whichever way it is nearer", "[dice]") {
    for (const double degrees : {20.0, 40.0, 50.0, 70.0}) {
        World world = roomy(1, 1, 3.0, 100.0);
        Die& d = world.dice()[0];
        d.q = about(Vec3{1.0, 0.0, 0.0}, degrees * std::numbers::pi / 180.0);
        d.x.z += -lowestCorner(d);
        run(world, 2.0);
        const Die& after = world.dice()[0];
        INFO("tipped by " << degrees << " degrees");
        CHECK(after.x.z == Approx(0.5).margin(0.01));
        CHECK(length(after.v) < 0.1);
        // flat: an axis is within a degree of vertical
        CHECK(angleBetween(after.q, squareTo(after.q)) < 0.03);
    }
}

TEST_CASE("a die thrown at the lid and the walls stays in the box", "[dice]") {
    World world = roomy(1, 1, 1.0, 2.45);
    world.dice()[0].v = Vec3{25.0, 17.0, 31.0};
    world.dice()[0].w = Vec3{9.0, -7.0, 12.0};
    for (int i = 0; i < 300; ++i) {
        world.advance(0.01);
        REQUIRE(finiteEverywhere(world));
        REQUIRE(outsideBox(world) < 0.05);
    }
}

TEST_CASE("sixteen dice shaken hard stay in the box and out of one another", "[dice]") {
    for (const std::uint32_t seed : {3U, 11U}) {
        Box box;
        World world(box, Tuning{}, seed);
        double deepest = 0.0;
        double outside = 0.0;
        double farthest = 0.0;
        double highest = 0.0;
        for (int i = 0; i < 250; ++i) {
            const double t = i * 0.01;
            world.setReading(shaking(t, seed));
            world.advance(0.01);
            REQUIRE(finiteEverywhere(world));
            outside = std::max(outside, outsideBox(world));
            if (i % 5 == 0) {
                deepest = std::max(deepest, world.deepestOverlap());
                // how far from their cells the dice are, all together, and how high one is
                double travelled = 0.0;
                for (int k = 0; k < world.count(); ++k) {
                    const Die& d = world.dice()[static_cast<std::size_t>(k)];
                    travelled += length(d.x - world.cellCentre(k));
                    highest = std::max(highest, d.x.z);
                }
                farthest = std::max(farthest, travelled);
            }
        }
        INFO("seed " << seed);
        CHECK(outside < 0.06);
        CHECK(deepest < 0.1);
        // and they were moved: the box did shake them, and some were thrown up
        CHECK(farthest > 8.0);
        CHECK(highest > 1.0);
    }
}

TEST_CASE("a replay is the same replay", "[dice]") {
    const auto play = [](std::uint32_t seed, std::uint32_t shakeSeed) {
        World world(Box{}, Tuning{}, seed);
        for (int i = 0; i < 120; ++i) {
            world.setReading(shaking(i * 0.01, shakeSeed));
            world.advance(0.01);
        }
        return world.digest();
    };
    CHECK(play(1, 5) == play(1, 5));
    CHECK(play(1, 5) != play(1, 6));
}

TEST_CASE("the least-cost assignment is the least cost", "[dice]") {
    std::uint32_t state = 12345;
    const auto next = [&state]() {
        state = state * 1664525U + 1013904223U;
        return static_cast<double>(state >> 8) / 16777216.0;
    };
    for (int trial = 0; trial < 30; ++trial) {
        const int n = 6;
        std::vector<double> cost(static_cast<std::size_t>(n * n));
        for (double& c : cost) {
            c = next() * 10.0;
        }
        const std::vector<int> found = leastCostAssignment(cost, n);
        std::vector<int> permutation(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            permutation[static_cast<std::size_t>(i)] = i;
        }
        double best = 1e18;
        do {
            double total = 0.0;
            for (int i = 0; i < n; ++i) {
                total += cost[static_cast<std::size_t>(i * n +
                                                       permutation[static_cast<std::size_t>(i)])];
            }
            best = std::min(best, total);
        } while (std::next_permutation(permutation.begin(), permutation.end()));
        double got = 0.0;
        std::vector<int> seen(static_cast<std::size_t>(n), 0);
        for (int i = 0; i < n; ++i) {
            got += cost[static_cast<std::size_t>(i * n + found[static_cast<std::size_t>(i)])];
            seen[static_cast<std::size_t>(found[static_cast<std::size_t>(i)])] += 1;
        }
        CHECK(got == Approx(best).epsilon(1e-9));
        CHECK(std::all_of(seen.begin(), seen.end(), [](int s) { return s == 1; }));
    }
}

TEST_CASE("a die is squared to the board by the nearest quarter turns", "[dice]") {
    // Already square: unchanged, whichever of the 24.
    const Quat flat = about(Vec3{0.0, 0.0, 1.0}, 0.5 * std::numbers::pi);
    CHECK(angleBetween(squareTo(flat), flat) < 1e-9);
    const Quat onSide = about(Vec3{1.0, 0.0, 0.0}, 0.5 * std::numbers::pi);
    CHECK(angleBetween(squareTo(onSide), onSide) < 1e-9);
    // A little off: back to the square one.
    const Quat nudged = about(Vec3{1.0, 1.0, 0.3}, 0.15) * onSide;
    CHECK(angleBetween(squareTo(nudged), onSide) < 1e-9);
    // Always one of the 24 right-handed ones: its axes are signed unit vectors.
    for (int i = 0; i < 40; ++i) {
        const Quat q = normalized(
            Quat{std::sin(i * 1.7) + 1.1, std::cos(i * 2.3), std::sin(i * 0.7), std::cos(i * 1.1)});
        const auto e = axesOf(squareTo(q));
        for (const Vec3& axis : e) {
            const double largest = std::max({std::abs(axis.x), std::abs(axis.y), std::abs(axis.z)});
            CHECK(largest == Approx(1.0).margin(1e-9));
        }
        CHECK(dot(cross(e[0], e[1]), e[2]) == Approx(1.0).margin(1e-9));
    }
}

TEST_CASE("the face that is up, and the turns that make its letter read upright", "[dice]") {
    const Upright still = uprightOf(Quat{});
    CHECK(still.face == 4);
    CHECK(still.turns == 0);
    // The die turned a quarter anticlockwise on the board: the face's up points to -x,
    // and three more quarter turns of the letter put it back to +y.
    const Upright quarter = uprightOf(about(Vec3{0.0, 0.0, 1.0}, 0.5 * std::numbers::pi));
    CHECK(quarter.face == 4);
    CHECK(quarter.turns == 3);
    // Turned so the face's up points to +x, one quarter turn does it.
    const Upright other = uprightOf(about(Vec3{0.0, 0.0, 1.0}, -0.5 * std::numbers::pi));
    CHECK(other.turns == 1);
    const Upright half = uprightOf(about(Vec3{0.0, 0.0, 1.0}, std::numbers::pi));
    CHECK(half.turns == 2);
    // On its side, with its +x face up.
    const Upright side = uprightOf(about(Vec3{0.0, 1.0, 0.0}, -0.5 * std::numbers::pi));
    CHECK(side.face == 0);
    // Whatever the turns, the letter's up lands on +y.
    for (int i = 0; i < 60; ++i) {
        const Quat q = squareTo(normalized(Quat{std::sin(i * 1.3) + 1.2, std::cos(i * 1.9),
                                                std::sin(i * 0.9), std::cos(i * 0.4)}));
        const Upright u = uprightOf(q);
        Vec3 up = kFaceUp[static_cast<std::size_t>(u.face)];
        const Vec3 normal = kFaceNormal[static_cast<std::size_t>(u.face)];
        for (int k = 0; k < u.turns; ++k) {
            // a quarter turn anticlockwise about the face's outward normal
            up = cross(normal, up);
        }
        const Vec3 world = rotate(q, up);
        CHECK(world.y == Approx(1.0).margin(1e-9));
    }
}

TEST_CASE("after a shake the guide brings every die home, square, and still", "[dice]") {
    for (const std::uint32_t seed : {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}) {
        World world(Box{}, Tuning{}, seed);
        double t = 0.0;
        for (; t < 0.9; t += 0.01) {
            world.setReading(shaking(t, seed));
            world.advance(0.01);
        }
        // the hand puts the box down
        world.setReading(Reading{});
        world.advance(0.15);
        world.setLid(false);
        const std::vector<int> cells = world.assignCells();
        std::vector<double> delays;
        for (int i = 0; i < world.count(); ++i) {
            delays.push_back(0.03 * ((i * 7) % world.count()));
        }
        world.guide(delays);
        double elapsed = 0.0;
        while (!world.allDone() && elapsed < 3.0) {
            world.advance(0.01);
            elapsed += 0.01;
        }
        INFO("seed " << seed << " took " << elapsed << " s");
        CHECK(world.allDone());
        CHECK(elapsed < 2.5);
        std::vector<int> seen(cells.size(), 0);
        for (int i = 0; i < world.count(); ++i) {
            const Die& d = world.dice()[static_cast<std::size_t>(i)];
            seen[static_cast<std::size_t>(cells[static_cast<std::size_t>(i)])] += 1;
            const Vec3 home = world.cellCentre(cells[static_cast<std::size_t>(i)]);
            CHECK(length(d.x - home) < 1e-9);
            CHECK(length(d.v) == 0.0);
        }
        CHECK(std::all_of(seen.begin(), seen.end(), [](int s) { return s == 1; }));
        CHECK(world.deepestOverlap() < 0.02);
    }
}
