#include "dcel.h"

// ─── Add a vertex ─────────────────────────────────────────
Vertex* DCEL::addVertex(const Point& p) {
    Vertex* v = new Vertex(p);
    vertices.push_back(v);
    return v;
}

// ─── Add a half-edge pair u→v and v→u ────────────────────
HalfEdge* DCEL::addHalfEdgePair(Vertex* u, Vertex* v) {
    HalfEdge* he1 = new HalfEdge(); // u → v
    HalfEdge* he2 = new HalfEdge(); // v → u
    he1->origin = u;
    he2->origin = v;
    he1->twin   = he2;
    he2->twin   = he1;
    if (!u->incident) u->incident = he1;
    if (!v->incident) v->incident = he2;
    halfEdges.push_back(he1);
    halfEdges.push_back(he2);
    return he1;
}

// ─── Build DCEL from outer boundary + holes ───────────────
void DCEL::build(const vector<Point>& outer,
                 const vector<vector<Point>>& holes) {

    // helper: build a ring of half-edges for a polygon
    // returns the half-edges in order
    auto buildRing = [&](const vector<Point>& pts, Face* f)
                        -> vector<HalfEdge*> {
        int n = pts.size();
        vector<Vertex*>   verts(n);
        vector<HalfEdge*> edges(n);

        for (int i = 0; i < n; i++)
            verts[i] = addVertex(pts[i]);

        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            edges[i] = addHalfEdgePair(verts[i], verts[j]);
            edges[i]->face = f;
        }
        // link next / prev within the ring
        for (int i = 0; i < n; i++) {
            edges[i]->next = edges[(i + 1) % n];
            edges[i]->prev = edges[(i - 1 + n) % n];
        }
        // link twin edges in reverse order so addDiagonal can walk them
        for (int i = 0; i < n; i++) {
            edges[i]->twin->next = edges[(i - 1 + n) % n]->twin;
            edges[i]->twin->prev = edges[(i + 1) % n]->twin;
        }
        return edges;
    };

    // ── Outer boundary face ────────────────────────────────
    Face* outerFace = new Face();
    outerFace->isHole = false;
    auto outerEdges = buildRing(outer, outerFace);
    outerFace->outer = outerEdges[0];
    faces.push_back(outerFace);

    // ── Unbounded face (outside outer boundary) ────────────
    Face* unbounded = new Face();
    unbounded->isHole = true;
    unbounded->outer  = nullptr;
    faces.push_back(unbounded);

    // link twin edges to unbounded face
    for (auto he : outerEdges)
        he->twin->face = unbounded;

    // ── Hole faces ────────────────────────────────────────
    for (auto& hole : holes) {
        Face* holeFace = new Face();
        holeFace->isHole = true;
        
        // Standard CW hole edges keep the walkable exterior on their LEFT
        auto holeEdges = buildRing(hole, outerFace);
        holeFace->outer = holeEdges[0]->twin; // Twin edges bound the hole interior
        faces.push_back(holeFace);

        // twin CCW edges keep the void hole on their LEFT
        for (auto he : holeEdges)
            he->twin->face = holeFace;
    }
}

static bool isAngleInterior(const Point& a, const Point& b, const Point& c, const Point& d) {
    double cp = cross(a, b, c);
    double cp1 = cross(a, b, d);
    double cp2 = cross(b, c, d);
    
    // The interior of the face is always to the LEFT of the boundary traversing A -> B -> C.
    // If the angle A->B->C makes a left turn (cross > 0), the internal angle is convex (< 180).
    // The ray B->D must be strictly left of A->B and strictly left of B->C.
    if (cp > 1e-9) { 
        return (cp1 > 1e-9) && (cp2 > 1e-9);
    } 
    // If it makes a right turn (cross <= 0), the internal angle is reflex (>= 180).
    // The ray B->D only needs to be left of A->B OR left of B->C.
    else if (cp < -1e-9) { 
        return (cp1 > 1e-9) || (cp2 > 1e-9);
    } 
    else { 
        return cp1 > 1e-9;
    }
}

// ─── Add diagonal or bridge between two vertices ──────────
void DCEL::addDiagonal(Vertex* u, Vertex* v) {
    // Find half-edges leaving u and v in the SAME walkable face (!isHole), geometrically validated
    HalfEdge* hu = nullptr;
    HalfEdge* curU = u->incident;
    do {
        if (curU->face && !curU->face->isHole) {
            Point p_in = curU->prev->origin->p;
            Point p_out = curU->twin->origin->p;
            if (isAngleInterior(p_in, u->p, p_out, v->p)) { hu = curU; break; }
        }
        curU = curU->twin->next;
    } while (curU != u->incident);

    HalfEdge* hv = nullptr;
    HalfEdge* curV = v->incident;
    do {
        if (curV->face && !curV->face->isHole) {
            Point p_in = curV->prev->origin->p;
            Point p_out = curV->twin->origin->p;
            if (isAngleInterior(p_in, v->p, p_out, u->p)) { hv = curV; break; }
        }
        curV = curV->twin->next;
    } while (curV != v->incident);

    // Initial fallback if strict interior check failed due to collinearities
    if (!hu) {
        curU = u->incident;
        do {
            if (curU->face && !curU->face->isHole) { hu = curU; break; }
            curU = curU->twin->next;
        } while (curU != u->incident);
    }
    if (!hv) {
        curV = v->incident;
        do {
            if (curV->face && !curV->face->isHole) { hv = curV; break; }
            curV = curV->twin->next;
        } while (curV != v->incident);
    }

    if (!hu || !hv) return; // they do not share a walkable face

    if (hu->face != hv->face) {
        // One ring may be an orphaned hole ring (face->outer == nullptr) that was
        // never connected to the active face because a prior face-split occurred
        // BEFORE this bridge was applied.  Re-adopt the orphaned ring into the
        // active face so the bridge can proceed.
        if (!hu->face->isHole && !hu->face->outer) {
            // hu's ring is orphaned – re-adopt into hv's active face
            HalfEdge* e = hu;
            do { e->face = hv->face; e = e->next; } while (e != hu);
        } else if (!hv->face->isHole && !hv->face->outer) {
            // hv's ring is orphaned – re-adopt into hu's active face
            HalfEdge* e = hv;
            do { e->face = hu->face; e = e->next; } while (e != hv);
        } else {
            return; // genuinely different active faces – skip
        }
    }

    // Check if v is on the SAME boundary component as u
    bool sameComponent = false;
    HalfEdge* tmp = hu;
    do {
        if (tmp == hv) { sameComponent = true; break; }
        tmp = tmp->next;
    } while (tmp != hu);

    // Create the two new half-edges
    HalfEdge* d1 = new HalfEdge(); // u → v
    HalfEdge* d2 = new HalfEdge(); // v → u
    d1->twin = d2; d2->twin = d1;
    d1->origin = u;
    d2->origin = v;

    if (sameComponent) {
        // --- DIAGONAL (face split) ---

        Face* oldFace = hu->face;

        Face* f1 = new Face(); f1->isHole = false;
        Face* f2 = new Face(); f2->isHole = false;

        d1->next = hv;       d1->prev = hu->prev;
        d2->next = hu;       d2->prev = hv->prev;
        hu->prev->next = d1; hv->prev->next = d2;
        hu->prev = d2;       hv->prev = d1;

        d1->face = f1; d2->face = f2;
        f1->outer = d1; f2->outer = d2;

        HalfEdge* e = d1; do { e->face = f1; e = e->next; } while (e != d1);
        e = d2; do { e->face = f2; e = e->next; } while (e != d2);

        faces.push_back(f1);
        faces.push_back(f2);
        
        // Disable the old face so it's not extracted
        oldFace->outer = nullptr;
    } else {
        // --- BRIDGE (component merge) ---
        d1->next = hv;       d1->prev = hu->prev;
        d2->next = hu;       d2->prev = hv->prev;
        hu->prev->next = d1; hv->prev->next = d2;
        hu->prev = d2;       hv->prev = d1;

        Face* merged = hu->face;
        d1->face = merged;
        d2->face = merged;
        HalfEdge* e = d1; do { e->face = merged; e = e->next; } while (e != d1);
        merged->outer = d1;
    }

    halfEdges.push_back(d1);
    halfEdges.push_back(d2);
}

// ─── Extract each face as list of Points ─────────────────
vector<vector<Point>> DCEL::extractFaces() {
    vector<vector<Point>> result;
    for (auto f : faces) {
        if (f->isHole || !f->outer) continue;
        vector<Point> poly;
        HalfEdge* start = f->outer;
        HalfEdge* cur   = start;
        do {
            poly.push_back(cur->origin->p);
            cur = cur->next;
        } while (cur != start);
        if (poly.size() >= 3)
            result.push_back(poly);
    }
    return result;
}

// ─── Destructor ───────────────────────────────────────────
DCEL::~DCEL() {
    for (auto v : vertices)  delete v;
    for (auto e : halfEdges) delete e;
    for (auto f : faces)     delete f;
}