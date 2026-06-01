#include "songline_api.h"

sl_graph songline_build_graph(sl_arena *a, const sl_song *s)
{
    sl_graph g = graph_create(a, s->waypoint_count + 1, s->waypoint_count + 1);

    /* Add all waypoints */
    for (size_t i = 0; i < s->waypoint_count; i++)
        graph_add_waypoint(a, &g, s->waypoints[i]);

    /* Add sequential verses (each adjacent pair) */
    for (size_t i = 1; i < s->waypoint_count; i++) {
        sl_verse v;
        v.from   = s->waypoints[i-1].id;
        v.to     = s->waypoints[i].id;
        v.weight = 1.0;
        graph_add_verse(a, &g, v);
    }

    return g;
}

sl_analysis songline_analyze(sl_arena *a, const sl_graph *g,
                              sl_id start, sl_id end)
{
    sl_analysis an;
    an.path      = songline_navigate(a, g, start, end);
    an.hubs      = corroboree_find(a, g);
    an.nav_score = navigability_score(g);
    an.mod_score = modularity_score(g);
    return an;
}
