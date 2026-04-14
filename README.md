# 🖼️ Art Gallery Problem  
### Sweep-Line Triangulation & Guard Placement

This project implements an algorithmic solution to the **Art Gallery Problem**, which determines the minimum number of guards required to observe every point inside an art gallery represented as a polygon (possibly with holes).

According to **Chvátal’s Theorem**, a simple polygon with **n vertices** can always be guarded with at most:

⌊ n / 3 ⌋

For polygons containing holes, the bound becomes:

⌊ (n + 2h) / 3 ⌋

where **h** is the number of holes.

Our implementation computes guard positions by decomposing the polygon, triangulating it, and applying **3-colouring on the triangulation graph**.

---

# 📌 Features

- Handles **simple polygons and polygons with holes**
- Efficient **sweep-line monotone decomposition**
- Robust **DCEL (Doubly Connected Edge List)** implementation
- **Linear-time triangulation** of monotone polygons
- **3-colouring algorithm** to compute optimal guard placements
- **Visualization using Python (Matplotlib)**

---

# 🧠 Algorithm Overview

The solution follows four major stages.

---

## 1️⃣ Sweep-Line Decomposition

A vertical sweep line processes vertices from **top to bottom**, classifying each vertex into one of five types:

- Start  
- End  
- Split  
- Merge  
- Regular  

During the sweep, **bridge diagonals** are inserted to connect holes to the outer boundary when necessary. This converts the polygon with holes into a single connected region that can be decomposed into **y-monotone polygons**.

---

## 2️⃣ DCEL Construction

The polygon is represented using a **Doubly Connected Edge List (DCEL)** data structure.

The DCEL maintains:

- vertices  
- half-edges  
- faces  

This structure allows efficient insertion of diagonals and correct updates when faces split during the decomposition stage.

---

## 3️⃣ Monotone Polygon Triangulation

Each monotone polygon is triangulated using a **stack-based algorithm**.

Steps:

1. Split the polygon into left and right chains.
2. Sort vertices from top to bottom.
3. Use a stack to add valid diagonals.

This triangulates each polygon in **O(n)** time.

---

## 4️⃣ 3-Colouring for Guard Placement

Once triangulation is complete:

1. A **dual graph** of triangles is constructed.
2. The graph is **3-coloured using BFS**.
3. Each vertex receives one of three colours.

The **smallest colour class** represents the **minimum set of guard locations**.

---

# 📂 Project Structure

```
.
├── geometry.h / geometry.cpp
│   Geometric primitives and vertex classification
│
├── sweep.h / sweep.cpp
│   Sweep-line algorithm for monotone decomposition
│
├── dcel.h / dcel.cpp
│   DCEL data structure implementation
│
├── triangulate.h / triangulate.cpp
│   Monotone polygon triangulation
│
├── main.cpp
│   Pipeline driver and guard computation
│
├── visualize.py
│   Matplotlib visualization of triangulation and guards
│
├── Makefile
│   Build instructions
│
├── test.sh
│   Compile + run + visualize helper script
│
└── tests/
    ├── case1.txt
    ├── case2.txt
    ├── case3.txt
```

---

# ⚙️ Build Instructions

Compile the project using `make`.

```bash
make
```

This compiles the project using:

```
g++ -std=c++17 -O2
```

---

# ▶️ Running the Program

Run the program with a test case:

```bash
./main < tests/case1.txt
```

The output contains the following sections:

```
TRIANGLES
GUARDS
OUTER
HOLES
```

These describe the triangulation and computed guard positions.

---

# 📊 Visualization (Recommended)

To generate a visualization of the triangulation and guard placements:

```bash
./test.sh case1.txt
```

This script will:

1. Compile the project
2. Run the algorithm
3. Generate a **PNG visualization**

The visualization shows:

- coloured triangles (triangulation)
- polygon boundary
- holes
- guard locations (red stars)

---

# 🧾 Input Format

```
T
n x1 y1 x2 y2 ... xn yn
h
m x1 y1 ... xm ym
```

Where:

- **T** → number of test cases  
- **n** → vertices of outer polygon (CCW order)  
- **h** → number of holes  
- each hole has **m** vertices (CW order)

---

# 📈 Time Complexity

| Stage | Time Complexity |
|------|----------------|
| Sweep-line decomposition | O(n log n) |
| DCEL construction | O(n log n) |
| Monotone triangulation | O(n) |
| 3-colouring | O(n) |

Total time complexity:

```
O(n log n)
```

Space complexity:

```
O(n)
```

---

# 📷 Example Result

The generated visualization shows:

- coloured triangles filling the polygon interior
- white regions representing holes
- red star markers indicating guard locations
- an information box displaying the number of vertices, triangles, guards, and holes

---

# 👨‍💻 Authors

- Harshith  
- Team Member  

Design and Analysis of Algorithms – Assignment 2  
Art Gallery Problem