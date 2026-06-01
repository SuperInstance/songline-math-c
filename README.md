# songline-math-c

> Australian Aboriginal songline navigation in C — graph pathfinding, corroboree clustering, and tradition evolution.

## What This Does

`songline-math-c` implements navigable knowledge graphs in C, inspired by Australian Aboriginal songlines. It provides graph construction, pathfinding with dreamtime fallback, corroboree (convergence hub) detection, navigability scoring, modularity computation, and tradition evolution. Uses arena allocation for zero-malloc hot paths. Use it for embedded pathfinding, robotics, game AI, or high-performance graph analysis.

## The Cultural Root

See `songline-math` (npm) for the full cultural background. Songlines encode navigation as songs — each verse is a waypoint, and singing traverses the graph.

## Install

```bash
git clone https://github.com/SuperInstance/songline-math-c.git
cd songline-math-c
make
```

## Quick Start

```c
#include "songline_api.h"

int main() {
    // Arena allocation for all operations
    char buf[65536];
    sl_arena arena;
    sl_arena_init(&arena, buf, sizeof(buf));

    // Build a knowledge graph
    sl_graph *g = sl_graph_create(&arena);
    graph_add_waypoint(&arena, g, (sl_waypoint){.id = 0, .x = 0.0, .y = 0.0});
    graph_add_waypoint(&arena, g, (sl_waypoint){.id = 1, .x = 1.0, .y = 0.0});
    graph_add_waypoint(&arena, g, (sl_waypoint){.id = 2, .x = 2.0, .y = 1.0});
    graph_add_verse(&arena, g, (sl_verse){.from = 0, .to = 1, .weight = 1.0});
    graph_add_verse(&arena, g, (sl_verse){.from = 1, .to = 2, .weight = 1.5});

    // Navigate (pathfind)
    sl_path path = sl_sing(g, /*from=*/0, /*to=*/2);
    printf("Path length: %zu, cost: %.2f\n", path.len, path.cost);

    // Navigability score (0-1)
    double nav = navigability_score(g);
    printf("Navigability: %.3f\n", nav);

    // Find hubs
    size_t hub_count;
    int *hubs = sl_find_hubs(g, &hub_count);

    // Corroboree clustering
    size_t cluster_count;
    int *clusters = corroboree_clusters(&arena, g, &cluster_count);

    // Modularity
    double mod = modularity_score(g);
    printf("Modularity: %.3f\n", mod);

    sl_arena_reset(&arena);
    return 0;
}
```

## API Reference

### Arena
- `void sl_arena_init(sl_arena *a, void *buf, size_t cap)`
- `void *sl_arena_alloc(sl_arena *a, size_t size)`
- `void sl_arena_reset(sl_arena *a)`

### Graph
- `sl_graph *sl_graph_create(sl_arena *a)`
- `void graph_add_waypoint(sl_arena *a, sl_graph *g, sl_waypoint w)`
- `void graph_add_verse(sl_arena *a, sl_graph *g, sl_verse v)`

### Navigation
- `sl_path sl_sing(const sl_graph *g, int from, int to)` — Pathfind (dreamtime fallback)
- `double navigability_score(const sl_graph *g)` — 0–1 navigability

### Corroboree
- `int *corroboree_clusters(sl_arena *a, const sl_graph *g, size_t *out_count)`
- `double modularity_score(const sl_graph *g)`

### Hubs & Evolution
- `int *sl_find_hubs(const sl_graph *g, size_t *count)`
- `double tradition_fitness(const sl_graph *g)`

### Unified API (`songline_api.h`)
- `SonglineCtx *songline_init(void *buf, size_t cap)`
- High-level wrapper combining all operations

## How It Works

Uses arena allocation — all memory comes from a pre-allocated buffer. Graphs are adjacency lists. Pathfinding is Dijkstra with a popularity bonus for highly-connected nodes. Dreamtime fallback uses coordinate-space greedy nearest-neighbor. Modularity uses the standard Q = (1/2m) Σ[A_ij - k_i·k_j/2m]δ(c_i,c_j).

## License

MIT
