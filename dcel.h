#pragma once
#include "geometry.h"

// ─── DCEL Structures ──────────────────────────────────────
// A Doubly Connected Edge List represents the polygon as:
//   Vertices → HalfEdges → Faces
// Each edge is split into 2 opposite half-edges

struct HalfEdge;
struct Face;

struct Vertex {
    Point p;
    HalfEdge* incident;  // one outgoing half-edge from this vertex

    Vertex(Point p = {}) : p(p), incident(nullptr) {}
};

struct HalfEdge {
    Vertex*   origin;    // start vertex
    HalfEdge* twin;      // opposite half-edge
    HalfEdge* next;      // next half-edge in same face (CCW)
    HalfEdge* prev;      // previous half-edge in same face
    Face*     face;      // face this half-edge bounds

    HalfEdge() : origin(nullptr), twin(nullptr),
                 next(nullptr), prev(nullptr), face(nullptr) {}
};

struct Face {
    HalfEdge* outer;     // one half-edge on boundary (nullptr = unbounded)
    bool      isHole;    // true if this face is a hole

    Face() : outer(nullptr), isHole(false) {}
};

// ─── DCEL ─────────────────────────────────────────────────
struct DCEL {
    vector<Vertex*>   vertices;
    vector<HalfEdge*> halfEdges;
    vector<Face*>     faces;

    // ── Build from input ───────────────────────────────────
    // outer: CCW ordered vertices of outer boundary
    // holes: CW ordered vertices of each hole
    void build(const vector<Point>& outer,
               const vector<vector<Point>>& holes);

    // ── Add a diagonal between two vertices ───────────────
    // Used during sweep to record monotone decomposition cuts
    // This function automatically detects if it's a bridge or face-splitting diagonal
    void addDiagonal(Vertex* u, Vertex* v);

    // ── Extract all faces as vertex lists ─────────────────
    // Used by triangulator to get each monotone polygon
    vector<vector<Point>> extractFaces();

    // ── Helpers ───────────────────────────────────────────
    Vertex*   addVertex(const Point& p);
    HalfEdge* addHalfEdgePair(Vertex* u, Vertex* v);

    // ── Destructor ────────────────────────────────────────
    ~DCEL();
};