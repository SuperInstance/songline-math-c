#include "tradition.h"
#include <math.h>

/* Simple LCG PRNG */
static uint32_t lcg_next(uint32_t *state)
{
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static double lcg_double(uint32_t *state)
{
    return (double)(lcg_next(state) >> 1) / (double)0x7FFFFFFFu;
}

/* ---------- Mutate ---------- */

sl_tradition tradition_mutate(sl_arena *a, const sl_tradition *t,
                               double add_probability, uint32_t seed)
{
    sl_tradition out;
    out.song = song_create(a, t->song.waypoint_count * 2 + 4);
    out.fitness = 0.0;

    uint32_t rng = seed;

    /* Copy existing waypoints, possibly skipping some */
    for (size_t i = 0; i < t->song.waypoint_count; i++) {
        double r = lcg_double(&rng);
        if (r < 0.9) { /* 90% chance to keep */
            song_add_waypoint(a, &out.song, t->song.waypoints[i]);
        }
    }

    /* Possibly add new waypoints */
    for (size_t i = 0; i < t->song.waypoint_count; i++) {
        double r = lcg_double(&rng);
        if (r < add_probability) {
            sl_waypoint w;
            w.id = 1000u + (lcg_next(&rng) % 9000u);
            w.knowledge_x = lcg_double(&rng) * 100.0;
            w.knowledge_y = lcg_double(&rng) * 100.0;
            w.label = NULL;
            song_add_waypoint(a, &out.song, w);
        }
    }

    return out;
}

/* ---------- Recombine ---------- */

sl_tradition tradition_recombine(sl_arena *a, const sl_tradition *t1,
                                  const sl_tradition *t2, uint32_t seed)
{
    sl_tradition out;
    size_t cap = t1->song.waypoint_count + t2->song.waypoint_count;
    out.song = song_create(a, cap > 0 ? cap : 1);
    out.fitness = 0.0;

    (void)seed;

    /* Take first half of t1 */
    size_t half1 = t1->song.waypoint_count / 2;
    for (size_t i = 0; i < half1; i++)
        song_add_waypoint(a, &out.song, t1->song.waypoints[i]);

    /* Take second half of t2 */
    size_t half2 = t2->song.waypoint_count / 2;
    for (size_t i = half2; i < t2->song.waypoint_count; i++)
        song_add_waypoint(a, &out.song, t2->song.waypoints[i]);

    return out;
}

/* ---------- Decay ---------- */

sl_song tradition_decay(sl_arena *a, const sl_song *s, double time)
{
    sl_song d = song_create(a, s->waypoint_count ? s->waypoint_count : 1);
    for (size_t i = 0; i < s->waypoint_count; i++) {
        sl_waypoint w = s->waypoints[i];
        /* Decay position toward origin — simulates forgetting */
        double factor = exp(-0.1 * time);
        w.knowledge_x *= factor;
        w.knowledge_y *= factor;
        song_add_waypoint(a, &d, w);
    }
    return d;
}

/* ---------- Fitness ---------- */

double tradition_fitness(const sl_graph *g)
{
    return navigability_score(g);
}
