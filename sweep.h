#pragma once
#include "geometry.h"
#include "dcel.h"
#include <set>
#include <cmath>

// ─── Sweep Line Status (BST of active edges) ──────────────
// Edges sorted by x-coordinate at current sweep line y

extern double global_sweepY;

struct SweepEdge {
    Point a, b;           // endpoints
    mutable Point helper;      // mutated when handling events
    mutable bool  helperIsMerge;

    SweepEdge(Point p1={}, Point p2={}, Point h={}) {
        if (p1.y > p2.y || (p1.y == p2.y && p1.x < p2.x)) {
            a = p1; b = p2;
        } else {
            a = p2; b = p1;
        }
        helper = h;
        helperIsMerge = false;
    }

    double xAt(double y) const {
        if (fabs(a.y - b.y) < 1e-9) return min(a.x, b.x);
        return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
    }
};

struct EdgeCmp {
    bool operator()(const SweepEdge& e1, const SweepEdge& e2) const {
        if (e1.a == e2.a && e1.b == e2.b) return false;
        
        double x1 = e1.xAt(global_sweepY);
        double x2 = e2.xAt(global_sweepY);
        
        if (fabs(x1 - x2) > 1e-9) return x1 < x2;
        
        // Exact X match: use inverse slope tie-breaker
        double m1 = (e1.b.x - e1.a.x) / (e1.a.y - e1.b.y + 1e-9);
        double m2 = (e2.b.x - e2.a.x) / (e2.a.y - e2.b.y + 1e-9);

        // At top vertex
        if (fabs(global_sweepY - e1.a.y) < 1e-9 || fabs(global_sweepY - e2.a.y) < 1e-9) {
            if (fabs(m1 - m2) > 1e-9) return m1 < m2;
        } else {
            // At bottom vertex
            if (fabs(m1 - m2) > 1e-9) return m1 > m2;
        }

        if (e1.a.x != e2.a.x) return e1.a.x < e2.a.x;
        if (e1.a.y != e2.a.y) return e1.a.y < e2.a.y;
        if (e1.b.x != e2.b.x) return e1.b.x < e2.b.x;
        return e1.b.y < e2.b.y;
    }
};

// ─── Sweep Event ──────────────────────────────────────────
struct SweepVertex {
    Point      p;
    Point      prev;     // previous neighbor in polygon ring
    Point      next;     // next neighbor in polygon ring
    VertexType type;
    int        idx;      // index in merged vertex array
    bool       isHole;
};

// ─── BST comparator (Replaced by linear scan for safety) ─
// We replaced the std::map with std::vector because x-coordinates
// change dynamically as sweepY moves and map keys cannot be updated.

// ─── Diagonal ─────────────────────────────────────────────
struct Diagonal {
    Point u, v;
    bool isBridge = false; // true when this is a hole-to-outer bridge (must be applied first)
};

// ─── Main Sweep Class ─────────────────────────────────────
class SweepLine {
public:
    // inputs
    vector<Point>         outerBoundary;  // CCW
    vector<vector<Point>> holes;          // each CW

    // outputs
    vector<Diagonal>      diagonals;      // monotone decomp cuts

    // run the combined bridge + y-monotone sweep
    void run();

private:
    double                sweepY;
    set<SweepEdge, EdgeCmp> status;  // active edges
    vector<SweepVertex>   events;

    void buildEvents();
    void classifyAll();

    void handleStart  (const SweepVertex& v);
    void handleEnd    (const SweepVertex& v);
    void handleSplit  (const SweepVertex& v);  // also handles hole top
    void handleMerge  (const SweepVertex& v);  // also handles hole bottom
    void handleRegular(const SweepVertex& v);

    // BST operations -> Vector operations
    void   insertEdge(const Point& a, const Point& b, Point helper);
    void   removeEdge(const Point& a, const Point& b);
    const SweepEdge* getEdge(const Point& a, const Point& b);
    const SweepEdge* findLeftEdge(const Point& p); // edge directly left of p

    void   addDiagonal(const Point& u, const Point& v);
};