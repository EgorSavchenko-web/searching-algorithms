# Ring Destroyer

## Overview

This repository contains the implementation for **Ring Destroyer**. The goal is to guide **Frodo Baggins** through a 13×13 grid world of Middle-earth, find **Gollum**, and then reach **Mount Doom** to destroy the One Ring—all while avoiding lethal enemies and strategically using the Ring and Mithril Mail‑coat.

Two search algorithms are implemented:

- **Backtracking** (DFS with pruning)
- **A\*** (with Manhattan heuristic)

The project includes a **test harness** that evaluates both algorithms on 1000 randomly generated maps, collects performance statistics, and identifies impossible maps.

### Expected Output

The tester prints:

1. **Statistics** for each algorithm and variant (mean/median/mode time, wins/losses, percentages)
2. **Impossible maps** (maps that both algorithms declared unsolvable) displayed as grids

## Code Structure

### `astar.cpp`

- Implements the **A\*** search algorithm.
- State space: `(x, y, ring_active, has_mithril)`.
- Heuristic: Manhattan distance to current target (Gollum or Mount Doom).
- Handles ring toggling and Mithril pickup as zero‑cost actions.
- Includes a fallback exploration strategy when no path is found.

### `backtracking.cpp`

- Implements a **depth‑first backtracking** search with pruning and memoization.
- Uses branch‑and‑bound to cut off suboptimal paths.
- Memoization table: `best_distance[x][y][ring][mithril]`.
- Explores safe neighboring cells first, then unknown ones.

### `tester.cpp`

- **Map generator**: creates random maps with enemies, Gollum, Mount Doom, and Mithril.
- **Interactor**: simulates the game environment, communicates via pipes with the agents.
- **Statistics collector**: computes mean, median, mode, standard deviation, win/loss percentages.
- **Impossible‑map detector**: saves maps that both algorithms failed to solve.

## Algorithms

### A\* (astar.cpp)

- **Heuristic**: Manhattan distance to target.
- **State expansion**: orthogonal moves (cost 1) and ring toggles (cost 0).
- **Safety check**: `is_dangerous()` evaluates enemy perception zones under current ring/mithril state.
- **Exploration fallback**: when no path exists, moves toward the cell revealing the most unseen tiles.

### Backtracking (backtracking.cpp)

- **DFS** with pruning via `best_distance` table.
- **Branch‑and‑bound**: abandons paths longer than the current best.
- **Order of expansion**: known safe cells first, then unknown cells.
- **Backtracking moves**: physically moves the agent back during search.

## Statistical Analysis

The tester runs each algorithm on **1000 random maps** for **both perception variants** (Variant 1: radius 1, Variant 2: radius 2).

### Collected Metrics

- **Execution time** (mean, median, mode, standard deviation)
- **Number of wins** (successful destruction of the Ring)
- **Number of losses** (agent died or declared map unsolvable)
- **Win/Loss percentages**

### Comparison

Results are compared between:

- Backtracking vs. A\* for each variant
- Variant 1 vs. Variant 2 for each algorithm

The statistics are printed in tables, and impossible maps are displayed for further analysis.

## Limitations & Assumptions

- **Windows‑only**: The tester uses Windows pipes (`CreatePipe`). For Linux/macOS, replace with `fork()` and `pipe()`.
- **Deterministic randomness**: Maps are generated using `std::random_device` and `mt19937`.
- **Enemy placement**: Enemies never overlap, and key items (Gollum, Mount Doom, Mithril) are placed in safe cells.
- **Perception**: The agent only perceives cells within the defined Moore radius; the rest of the map is unknown.
- **Ring & Mithril effects**: Implemented as described in the assignment spec.
