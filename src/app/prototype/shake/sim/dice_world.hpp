#pragma once

// Dice in a box, simulated as rigid bodies, from first principles.
//
// What is modelled: each die is a rigid cube of one unit's edge and one unit's
// mass (so its moment of inertia about any axis through its centre is 1/6, and
// it has no gyroscopic term). Lengths are in die edges; with a die taken as
// 20 mm, 1 g is 490.5 edges per second squared and time is real. The dice move
// in the frame of the box, which is a floor, four walls and a lid. The box is
// moved by a hand, and the dice feel it as inertia: in the box's frame a die is
// accelerated by the *negative* of what the device's accelerometer reads (so
// gravity, a tilt, a shove and a jolt are one thing), by the centrifugal,
// Coriolis and Euler terms of the gyroscope's rotation, and by nothing else.
//
// How: sequential impulses (Catto, "Iterative dynamics with temporal
// coherence"), in small fixed steps. Each step the forces change the velocities,
// the contacts between dice, and between dice and the box, are found (a
// separating-axis test with face clipping for two cubes, the eight corners
// against a plane for the box; points a little short of touching are kept, so a
// die cannot close on a surface within a step), each contact starts from the
// impulse it carried in the step before (warm starting), a few passes solve the
// velocity constraints with the impulses accumulated and clamped (a contact can
// only push; friction stays inside the cone of the normal impulse; a fast
// impact bounces by the contact's restitution), the bodies move, and what
// overlap is left is pushed out in position alone, which adds no energy.
// Steps are fixed, in double precision, and nothing is random but the seed, so
// the same inputs give the same dice on the same build.
//
// On top of the physics there is one thing that is not physical: a guide. When
// the shaking has stopped, each die is given a cell of the board to go to (the
// assignment that makes the dice travel least), and from a moment of its own it
// is hopped and pulled there by a spring while it is also turned flat and square
// to the board. The dice still collide with one another on the way. The outcome
// of a play is decided before it starts; the physics decides how it looks.

#include <array>
#include <cstdint>
#include <vector>

#include "math3.hpp"

namespace wjs::dice {

// A die's faces, numbered by the local axis they face: +x, -x, +y, -y, +z, -z.
// Each has a frame (u to the right, v up, outward normal n = u x v) in which its
// letter is drawn.
inline constexpr int kFaceCount = 6;
inline constexpr std::array<Vec3, kFaceCount> kFaceNormal{{{1.0, 0.0, 0.0},
                                                           {-1.0, 0.0, 0.0},
                                                           {0.0, 1.0, 0.0},
                                                           {0.0, -1.0, 0.0},
                                                           {0.0, 0.0, 1.0},
                                                           {0.0, 0.0, -1.0}}};
inline constexpr std::array<Vec3, kFaceCount> kFaceRight{{{0.0, 1.0, 0.0},
                                                          {0.0, 0.0, 1.0},
                                                          {0.0, 0.0, 1.0},
                                                          {1.0, 0.0, 0.0},
                                                          {1.0, 0.0, 0.0},
                                                          {0.0, 1.0, 0.0}}};
inline constexpr std::array<Vec3, kFaceCount> kFaceUp{{{0.0, 0.0, 1.0},
                                                       {0.0, 1.0, 0.0},
                                                       {1.0, 0.0, 0.0},
                                                       {0.0, 0.0, 1.0},
                                                       {0.0, 1.0, 0.0},
                                                       {1.0, 0.0, 0.0}}};

// The box. Rows run down the board (y up on screen), columns across it (x right).
struct Box {
    int rows = 4;
    int columns = 4;
    double pitch = 1.08;      // between the centres of neighbouring cells
    double slack = 0.6;       // room past the outer cells, on every side
    double lidHeight = 2.45;  // the lid's underside above the floor
};

// What the device reports: the accelerometer's specific force in g (gravity
// included: a device lying face up reads 0, 0, 1), and the gyroscope's rate in
// rad/s and the rate of change of that.
struct Reading {
    Vec3 force{0.0, 0.0, 1.0};
    Vec3 spin;
    Vec3 spinChange;
};

struct Tuning {
    double g = 9810.0 / 20.0;       // 1 g, in die edges per second squared
    double substep = 1.0 / 1200.0;  // seconds
    int velocityIterations = 6;
    int positionPasses = 3;
    double slop = 0.0005;          // overlap tolerated, in edges
    double airDrag = 0.05;         // per second
    double airSpinDrag = 0.05;     // per second
    double contactSpinDrag = 1.5;  // per second, while touching: the rolling resistance of felt
    // Restitution and friction for the surfaces a die meets.
    double restitutionFloor = 0.35;
    double restitutionWall = 0.30;
    double restitutionLid = 0.25;
    double restitutionDie = 0.40;
    double frictionFloor = 0.50;
    double frictionWall = 0.20;
    double frictionLid = 0.20;
    double frictionDie = 0.30;
    double variety = 0.15;        // dice differ in grip and bounce by up to this much
    double contactMargin = 0.03;  // points this near touching are held too (speculative contacts)
    // Below this closing speed (and what the box's own acceleration adds in a step)
    // a contact does not bounce.
    double restSpeed = 0.5;
    // The guide.
    double guideRamp = 0.04;         // seconds from nothing to full
    double guideStiffness = 700.0;   // 1/s^2 toward the cell, in the plan
    double guideDamping = 53.0;      // 1/s
    double guideTurn = 900.0;        // 1/s^2 toward flat and square
    double guideTurnDamping = 60.0;  // 1/s
    double guideSpeedLimit = 28.0;   // edges per second
    double hopNear = 0.3;            // height of the hop at a cell's distance of nothing, edges
    double hopFar = 1.0;             // and for crossing the box
    double unstickRise = 1.3;      // height of the hop of a die that is stuck behind others, edges
    double stuckAfter = 0.12;      // seconds going nowhere before it hops again
    double settleDistance = 0.03;  // within this of its cell, flat, still: it is down
    double settleAngle = 0.05;     // radians
    double settleSpeed = 0.6;      // edges per second, and radians per second
    double settleCalm = 0.04;      // seconds it has to be so for
};

enum class Surface { Floor, Wall, Lid, Die };

// A collision at speed, for sounds and haptics.
struct Impact {
    int die = 0;
    Surface surface = Surface::Floor;
    double speed = 0.0;
};

struct Die {
    Vec3 x{0.0, 0.0,
           0.5};  // centre, in the box's frame: x right, y up the board, z up from the floor
    Quat q;       // orientation in the box's frame
    Vec3 v;
    Vec3 w;                 // angular velocity relative to the box
    int home = -1;          // the cell it has been given, once there is a guide
    double guideAt = -1.0;  // time at which the guide starts for it; negative for none
    bool launched = false;  // has been hopped
    bool settled = false;   // is on its cell, flat and still, for now
    bool done = false;      // is home: fixed from now on
    double calm = 0.0;
    double stuck = 0.0;   // seconds spent on the floor, short of home, going nowhere
    double grip = 1.0;    // how much grippier than the next die this one is
    double bounce = 1.0;  // and how much bouncier: no two dice are alike
    double hopRise = 0.0;
    Vec3 xPrev;
    Quat qPrev;
};

// The face of a die pointing most nearly up, and how many quarter turns its
// letter has to be turned (anticlockwise as seen from outside the face) to read
// upright, with the die as square to the board as it can be made.
struct Upright {
    int face = 4;
    int turns = 0;
};
Upright uprightOf(Quat q);

// The nearest of the 24 orientations that put a die flat and square to the box.
Quat squareTo(Quat q);

class World {
public:
    World(const Box& box, const Tuning& tuning, std::uint32_t seed);

    const Box& box() const { return box_; }
    const Tuning& tuning() const { return tuning_; }
    int count() const { return static_cast<int>(dice_.size()); }
    std::vector<Die>& dice() { return dice_; }
    const std::vector<Die>& dice() const { return dice_; }
    double time() const { return time_; }
    double halfWidth() const { return halfWidth_; }
    double halfDepth() const { return halfDepth_; }
    double lidZ() const { return lidZ_; }

    // The middle of a cell, with a die's height. Cells are numbered row-major.
    Vec3 cellCentre(int cell) const;

    // What the box is doing, from now until it is set again.
    void setReading(const Reading& reading) { reading_ = reading; }
    // Whether the lid is on.
    void setLid(bool on) { lid_ = on; }

    // Moves time on. Whole steps are taken; what is left over is carried.
    void advance(double seconds);

    // Gives each die a cell, by least total distance in the plan, and returns
    // the cell of each die.
    std::vector<int> assignCells();
    // Starts the guide for die i at time() + delays[i]. Needs assignCells first.
    void guide(const std::vector<double>& delays);
    // Puts every die on its cell, square, at rest.
    void finish();
    bool allDone() const;

    // The impacts since the last call.
    std::vector<Impact> takeImpacts();

    // A checksum of every die's state, to compare replays.
    std::uint32_t digest() const;

    // For tests.
    double kineticEnergy() const;
    double potentialEnergy() const;
    // The deepest overlap between two dice or a die and the box now, in edges.
    double deepestOverlap() const;

private:
    struct Contact {
        int a = 0;
        int b = -1;    // -1: a fixed surface, whose point is in lb
        int key = -1;  // for the box: which plane and corner, to find it in the next step
        Vec3 n;        // from a toward b
        Vec3 la;       // the point on a, in a's frame
        Vec3 lb;       // the point on b, in b's frame (on a surface: in the box's)
        double restitution = 0.0;
        double friction = 0.0;
        Surface surface = Surface::Floor;
        // Worked out when the step begins.
        double separation = 0.0;  // along n; negative when they overlap
        double approach = 0.0;    // closing speed along n
        double target = 0.0;      // the closing speed the contact may have
        Vec3 rA;
        Vec3 rB;
        Vec3 t1;
        Vec3 t2;
        double massN = 0.0;
        double massT1 = 0.0;
        double massT2 = 0.0;
        // The impulses so far, along n and the two tangents.
        double jn = 0.0;
        double jt1 = 0.0;
        double jt2 = 0.0;
        Vec3 tangentImpulse;  // jt1 t1 + jt2 t2, kept for the next step
    };

    void step(double h);
    void applyForces(double h);
    void findContacts();
    void detectBox(int i);
    void collidePair(int a, int b);
    void addContact(Contact c);
    void prepareContacts(double h);
    void solveContacts();
    void moveBodies(double h);
    void correctPositions();
    void finishStep(double h);
    const Contact* previousOf(const Contact& c) const;
    double effectiveMass(const Contact& c, Vec3 direction) const;
    Vec3 relativeVelocity(const Contact& c) const;
    double guideWeight(const Die& d) const;
    double inverseMass(int i) const;
    double inverseInertia(int i) const;
    Vec3 pointOf(int i, Vec3 local) const;
    Vec3 pointVelocity(int i, Vec3 r) const;
    void push(int i, Vec3 impulse, Vec3 r);
    void impulse(int i, Vec3 impulse, Vec3 r);
    double random();

    Box box_;
    Tuning tuning_;
    std::vector<Die> dice_;
    Reading reading_;
    bool lid_ = true;
    double halfWidth_ = 0.0;
    double halfDepth_ = 0.0;
    double lidZ_ = 0.0;
    double time_ = 0.0;
    double pending_ = 0.0;
    std::int64_t steps_ = 0;
    std::uint32_t rng_ = 0;
    std::vector<Contact> contacts_;
    std::vector<Contact> previous_;
    std::vector<Impact> impacts_;
};

// The assignment of n rows to n columns of a cost matrix (row-major) that costs
// least, as the column of each row (the Hungarian method).
std::vector<int> leastCostAssignment(const std::vector<double>& cost, int n);

}  // namespace wjs::dice
