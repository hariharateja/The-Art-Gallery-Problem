#include "triangulate.h"

// ─── Split into left and right chains ────────────────────
pair<vector<Point>, vector<Point>>
getMonotoneChains(const vector<Point>& poly) {
    int n = poly.size();

    // find topmost and bottommost vertices
    int topIdx = 0, botIdx = 0;
    for (int i = 1; i < n; i++) {
        if (poly[i] < poly[topIdx]) topIdx = i; // higher y
        if (poly[botIdx] < poly[i]) botIdx = i; // lower y
    }

    // left chain: topIdx → botIdx going one way
    // right chain: topIdx → botIdx going other way
    vector<Point> left, right;

    // walk from top to bottom in CCW order → left chain
    int i = topIdx;
    while (i != botIdx) {
        left.push_back(poly[i]);
        i = (i + 1) % n;
    }
    left.push_back(poly[botIdx]);

    // walk from top to bottom in CW order → right chain
    i = topIdx;
    while (i != botIdx) {
        right.push_back(poly[i]);
        i = (i - 1 + n) % n;
    }
    right.push_back(poly[botIdx]);

    return {left, right};
}

// ─── Check if turn is valid for triangulation ─────────────
bool isValidDiagonal(const Point& u, const Point& v,
                      const Point& w, bool isLeftChain) {
    double c = cross(u, v, w);
    if (isLeftChain) return c < -1e-9;  // strict right turn on left chain
    else             return c > 1e-9;   // strict left turn on right chain
}

// ─── Triangulate a single monotone polygon ────────────────
vector<Triangle> triangulateMonotone(vector<Point> poly) {
    vector<Triangle> result;
    int n = poly.size();
    if (n < 3) return result;
    if (n == 3) {
        result.push_back({poly[0], poly[1], poly[2]});
        return result;
    }

    // get left and right chains (both top → bottom)
    auto [left, right] = getMonotoneChains(poly);

    // merge chains into single sorted sequence
    // tag each vertex: 0 = left chain, 1 = right chain
    vector<pair<Point,int>> sorted;
    int li = 0, ri = 0;
    while (li < (int)left.size() && ri < (int)right.size()) {
        if (left[li] < right[ri]) {          // left vertex higher
            sorted.push_back({left[li++], 0});
        } else if (right[ri] < left[li]) {   // right vertex higher
            sorted.push_back({right[ri++], 1});
        } else {
            sorted.push_back({left[li++],  0}); // top or bottom
            if (ri < (int)right.size())
                sorted.push_back({right[ri++], 1});
        }
    }
    while (li < (int)left.size())  sorted.push_back({left[li++],  0});
    while (ri < (int)right.size()) sorted.push_back({right[ri++], 1});

    // stack-based triangulation
    stack<pair<Point,int>> stk;
    stk.push(sorted[0]);
    stk.push(sorted[1]);

    for (int i = 2; i < (int)sorted.size() - 1; i++) {
        auto [v, vChain] = sorted[i];

        if (stk.top().second != vChain) {
            // different chain → connect to all stack vertices
            pair<Point,int> last = stk.top();
            while (stk.size() > 1) {
                auto [u, uChain] = stk.top(); stk.pop();
                auto [w, wChain] = stk.top();
                if (fabs(cross(v, u, w)) > 1e-9) {
                    result.push_back({v, u, w});
                }
            }
            stk.pop();
            stk.push(last);
            stk.push({v, vChain});
        } else {
            // same chain → pop while valid diagonal
            pair<Point,int> last = stk.top(); stk.pop();
            while (!stk.empty()) {
                auto [u, uChain] = stk.top();
                if (isValidDiagonal(v, last.first, u, vChain == 0)) {
                    if (fabs(cross(v, last.first, u)) > 1e-9) {
                        result.push_back({v, last.first, u});
                    }
                    last = stk.top();
                    stk.pop();
                } else break;
            }
            stk.push(last);
            stk.push({v, vChain});
        }
    }

    // process last vertex → connect to all remaining stack vertices
    auto [v, vChain] = sorted.back();
    stk.pop(); // skip top
    while (stk.size() > 1) {
        auto [u, uChain] = stk.top(); stk.pop();
        auto [w, wChain] = stk.top();
        if (fabs(cross(v, u, w)) > 1e-9) {
            result.push_back({v, u, w});
        }
    }

    return result;
}

// ─── Triangulate all monotone pieces ─────────────────────
vector<Triangle> triangulateAll(const vector<vector<Point>>& pieces) {
    vector<Triangle> all;
    for (auto& piece : pieces) {
        auto tris = triangulateMonotone(piece);
        all.insert(all.end(), tris.begin(), tris.end());
    }
    return all;
}
