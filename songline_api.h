#ifndef SONGLINE_API_H
#define SONGLINE_API_H

#include "songline.h"
#include "navigation.h"
#include "corroboree.h"
#include "tradition.h"

/* Unified API for the songline-math-c library.
   All functions take an arena allocator for memory management. */

/* Create a full graph from a song: waypoints + sequential verses. */
sl_graph songline_build_graph(sl_arena *a, const sl_song *s);

/* Full analysis: navigate, find hubs, compute scores. */
typedef struct {
    sl_path        path;
    sl_corroboree  hubs;
    double         nav_score;
    double         mod_score;
} sl_analysis;

sl_analysis songline_analyze(sl_arena *a, const sl_graph *g,
                              sl_id start, sl_id end);

#endif /* SONGLINE_API_H */
