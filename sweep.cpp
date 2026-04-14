#include "sweep.h"

double global_sweepY = 0;// ─── Add diagonal to output ───────────────────────────────
void SweepLine::addDiagonal(const Point& u, const Point& v) {
    diagonals.push_back({u, v, false});
}

// ─── Build event queue from all vertices ─────────────────
void SweepLine::buildEvents() {
    events.clear();

    // outer boundary vertices
    int n = outerBoundary.size();
    for (int i = 0; i < n; i++) {
        SweepVertex sv;
        sv.p      = outerBoundary[i];
        sv.prev   = outerBoundary[(i - 1 + n) % n];   // polygon neighbor
        sv.next   = outerBoundary[(i + 1) % n];        // polygon neighbor
        sv.idx    = i;
        sv.isHole = false;
        sv.type   = classifyVertex(sv.prev, sv.p, sv.next, false);
        events.push_back(sv);
    }

    // hole vertices
    int offset = n;
    for (auto& hole : holes) {
        int m = hole.size();
        for (int i = 0; i < m; i++) {
            SweepVertex sv;
            sv.p      = hole[i];
            sv.prev   = hole[(i - 1 + m) % m];
            sv.next   = hole[(i + 1) % m];
            sv.idx    = offset + i;
            sv.isHole = true;
            sv.type   = classifyVertex(sv.prev, sv.p, sv.next, true);
            events.push_back(sv);
        }
        offset += m;
    }

    // sort top → bottom, left → right for ties
    sort(events.begin(), events.end(),
         [](const SweepVertex& a, const SweepVertex& b){
             return a.p < b.p;
         });
}

void SweepLine::insertEdge(const Point& a, const Point& b, Point helper) {
    status.insert(SweepEdge(a, b, helper));
}

void SweepLine::removeEdge(const Point& a, const Point& b) {
    SweepEdge query(a, b, Point());
    status.erase(query);
}

const SweepEdge* SweepLine::getEdge(const Point& a, const Point& b) {
    SweepEdge query(a, b, Point());
    auto it = status.find(query);
    if (it != status.end()) return &(*it);
    return nullptr;
}

const SweepEdge* SweepLine::findLeftEdge(const Point& p) {
    SweepEdge dummy(Point(p.x, p.y + 1.0), Point(p.x, p.y - 1.0), Point());
    auto it = status.lower_bound(dummy);
    
    while (it != status.begin()) {
        --it;
        if (it->a != p && it->b != p) {
            if (it->xAt(global_sweepY) < p.x - 1e-9) {
                return &(*it);
            }
        }
    }
    return nullptr;
}

// ─── Handlers ─────────────────────────────────────────────
void SweepLine::handleStart(const SweepVertex& v) {
    // Both edges originate at v.p; temporarily offset sweepY downward
    // so xAt() separates their keys before they diverge.
    double savedY = sweepY;
    sweepY -= 1e-7;
    insertEdge(v.p, v.prev, v.p);
    insertEdge(v.p, v.next, v.p);
    sweepY = savedY;
}

void SweepLine::handleEnd(const SweepVertex& v) {
    const SweepEdge* edgePrev = getEdge(v.p, v.prev);
    if (edgePrev && edgePrev->helperIsMerge) addDiagonal(v.p, edgePrev->helper);
    const SweepEdge* edgeNext = getEdge(v.p, v.next);
    if (edgeNext && edgeNext->helperIsMerge) addDiagonal(v.p, edgeNext->helper);

    removeEdge(v.p, v.prev);
    removeEdge(v.p, v.next);
}

void SweepLine::handleSplit(const SweepVertex& v) {
    // Handles BOTH regular split vertices (outer)
    // AND topmost vertices of holes → bridge creation!
    const SweepEdge* left = findLeftEdge(v.p);
    if (left) {
        // Mark as bridge when this is a hole's topmost SPLIT vertex
        diagonals.push_back({v.p, left->helper, v.isHole});
        left->helper     = v.p;
        left->helperIsMerge = false;
    }
    insertEdge(v.p, v.prev, v.p);
    insertEdge(v.p, v.next, v.p);
}

void SweepLine::handleMerge(const SweepVertex& v) {
    // Check helpers of both incident edges before removing them
    // (same pattern as handleEnd — missing this was causing incomplete decomposition)
    const SweepEdge* edgePrev = getEdge(v.p, v.prev);
    if (edgePrev && edgePrev->helperIsMerge) addDiagonal(v.p, edgePrev->helper);
    const SweepEdge* edgeNext = getEdge(v.p, v.next);
    if (edgeNext && edgeNext->helperIsMerge) addDiagonal(v.p, edgeNext->helper);

    removeEdge(v.p, v.prev);
    removeEdge(v.p, v.next);

    const SweepEdge* left = findLeftEdge(v.p);
    if (left && left->helperIsMerge) addDiagonal(v.p, left->helper);
    if (left) {
        left->helper     = v.p;
        left->helperIsMerge = true;
    }
}

void SweepLine::handleRegular(const SweepVertex& v) {
    bool prevAbove = isAbove(v.prev, v.p);
    Point above = prevAbove ? v.prev : v.next;
    Point below = prevAbove ? v.next : v.prev;

    // Is the interior to the right?
    // Determine by checking classification and edge direction.
    // If we're going down the left boundary, prev is above, next is below.
    // So the edge 'above' is literally the bounding edge on the left.
    // But since testing is cheap, we just check the edge above since it's ending at v.
    const SweepEdge* edgeAbove = getEdge(v.p, above);
    if (edgeAbove && edgeAbove->helperIsMerge) {
        addDiagonal(v.p, edgeAbove->helper);
    }
    removeEdge(v.p, above);

    // If we are on the right boundary (interior to the left), we must also update the left edge
    const SweepEdge* left = findLeftEdge(v.p);
    // standard algorithm: interior to the right -> polygon is on the right -> we are the left boundary.
    // If we are left boundary, we don't look at `left`.
    // Actually, CCW left boundary means prev is above. CW hole left boundary means prev is below.
    bool leftBoundary = false;
    if (!v.isHole) {
        leftBoundary = prevAbove; // outer CCW: left boundary goes down (prev is above)
    } else {
        leftBoundary = !prevAbove; // hole CW: left boundary goes up (next is above)
    }

    if (leftBoundary) {
        insertEdge(v.p, below, v.p);
    } else {
        insertEdge(v.p, below, v.p);
        if (left) {
            if (left->helperIsMerge) addDiagonal(v.p, left->helper);
            left->helper = v.p;
            left->helperIsMerge = false;
        }
    }
}

// ─── Main Run ─────────────────────────────────────────────
void SweepLine::run() {
    buildEvents();

    for (auto& v : events) {
        sweepY = v.p.y;
        global_sweepY = sweepY;

        switch (v.type) {
            case START:   handleStart(v);   break;
            case END:     handleEnd(v);     break;
            case SPLIT:   handleSplit(v);   break;  // ← bridges holes!
            case MERGE:   handleMerge(v);   break;
            case REGULAR: handleRegular(v); break;
        }
    }
}
