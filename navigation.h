#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "songline.h"

/* ---------- Songline graph ---------- */

typedef struct {
    sl_waypoint *waypoints;
    size_t       waypoint_count;
    sl_verse    *verses;
    size_t       verse_count;
} sl_graph;

sl_graph graph_create(sl_arena *a, size_t wp_cap, size_t v_cap);
void     graph_add_waypoint(sl_arena *a, sl_graph *g, sl_waypoint w);
void     graph_add_verse(sl_arena *a, sl_graph *g, sl_verse v);

/* ---------- Navigation ---------- */

typedef struct {
    sl_id  *ids;
    size_t  count;
    double  total_weight;
} sl_path;

/* Pathfinding preferring high-weight (well-known) waypoints.
   Returns path with count==0 if no path found. */
sl_path  songline_navigate(sl_arena *a, const sl_graph *g, sl_id start, sl_id end);

/* Fallback for disconnected graphs — finds the best bridge path. */
sl_path  dreamtime_fallback(sl_arena *a, const sl_graph *g, sl_id start, sl_id end);

/* Score 0..1 — how well-connected the graph is. */
double   navigability_score(const sl_graph *g);

#endif /* NAVIGATION_H */
