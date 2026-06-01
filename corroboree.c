#include "corroboree.h"
#include <stdlib.h>

/* ---------- Union-Find ---------- */

static void uf_init(int *parent, int *rank, size_t n)
{
    for (size_t i = 0; i < n; i++) { parent[i] = (int)i; rank[i] = 0; }
}

static int uf_find(int *parent, int x)
{
    while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
    return x;
}

static void uf_union(int *parent, int *rank, int a, int b)
{
    a = uf_find(parent, a);
    b = uf_find(parent, b);
    if (a == b) return;
    if (rank[a] < rank[b]) { int t = a; a = b; b = t; }
    parent[b] = a;
    if (rank[a] == rank[b]) rank[a]++;
}

/* ---------- Convergence hubs ---------- */

sl_corroboree corroboree_find(sl_arena *a, const sl_graph *g)
{
    sl_corroboree c;
    c.hub_count = g->waypoint_count;
    c.hubs = (sl_hub *)sl_arena_alloc(a, g->waypoint_count * sizeof(sl_hub));

    for (size_t i = 0; i < g->waypoint_count; i++) {
        double score = 0.0;
        for (size_t e = 0; e < g->verse_count; e++) {
            if (g->verses[e].from == g->waypoints[i].id ||
                g->verses[e].to   == g->waypoints[i].id) {
                score += g->verses[e].weight;
            }
        }
        c.hubs[i].waypoint_id       = g->waypoints[i].id;
        c.hubs[i].convergence_score = score;
    }
    return c;
}

/* ---------- Union-Find clustering ---------- */

int *corroboree_clusters(sl_arena *a, const sl_graph *g, size_t *out_count)
{
    size_t n = g->waypoint_count;
    *out_count = n;
    int *parent = (int *)sl_arena_alloc(a, n * sizeof(int));
    int *rank   = (int *)sl_arena_alloc(a, n * sizeof(int));
    if (!parent || !rank) { *out_count = 0; return NULL; }

    uf_init(parent, rank, n);

    for (size_t e = 0; e < g->verse_count; e++) {
        int fi = -1, ti = -1;
        for (size_t i = 0; i < n; i++) {
            if (g->waypoints[i].id == g->verses[e].from) fi = (int)i;
            if (g->waypoints[i].id == g->verses[e].to)   ti = (int)i;
        }
        if (fi >= 0 && ti >= 0) uf_union(parent, rank, fi, ti);
    }

    /* Canonicalize: each entry → its root */
    for (size_t i = 0; i < n; i++) parent[i] = uf_find(parent, (int)i);

    return parent;
}

/* ---------- Modularity score ---------- */

double modularity_score(const sl_graph *g)
{
    size_t n = g->waypoint_count;
    if (n <= 1) return 1.0;

    /* Total edge weight */
    double m = 0.0;
    for (size_t e = 0; e < g->verse_count; e++) m += g->verses[e].weight;
    if (m == 0.0) return 0.0;
    m /= 2.0; /* undirected double-count */

    /* Degree of each waypoint */
    double *deg = (double *)malloc(n * sizeof(double));
    if (!deg) return 0.0;
    for (size_t i = 0; i < n; i++) deg[i] = 0.0;
    for (size_t e = 0; e < g->verse_count; e++) {
        for (size_t i = 0; i < n; i++) {
            if (g->waypoints[i].id == g->verses[e].from) deg[i] += g->verses[e].weight;
            if (g->waypoints[i].id == g->verses[e].to)   deg[i] += g->verses[e].weight;
        }
    }

    /* Simple community detection: use connected components as communities */
    int *parent = (int *)malloc(n * sizeof(int));
    int *rank   = (int *)malloc(n * sizeof(int));
    if (!parent || !rank) { free(deg); free(parent); free(rank); return 0.0; }
    uf_init(parent, rank, n);
    for (size_t e = 0; e < g->verse_count; e++) {
        int fi = -1, ti = -1;
        for (size_t i = 0; i < n; i++) {
            if (g->waypoints[i].id == g->verses[e].from) fi = (int)i;
            if (g->waypoints[i].id == g->verses[e].to)   ti = (int)i;
        }
        if (fi >= 0 && ti >= 0) uf_union(parent, rank, fi, ti);
    }
    for (size_t i = 0; i < n; i++) parent[i] = uf_find(parent, (int)i);

    /* Q = sum_{ij} [ A_ij/(2m) - k_i*k_j/(2m)^2 ] * delta(c_i, c_j) */
    double Q = 0.0;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if (parent[i] != parent[j]) continue;
            double A_ij = 0.0;
            for (size_t e = 0; e < g->verse_count; e++) {
                if ((g->verses[e].from == g->waypoints[i].id && g->verses[e].to == g->waypoints[j].id) ||
                    (g->verses[e].from == g->waypoints[j].id && g->verses[e].to == g->waypoints[i].id)) {
                    A_ij += g->verses[e].weight;
                }
            }
            Q += A_ij / (2.0 * m) - (deg[i] * deg[j]) / ((2.0 * m) * (2.0 * m));
        }
    }

    free(deg); free(parent); free(rank);
    return Q < 0.0 ? 0.0 : (Q > 1.0 ? 1.0 : Q);
}
