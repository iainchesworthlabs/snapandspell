#include "wjs/scan/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace wjs::scan {

namespace {

constexpr double kEpsilon = 1e-12;

double cross2(Point a, Point b) noexcept {
    return a.x * b.y - a.y * b.x;
}

}  // namespace

double signed_area(std::span<const Point> polygon) noexcept {
    double twice = 0.0;
    const std::size_t n = polygon.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Point a = polygon[i];
        const Point b = polygon[(i + 1) % n];
        twice += a.x * b.y - b.x * a.y;
    }
    return twice / 2.0;
}

double distance(Point a, Point b) noexcept {
    return std::hypot(a.x - b.x, a.y - b.y);
}

std::vector<Point> convex_hull(std::vector<Point> points) {
    std::ranges::sort(points, [](Point a, Point b) { return a.x != b.x ? a.x < b.x : a.y < b.y; });
    points.erase(std::unique(points.begin(), points.end()), points.end());
    if (points.size() < 3) {
        return points;
    }

    // Andrew's monotone chain, keeping the turns that are clockwise on a screen.
    std::vector<Point> hull(2 * points.size());
    std::size_t k = 0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        while (k >= 2 && cross(hull[k - 2], hull[k - 1], points[i]) <= 0.0) {
            --k;
        }
        hull[k++] = points[i];
    }
    const std::size_t lower = k + 1;
    for (std::size_t i = points.size() - 1; i > 0; --i) {
        while (k >= lower && cross(hull[k - 2], hull[k - 1], points[i - 1]) <= 0.0) {
            --k;
        }
        hull[k++] = points[i - 1];
    }
    hull.resize(k - 1);
    return hull;
}

std::optional<Quad> enclosing_quad(std::span<const Point> convex_polygon) {
    std::vector<Point> p(convex_polygon.begin(), convex_polygon.end());
    if (p.size() < 4) {
        return std::nullopt;
    }

    while (p.size() > 4) {
        const std::size_t n = p.size();
        std::size_t best = n;
        double best_added = std::numeric_limits<double>::max();
        Point best_point{};

        for (std::size_t i = 0; i < n; ++i) {
            // Merge the edge b -> c: extend a -> b forward and d -> c backward
            // (that is, c -> d's line, past c) until they meet.
            const Point a = p[(i + n - 1) % n];
            const Point b = p[i];
            const Point c = p[(i + 1) % n];
            const Point d = p[(i + 2) % n];
            const Point r = b - a;
            const Point s = c - d;
            const double denominator = cross2(r, s);
            if (std::abs(denominator) < kEpsilon) {
                continue;
            }
            const Point from_a = d - a;
            const double t = cross2(from_a, s) / denominator;
            const double u = cross2(from_a, r) / denominator;
            if (t <= 1.0 || u <= 1.0) {
                continue;  // the edges diverge: no point beyond b and c joins them
            }
            const Point meet = a + r * t;
            const double added = std::abs(cross(b, meet, c)) / 2.0;
            if (added < best_added) {
                best_added = added;
                best = i;
                best_point = meet;
            }
        }

        if (best == n) {
            return std::nullopt;
        }
        const std::size_t next = (best + 1) % n;
        p[best] = best_point;
        p.erase(p.begin() + static_cast<std::ptrdiff_t>(next));
    }

    return Quad{p[0], p[1], p[2], p[3]};
}

Quad rotate_to_upright(const Quad& quad) noexcept {
    std::size_t first = 0;
    double best = std::numeric_limits<double>::max();
    for (std::size_t k = 0; k < 4; ++k) {
        const Point edge = quad[(k + 1) % 4] - quad[k];
        const double angle = std::abs(std::atan2(edge.y, edge.x));
        if (angle < best) {
            best = angle;
            first = k;
        }
    }
    return Quad{quad[first], quad[(first + 1) % 4], quad[(first + 2) % 4], quad[(first + 3) % 4]};
}

Point Homography::apply(Point p) const noexcept {
    const double w = m[6] * p.x + m[7] * p.y + m[8];
    const double x = m[0] * p.x + m[1] * p.y + m[2];
    const double y = m[3] * p.x + m[4] * p.y + m[5];
    return {x / w, y / w};
}

std::optional<Homography> homography(const Quad& from, const Quad& to) noexcept {
    // Eight unknowns h0..h7 (h8 = 1), two equations a point.
    double a[8][9] = {};
    for (std::size_t k = 0; k < 4; ++k) {
        const double x = from[k].x;
        const double y = from[k].y;
        const double u = to[k].x;
        const double v = to[k].y;
        double* row_u = a[2 * k];
        double* row_v = a[2 * k + 1];
        row_u[0] = x;
        row_u[1] = y;
        row_u[2] = 1.0;
        row_u[3] = 0.0;
        row_u[4] = 0.0;
        row_u[5] = 0.0;
        row_u[6] = -x * u;
        row_u[7] = -y * u;
        row_u[8] = u;
        row_v[0] = 0.0;
        row_v[1] = 0.0;
        row_v[2] = 0.0;
        row_v[3] = x;
        row_v[4] = y;
        row_v[5] = 1.0;
        row_v[6] = -x * v;
        row_v[7] = -y * v;
        row_v[8] = v;
    }

    // Gaussian elimination with partial pivoting.
    for (std::size_t column = 0; column < 8; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < 8; ++row) {
            if (std::abs(a[row][column]) > std::abs(a[pivot][column])) {
                pivot = row;
            }
        }
        if (std::abs(a[pivot][column]) < kEpsilon) {
            return std::nullopt;
        }
        if (pivot != column) {
            for (std::size_t j = 0; j < 9; ++j) {
                std::swap(a[pivot][j], a[column][j]);
            }
        }
        for (std::size_t row = 0; row < 8; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = a[row][column] / a[column][column];
            for (std::size_t j = column; j < 9; ++j) {
                a[row][j] -= factor * a[column][j];
            }
        }
    }

    Homography h;
    for (std::size_t i = 0; i < 8; ++i) {
        h.m[i] = a[i][8] / a[i][i];
    }
    h.m[8] = 1.0;
    return h;
}

std::optional<Homography> inverse(const Homography& h) noexcept {
    const auto& m = h.m;
    const double c0 = m[4] * m[8] - m[5] * m[7];
    const double c1 = m[5] * m[6] - m[3] * m[8];
    const double c2 = m[3] * m[7] - m[4] * m[6];
    const double det = m[0] * c0 + m[1] * c1 + m[2] * c2;
    if (std::abs(det) < kEpsilon) {
        return std::nullopt;
    }
    Homography out;
    out.m[0] = c0 / det;
    out.m[1] = (m[2] * m[7] - m[1] * m[8]) / det;
    out.m[2] = (m[1] * m[5] - m[2] * m[4]) / det;
    out.m[3] = c1 / det;
    out.m[4] = (m[0] * m[8] - m[2] * m[6]) / det;
    out.m[5] = (m[2] * m[3] - m[0] * m[5]) / det;
    out.m[6] = c2 / det;
    out.m[7] = (m[1] * m[6] - m[0] * m[7]) / det;
    out.m[8] = (m[0] * m[4] - m[1] * m[3]) / det;
    return out;
}

}  // namespace wjs::scan
