#ifndef TRADITION_H
#define TRADITION_H

#include "songline.h"
#include "navigation.h"

typedef struct {
    sl_song song;
    double  fitness;  /* navigability score */
} sl_tradition;

/* Random mutation: add random waypoints with given probability.
   Uses a simple LCG PRNG seeded by `seed`. */
sl_tradition tradition_mutate(sl_arena *a, const sl_tradition *t,
                               double add_probability, uint32_t seed);

/* Recombine two traditions into a child. */
sl_tradition tradition_recombine(sl_arena *a, const sl_tradition *t1,
                                  const sl_tradition *t2, uint32_t seed);

/* Decay: waypoints lose weight/familiarity over time. Returns decayed song. */
sl_song tradition_decay(sl_arena *a, const sl_song *s, double time);

/* Compute fitness as navigability of a graph built from the song. */
double tradition_fitness(const sl_graph *g);

#endif /* TRADITION_H */
