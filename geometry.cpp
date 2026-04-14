#include "geometry.h"

// ─── Cross Product OA × OB ───────────────────────────────
double cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y)
         - (A.y - O.y) * (B.x - O.x);
}

// ─── Is p on segment [a,b]? ──────────────────────────────
bool onSegment(const Point& p, const Point& a, const Point& b) {
    return fabs(cross(a, b, p)) < 1e-9
        && min(a.x, b.x) <= p.x && p.x <= max(a.x, b.x)
        && min(a.y, b.y) <= p.y && p.y <= max(a.y, b.y);
}

// ─── Proper intersection (not at endpoints) ───────────────
bool segmentsIntersectProperly(const Point& a, const Point& b,
                                const Point& c, const Point& d) {
    double d1 = cross(c, d, a);
    double d2 = cross(c, d, b);
    double d3 = cross(a, b, c);
    double d4 = cross(a, b, d);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;
    return false;
}

// ─── General intersection ─────────────────────────────────
bool segmentsIntersect(const Point& a, const Point& b,
                        const Point& c, const Point& d) {
    if (segmentsIntersectProperly(a, b, c, d)) return true;
    if (onSegment(a, c, d)) return true;
    if (onSegment(b, c, d)) return true;
    if (onSegment(c, a, b)) return true;
    if (onSegment(d, a, b)) return true;
    return false;
}

// ─── Sweep helpers ────────────────────────────────────────
bool isAbove(const Point& a, const Point& b) { return a < b; }

double xAtY(const Point& a, const Point& b, double y) {
    if (fabs(a.y - b.y) < 1e-9) return min(a.x, b.x);
    return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
}

// ─── Edge BST comparator (by x at current sweep y) ───────
bool Edge::operator<(const Edge& o) const {
    double y = max({a.y, b.y, o.a.y, o.b.y});
    return xAt(y) < o.xAt(y);
}

// ─── Classify vertex ──────────────────────────────────────
VertexType classifyVertex(const Point& prev, const Point& curr,
                           const Point& next, bool isHole) {
    bool prevAbove = isAbove(prev, curr);
    bool nextAbove = isAbove(next, curr);
    double c = cross(prev, curr, next);
    // CW holes already yield negative c at their topmost/bottommost vertices,
    // which maps correctly to SPLIT/MERGE without any flip.

    if (!prevAbove && !nextAbove)
        return (c > 0) ? START : SPLIT;
    if (prevAbove && nextAbove)
        return (c > 0) ? END : MERGE;
    return REGULAR;
}