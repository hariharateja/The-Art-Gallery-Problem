#pragma once
#include "geometry.h"

// ─── Triangle ─────────────────────────────────────────────
struct Triangle {
    Point a, b, c;
    Triangle(Point a, Point b, Point c) : a(a), b(b), c(c) {}
};

// ─── Triangulate a single Y-Monotone Polygon ──────────────
// Input  : vertices of a y-monotone polygon (any order)
// Output : list of triangles
vector<Triangle> triangulateMonotone(vector<Point> poly);

// ─── Triangulate all monotone pieces ──────────────────────
// Input  : list of monotone polygons (from sweep decomposition)
// Output : full list of triangles
vector<Triangle> triangulateAll(const vector<vector<Point>>& monotonePolygons);

// ─── Helpers ──────────────────────────────────────────────

// Split monotone polygon into left and right chains
// (sorted top to bottom)
pair<vector<Point>, vector<Point>> getMonotoneChains(const vector<Point>& poly);

// Check if diagonal (stack.top → v) is inside polygon
bool isValidDiagonal(const Point& u, const Point& v,
                      const Point& w, bool isLeftChain);