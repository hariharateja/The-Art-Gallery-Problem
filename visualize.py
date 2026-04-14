import sys
try:
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon as MplPolygon
except ImportError:
    print("Error: Matplotlib is not installed.")
    print("Please install it by running: pip install matplotlib")
    sys.exit(1)

def parse_points_line(line):
    """Parses a line of coordinates in the format (x1,y1) (x2,y2) ..."""
    line = line.strip()
    if not line: return []
    pts = []
    parts = line.split(')')
    for p in parts:
        if '(' in p:
            coord = p.split('(')[1].split(',')
            pts.append((float(coord[0]), float(coord[1])))
    return pts

def main():
    if len(sys.argv) > 1:
        f = open(sys.argv[1], 'r')
    else:
        # Defaults to standard input if piped
        f = sys.stdin

    lines = [L.strip() for L in f.readlines() if L.strip()]
    if not lines:
        print("No input provided. Usage:")
        print("  python3 visualize.py output.txt")
        print("  ./main < input.txt | python3 visualize.py")
        return

    idx = 0
    test_cases = []
    current = None

    # Parse the output text format
    while idx < len(lines):
        line = lines[idx]
        
        # Determine if we are starting a completely new test case block.
        # We assume a new case starts if we see a keyword AND 'current' already 
        # has data for that keyword (meaning we've wrapped around to a new case).
        if any(line.startswith(k) for k in ['TRIANGLES', 'GUARDS', 'OUTER', 'HOLES']):
            if current is None:
                current = {'triangles': [], 'guards': [], 'outer': [], 'holes': []}
                test_cases.append(current)
            else:
                key = line.split()[0].lower()
                if len(current[key]) > 0: # We already have data for this in the current case; start new
                    current = {'triangles': [], 'guards': [], 'outer': [], 'holes': []}
                    test_cases.append(current)

        if line.startswith('TRIANGLES'):
            num = int(line.split()[1])
            idx += 1
            for _ in range(num):
                current['triangles'].append(parse_points_line(lines[idx]))
                idx += 1
                
        elif line.startswith('GUARDS'):
            num = int(line.split()[1])
            idx += 1
            for _ in range(num):
                current['guards'].extend(parse_points_line(lines[idx]))
                idx += 1
                
        elif line.startswith('OUTER'):
            num = int(line.split()[1])
            idx += 1
            for _ in range(num):
                parts = lines[idx].split()
                current['outer'].append((float(parts[0]), float(parts[1])))
                idx += 1
                
        elif line.startswith('HOLES'):
            num_holes = int(line.split()[1])
            idx += 1
            for _ in range(num_holes):
                num_pts = int(lines[idx])
                idx += 1
                hole_pts = []
                for _ in range(num_pts):
                    parts = lines[idx].split()
                    hole_pts.append((float(parts[0]), float(parts[1])))
                    idx += 1
                current['holes'].append(hole_pts)
        else:
            idx += 1

    if f is not sys.stdin:
        f.close()

    import matplotlib.colors as mcolors
    colors = list(mcolors.TABLEAU_COLORS.values())

    for case_idx, case in enumerate(test_cases):
        fig, ax = plt.subplots(figsize=(10, 8))
        
        # Track all X and Y coordinates to manually set limits later
        all_x, all_y = [], []
        def track_points(points):
            for p in points:
                all_x.append(p[0])
                all_y.append(p[1])

        # Add triangles
        for i, t in enumerate(case['triangles']):
            track_points(t)
            tri_poly = MplPolygon(t, closed=True, fill=True, alpha=0.5, color=colors[i % len(colors)], edgecolor='black', linewidth=0.5, zorder=1)
            ax.add_patch(tri_poly)

        # Add outer boundary on top
        if case['outer']:
            track_points(case['outer'])
            outer_poly = MplPolygon(case['outer'], closed=True, fill=False, edgecolor='black', linewidth=3, zorder=3)
            ax.add_patch(outer_poly)

        # Overlay white polygon for holes
        for h in case['holes']:
            track_points(h)
            hole_poly = MplPolygon(h, closed=True, fill=True, color='white', edgecolor='black', linewidth=2, zorder=4)
            ax.add_patch(hole_poly)

        # Plot red star markers for guards
        if case['guards']:
            track_points(case['guards'])
            gx = [p[0] for p in case['guards']]
            gy = [p[1] for p in case['guards']]
            ax.scatter(gx, gy, color='red', s=200, marker='*', zorder=5, label='Guards', edgecolors='black')
            ax.legend(loc='upper left')

        # ── Stats info box (top-right corner) ────────────────
        num_vertices = len(set(pt for t in case['triangles'] for pt in [tuple(t[0]), tuple(t[1]), tuple(t[2])]))
        num_triangles = len(case['triangles'])
        num_guards = len(case['guards'])
        num_holes = len(case['holes'])
        info_text = (
            f"Vertices  : {num_vertices}\n"
            f"Triangles : {num_triangles}\n"
            f"Guards    : {num_guards}\n"
            f"Holes     : {num_holes}"
        )
        ax.text(
            0.98, 0.98, info_text,
            transform=ax.transAxes,
            fontsize=10, fontfamily='monospace',
            verticalalignment='top', horizontalalignment='right',
            bbox=dict(boxstyle='round,pad=0.5', facecolor='lightyellow',
                      edgecolor='black', linewidth=1.2, alpha=0.9),
            zorder=10
        )

        # Calculate bounding box with a 5% margin
        if all_x and all_y:
            min_x, max_x = min(all_x), max(all_x)
            min_y, max_y = min(all_y), max(all_y)
            x_margin = (max_x - min_x) * 0.05
            y_margin = (max_y - min_y) * 0.05
            
            # Fallback for perfectly flat shapes or single points
            if x_margin == 0: x_margin = 1.0 
            if y_margin == 0: y_margin = 1.0
            
            ax.set_xlim(min_x - x_margin, max_x + x_margin)
            ax.set_ylim(min_y - y_margin, max_y + y_margin)
        
        # Prettify Graph
        ax.set_aspect('equal', 'box')
        plt.title(f"Polygon Triangulation & Guard Placements (Case {case_idx+1})", fontsize=14, fontweight='bold')
        plt.xlabel("X Coordinates")
        plt.ylabel("Y Coordinates")
        plt.grid(True, linestyle='--', alpha=0.4)
        
        # Rendering
        if len(sys.argv) > 2 and len(test_cases) == 1:
            out_filename = sys.argv[2]
        elif len(sys.argv) > 2:
            base = sys.argv[2]
            if "." in base:
                parts = base.rsplit(".", 1)
                out_filename = f"{parts[0]}_{case_idx+1}.{parts[1]}"
            else:
                out_filename = f"{base}_{case_idx+1}.png"
        else:
            if len(test_cases) == 1:
                out_filename = "visualization.png"
            else:
                out_filename = f"visualization_{case_idx+1}.png"
                
        plt.savefig(out_filename, dpi=300, bbox_inches='tight')
        print(f"Saved visualization to {out_filename}")
        plt.close(fig)

if __name__ == "__main__":
    main()