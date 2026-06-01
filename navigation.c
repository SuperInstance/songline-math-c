#include "navigation.h"
#include <stdio.h>
#include <float.h>
#include <math.h>

/* ---------- Graph helpers ---------- */

sl_graph graph_create(sl_arena *a, size_t wp_cap, size_t v_cap)
{
    sl_graph g;
    g.waypoint_count = 0;
    g.verse_count    = 0;
    g.waypoints = (sl_waypoint *)sl_arena_alloc(a, wp_cap * sizeof(sl_waypoint));
    g.verses    = (sl_verse *)sl_arena_alloc(a, v_cap * sizeof(sl_verse));
    return g;
}

void graph_add_waypoint(sl_arena *a, sl_graph *g, sl_waypoint w)
{
    (void)a;
    g->waypoints[g->waypoint_count++] = w;
}

void graph_add_verse(sl_arena *a, sl_graph *g, sl_verse v)
{
    (void)a;
    g->verses[g->verse_count++] = v;
}

/* ---------- Internal: find waypoint index by id ---------- */

static int find_wp(const sl_graph *g, sl_id id)
{
    for (size_t i = 0; i < g->waypoint_count; i++)
        if (g->waypoints[i].id == id) return (int)i;
    return -1;
}

/* ---------- Dijkstra preferring HIGH weight ---------- */

sl_path songline_navigate(sl_arena *a, const sl_graph *g, sl_id start, sl_id end)
{
    size_t n = g->waypoint_count;
    sl_path empty = {NULL, 0, 0.0};

    int si = find_wp(g, start);
    int ei = find_wp(g, end);
    if (si < 0 || ei < 0) return empty;

    double *dist = (double *)sl_arena_alloc(a, n * sizeof(double));
    int    *prev = (int *)sl_arena_alloc(a, n * sizeof(int));
    bool   *vis  = (bool *)sl_arena_alloc(a, n * sizeof(bool));
    if (!dist || !prev || !vis) return empty;

    for (size_t i = 0; i < n; i++) {
        dist[i] = -INFINITY;
        prev[i] = -1;
        vis[i]  = false;
    }
    dist[si] = 0.0;

    for (size_t iter = 0; iter < n; iter++) {
        int u = -1;
        double best = -INFINITY;
        for (size_t j = 0; j < n; j++) {
            if (!vis[j] && dist[j] > best) { best = dist[j]; u = (int)j; }
        }
        if (u < 0) break;
        vis[u] = true;
        if (u == ei) break;

        for (size_t e = 0; e < g->verse_count; e++) {
            int fi = find_wp(g, g->verses[e].from);
            int ti = find_wp(g, g->verses[e].to);
            /* Forward: fi == u */
            if (fi == u && ti >= 0 && !vis[ti]) {
                double nd = dist[u] + g->verses[e].weight;
                if (nd > dist[ti]) { dist[ti] = nd; prev[ti] = u; }
            }
            /* Reverse: ti == u */
            if (ti == u && fi >= 0 && !vis[fi]) {
                double nd = dist[u] + g->verses[e].weight;
                if (nd > dist[fi]) { dist[fi] = nd; prev[fi] = u; }
            }
        }
    }

    if (prev[ei] < 0 && si != ei) return empty;

    /* reconstruct path */
    size_t count = 0;
    int cur = ei;
    while (cur >= 0) { count++; cur = prev[cur]; }

    sl_path p;
    p.ids   = (sl_id *)sl_arena_alloc(a, count * sizeof(sl_id));
    p.count = count;
    cur = ei;
    for (size_t i = count; i > 0; i--) {
        p.ids[i-1] = g->waypoints[cur].id;
        cur = prev[cur];
    }
    p.total_weight = dist[ei];
    return p;
}

/* ---------- Dreamtime fallback ---------- */

sl_path dreamtime_fallback(sl_arena *a, const sl_graph *g, sl_id start, sl_id end)
{
    /* Try normal navigation first */
    sl_path p = songline_navigate(a, g, start, end);
    if (p.count > 0) return p;

    /* BFS for disconnected graphs */
    size_t n = g->waypoint_count;
    int si = find_wp(g, start);
    int ei = find_wp(g, end);
    if (si < 0 || ei < 0) return p;

    int *prev = (int *)sl_arena_alloc(a, n * sizeof(int));
    bool *vis = (bool *)sl_arena_alloc(a, n * sizeof(bool));
    if (!prev || !vis) return p;

    for (size_t i = 0; i < n; i++) { prev[i] = -1; vis[i] = false; }

    int *queue = (int *)sl_arena_alloc(a, n * sizeof(int));
    size_t qh = 0, qt = 0;
    queue[qt++] = si;
    vis[si] = true;

    while (qh < qt) {
        int u = queue[qh++];
        if (u == ei) break;
        for (size_t e = 0; e < g->verse_count; e++) {
            int fi = find_wp(g, g->verses[e].from);
            int ti = find_wp(g, g->verses[e].to);
            if (fi == u && ti >= 0 && !vis[ti]) { vis[ti]=true; prev[ti]=u; queue[qt++]=(int)ti; }
            if (ti == u && fi >= 0 && !vis[fi]) { vis[fi]=true; prev[fi]=u; queue[qt++]=(int)fi; }
        }
    }

    if (prev[ei] < 0 && si != ei) return p;

    size_t count = 0;
    int cur = ei;
    while (cur >= 0) { count++; cur = prev[cur]; }

    sl_path fp;
    fp.ids = (sl_id *)sl_arena_alloc(a, count * sizeof(sl_id));
    fp.count = count;
    fp.total_weight = 0.0;
    cur = ei;
    for (size_t i = count; i > 0; i--) {
        fp.ids[i-1] = g->waypoints[cur].id;
        if (prev[cur] >= 0) {
            for (size_t e = 0; e < g->verse_count; e++) {
                if ((g->verses[e].from == g->waypoints[prev[cur]].id && g->verses[e].to == g->waypoints[cur].id) ||
                    (g->verses[e].to == g->waypoints[prev[cur]].id && g->verses[e].from == g->waypoints[cur].id)) {
                    fp.total_weight += g->verses[e].weight;
                    break;
                }
            }
        }
        cur = prev[cur];
    }
    return fp;
}

/* ---------- Navigability score ---------- */

double navigability_score(const sl_graph *g)
{
    if (g->waypoint_count <= 1) return 1.0;
    size_t n = g->waypoint_count;
    size_t max_verses = n * (n - 1);
    if (max_verses == 0) return 1.0;
    double density = (double)g->verse_count / (double)max_verses;
    return density > 1.0 ? 1.0 : density;
}
