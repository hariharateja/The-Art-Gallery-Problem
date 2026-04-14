#pragma once
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <climits>
#include <cassert>
using namespace std;

// ─── Point ───────────────────────────────────────────────
struct Point {
    double x, y;
    int id; // original vertex index

    Point(double x = 0, double y = 0, int id = -1) : x(x), y(y), id(id) {}

    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
    bool operator<(const Point& o)  const {
        if (y != o.y) return y > o.y;   // top to bottom
        return x < o.x;                  // left to right for ties
    }
};

// ─── Geometric Primitives ────────────────────────────────
double cross(const Point& O, const Point& A, const Point& B);
bool   onSegment(const Point& p, const Point& a, const Point& b);
bool   segmentsIntersectProperly(const Point& a, const Point& b,
                                  const Point& c, const Point& d);
bool   segmentsIntersect(const Point& a, const Point& b,
                          const Point& c, const Point& d);

bool isAbove(const Point& a, const Point& b); // is a above b in sweep?

double xAtY(const Point& a, const Point& b, double y); // x-coord of edge ab at height y

// ─── Edge (for sweep-line BST) ───────────────────────────
struct Edge {
    Point a, b;
    Edge() {}
    Edge(const Point& a, const Point& b) : a(a), b(b) {}
    double xAt(double y) const { return xAtY(a, b, y); }
    bool operator<(const Edge& o) const;
};

// ─── Vertex Types (for sweep line) ───────────────────────
enum VertexType { START, END, SPLIT, MERGE, REGULAR };

VertexType classifyVertex(const Point& prev, const Point& curr,
                           const Point& next, bool isHole);