#ifndef SONGLINE_H
#define SONGLINE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ---------- Arena allocator ---------- */

typedef struct {
    uint8_t *base;
    size_t    capacity;
    size_t    offset;
} sl_arena;

void  sl_arena_init(sl_arena *a, void *buf, size_t cap);
void *sl_arena_alloc(sl_arena *a, size_t size);
void  sl_arena_reset(sl_arena *a);

/* ---------- Core types ---------- */

typedef uint32_t sl_id;

typedef struct {
    sl_id   id;
    double  knowledge_x;   /* position in knowledge-space */
    double  knowledge_y;
    const char *label;      /* optional, may be NULL */
} sl_waypoint;

typedef struct {
    sl_id   from;
    sl_id   to;
    double  weight;         /* traversal weight / familiarity */
} sl_verse;                 /* an edge in the songline graph */

typedef struct {
    sl_waypoint *waypoints;
    size_t       waypoint_count;
    size_t       waypoint_cap;
} sl_song;

/* ---------- Song functions ---------- */

sl_song  song_create(sl_arena *a, size_t cap);
void     song_add_waypoint(sl_arena *a, sl_song *s, sl_waypoint w);
sl_song  song_extract_sub(sl_arena *a, const sl_song *s, size_t from, size_t len);
sl_song  song_reverse(sl_arena *a, const sl_song *s);

#endif /* SONGLINE_H */
