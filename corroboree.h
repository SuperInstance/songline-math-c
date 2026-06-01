#ifndef CORROBOREE_H
#define CORROBOREE_H

#include "navigation.h"

/* ---------- Convergence hub ---------- */

typedef struct {
    sl_id   waypoint_id;
    double  convergence_score;  /* how many songlines converge here */
} sl_hub;

typedef struct {
    sl_hub *hubs;
    size_t  hub_count;
} sl_corroboree;

/* Find convergence hubs in the graph. */
sl_corroboree corroboree_find(sl_arena *a, const sl_graph *g);

/* Union-find clustering by shared songlines.
   Returns cluster assignments (array of cluster ids, one per waypoint). */
int *corroboree_clusters(sl_arena *a, const sl_graph *g, size_t *out_count);

/* Modularity score 0..1 — how strongly the graph partitions into communities. */
double modularity_score(const sl_graph *g);

#endif /* CORROBOREE_H */
