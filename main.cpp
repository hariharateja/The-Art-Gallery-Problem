#include "geometry.h"
#include "dcel.h"
#include "sweep.h"
#include "triangulate.h"
#include <iomanip>

// ─── 3-Coloring ───────────────────────────────────────────
// Returns a map from vertex coordinate → color (0/1/2),
// avoiding the index mismatch when caller rebuilds a separate vid map.
map<pair<double,double>, int> threeColor(const vector<Triangle>& tris) {
    map<pair<double,double>, int> vid;
    auto getId = [&](const Point& p) -> int {
        auto key = make_pair(p.x, p.y);
        if (!vid.count(key)) { int id = vid.size(); vid[key] = id; }
        return vid[key];
    };

    for (auto& t : tris) { getId(t.a); getId(t.b); getId(t.c); }
    int V = vid.size();
    vector<int> color(V, -1);

    // build triangle adjacency in O(n log n)
    int n = tris.size();
    vector<vector<int>> adj(n);
    map<pair<int, int>, vector<int>> edgeToTris;

    for (int i = 0; i < n; i++) {
        int v1 = getId(tris[i].a);
        int v2 = getId(tris[i].b);
        int v3 = getId(tris[i].c);
        
        edgeToTris[{min(v1, v2), max(v1, v2)}].push_back(i);
        edgeToTris[{min(v2, v3), max(v2, v3)}].push_back(i);
        edgeToTris[{min(v3, v1), max(v3, v1)}].push_back(i);
    }

    for (auto& pair_item : edgeToTris) {
        if (pair_item.second.size() == 2) {
            int t1 = pair_item.second[0];
            int t2 = pair_item.second[1];
            adj[t1].push_back(t2);
            adj[t2].push_back(t1);
        }
    }

    // color first triangle
    if (!tris.empty()) {
        color[getId(tris[0].a)] = 0;
        color[getId(tris[0].b)] = 1;
        color[getId(tris[0].c)] = 2;
    }

    // BFS to propagate colors
    vector<bool> visited(n, false);
    queue<int> q;
    if (!tris.empty()) { q.push(0); visited[0] = true; }

    while (!q.empty()) {
        int i = q.front(); q.pop();
        vector<Point> pts = {tris[i].a, tris[i].b, tris[i].c};

        // find missing color for this triangle
        set<int> used;
        for (auto& p : pts) if (color[getId(p)] != -1)
            used.insert(color[getId(p)]);
        int missing = -1;
        for (int c = 0; c <= 2; c++)
            if (!used.count(c)) { missing = c; break; }
        for (auto& p : pts)
            if (color[getId(p)] == -1) color[getId(p)] = missing;

        for (int j : adj[i]) {
            if (!visited[j]) {
                visited[j] = true;
                // copy shared vertex colors to neighbor
                vector<Point> npts = {tris[j].a, tris[j].b, tris[j].c};
                for (auto& np : npts)
                    for (auto& ip : pts)
                        if (np == ip) color[getId(np)] = color[getId(ip)];
                q.push(j);
            }
        }
    }

    // return coordinate → color map directly (no caller vid rebuild needed)
    map<pair<double,double>, int> colorMap;
    for (auto& [key, id] : vid)
        colorMap[key] = color[id];
    return colorMap;
}

// ─── Centroid ─────────────────────────────────────────────
Point centroid(const Triangle& t) {
    return Point((t.a.x+t.b.x+t.c.x)/3.0,
                 (t.a.y+t.b.y+t.c.y)/3.0);
}

// ─── Main ─────────────────────────────────────────────────
int main() {
    // freopen("input.txt", "r", stdin);
    // freopen("output.txt", "w", stdout);
    int T;
    cin >> T;

    while (T--) {
        // ── Read outer boundary ───────────────────────────
        int v0; cin >> v0;
        vector<Point> outer(v0);
        for (int i = 0; i < v0; i++) {
            cin >> outer[i].x >> outer[i].y;
            outer[i].id = i;
        }

        // ── Read holes ────────────────────────────────────
        int h; cin >> h;
        vector<vector<Point>> holes(h);
        int offset = v0;
        for (int i = 0; i < h; i++) {
            int vi; cin >> vi;
            holes[i].resize(vi);
            for (int j = 0; j < vi; j++) {
                cin >> holes[i][j].x >> holes[i][j].y;
                holes[i][j].id = offset + j;
            }
            offset += vi;
        }

        // Remove consecutive duplicate vertices (zero-length edges break sweep/DCEL)
        auto dedup = [](vector<Point>& pts) {
            pts.erase(unique(pts.begin(), pts.end(),
                [](const Point& a, const Point& b){ return a.x==b.x && a.y==b.y; }),
                pts.end());
        };
        dedup(outer);
        for (auto& hole : holes) dedup(hole);

        // Apply robust mathematical infinitesimal shear to break all perfectly horizontal/collinear degenerate ties
        for (auto& p : outer) p.y += p.x * 1e-10;
        for (auto& hole : holes) {
            for (auto& p : hole) {
                p.y += p.x * 1e-10;
            }
        }

        // Recount after dedup
        int n = (int)outer.size();
        for (auto& hole : holes) n += (int)hole.size();

        // ── Ensure correct orientations ───────────────────
        // outer boundary must be CCW
        double area = 0;
        int v0_actual = (int)outer.size();
        for (int i = 0; i < v0_actual; i++) {
            int j = (i+1) % v0_actual;
            area += outer[i].x * outer[j].y;
            area -= outer[j].x * outer[i].y;
        }
        if (area < 0) reverse(outer.begin(), outer.end()); // make CCW

        // holes must be CW
        for (auto& hole : holes) {
            double harea = 0;
            int m = hole.size();
            for (int i = 0; i < m; i++) {
                int j = (i+1) % m;
                harea += hole[i].x * hole[j].y;
                harea -= hole[j].x * hole[i].y;
            }
            if (harea > 0) reverse(hole.begin(), hole.end()); // make CW
        }

        // ── Run sweep (bridge + y-monotone) ──────────────
        SweepLine sweep;
        sweep.outerBoundary = outer;
        sweep.holes         = holes;
        sweep.run();

        // ── Build DCEL and add diagonals ──────────────────
        DCEL dcel;
        dcel.build(outer, holes);
        // map Point → Vertex* for diagonal insertion
        map<pair<double,double>, Vertex*> vertMap;
        for (auto v : dcel.vertices)
            vertMap[{v->p.x, v->p.y}] = v;

        // Apply bridge diagonals first so all holes are merged into the outer face
        // before any face-splitting diagonals run.
        for (auto& d : sweep.diagonals) {
            if (!d.isBridge) continue;
            auto it1 = vertMap.find({d.u.x, d.u.y});
            auto it2 = vertMap.find({d.v.x, d.v.y});
            if (it1 == vertMap.end() || it2 == vertMap.end()) continue;
            dcel.addDiagonal(it1->second, it2->second);
        }
        // Then apply all remaining (face-splitting) diagonals
        for (auto& d : sweep.diagonals) {
            if (d.isBridge) continue;
            auto it1 = vertMap.find({d.u.x, d.u.y});
            auto it2 = vertMap.find({d.v.x, d.v.y});
            if (it1 == vertMap.end() || it2 == vertMap.end()) continue;
            dcel.addDiagonal(it1->second, it2->second);
        }
        auto monotonePieces = dcel.extractFaces();
        vector<Triangle> triangles;
        for (auto& piece : monotonePieces) {
            auto tris = triangulateMonotone(piece);
            triangles.insert(triangles.end(), tris.begin(), tris.end());
        }
        // ── Output triangles ──────────────────────────────
        cout << "TRIANGLES " << triangles.size() << "\n";
        for (auto& t : triangles) {
            cout << fixed << setprecision(2)
                 << "(" << t.a.x << "," << t.a.y - t.a.x * 1e-10 << ") "
                 << "(" << t.b.x << "," << t.b.y - t.b.x * 1e-10 << ") "
                 << "(" << t.c.x << "," << t.c.y - t.c.x * 1e-10 << ")\n";
        }

        // ── Compute guard bound ───────────────────────────
        int bound = (n + 2*h) / 3;

        // 3-color and find smallest color class
        // colorMap is keyed by coordinate, so no separate vid rebuild needed
        auto colorMap = threeColor(triangles);

        map<int, vector<Point>> colorClass;
        for (auto& t : triangles)
            for (auto& p : {t.a, t.b, t.c}) {
                auto it = colorMap.find({p.x, p.y});
                if (it != colorMap.end() && it->second != -1)
                    colorClass[it->second].push_back(p);
            }

        // deduplicate each class
        int bestColor = 0, bestSize = INT_MAX;
        for (auto& [c, pts] : colorClass) {
            sort(pts.begin(), pts.end(),
                 [](const Point& a, const Point& b){
                     return make_pair(a.x,a.y) < make_pair(b.x,b.y);
                 });
            pts.erase(unique(pts.begin(), pts.end()), pts.end());
            if ((int)pts.size() < bestSize)
            { bestSize = pts.size(); bestColor = c; }
        }

        // Collect guard points: start from smallest color class,
        // supplement from others if needed to reach exactly `bound`.
        vector<Point> guardPts = colorClass[bestColor];
        if ((int)guardPts.size() > bound) guardPts.resize(bound);
        if ((int)guardPts.size() < bound) {
            set<pair<double,double>> seen;
            for (auto& p : guardPts) seen.insert({p.x, p.y});
            for (int c = 0; c <= 2 && (int)guardPts.size() < bound; c++) {
                if (c == bestColor) continue;
                for (auto& p : colorClass[c]) {
                    if ((int)guardPts.size() >= bound) break;
                    if (!seen.count({p.x, p.y})) {
                        guardPts.push_back(p);
                        seen.insert({p.x, p.y});
                    }
                }
            }
        }

        // ── Output guards ─────────────────────────────────
        cout << "GUARDS " << (int)guardPts.size() << "\n";
        for (auto& p : guardPts)
            cout << fixed << setprecision(2)
                 << "(" << p.x << "," << p.y - p.x * 1e-10 << ")\n";

        // ── Output for visualizer ─────────────────────────
        // outer boundary
        cout << "OUTER " << v0_actual << "\n";
        for (auto& p : outer)
            cout << p.x << " " << p.y - p.x * 1e-10 << "\n";

        // holes
        cout << "HOLES " << h << "\n";
        for (auto& hole : holes) {
            cout << hole.size() << "\n";
            for (auto& p : hole)
                cout << p.x << " " << p.y - p.x * 1e-10 << "\n";
        }

        cout << "\n"; // separate test cases
    }
    return 0;
}