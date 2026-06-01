#include "songline.h"
#include <string.h>

/* ---------- Arena ---------- */

void sl_arena_init(sl_arena *a, void *buf, size_t cap)
{
    a->base     = (uint8_t *)buf;
    a->capacity = cap;
    a->offset   = 0;
}

void *sl_arena_alloc(sl_arena *a, size_t size)
{
    /* align to 8 bytes */
    size_t aligned = (a->offset + 7u) & ~(size_t)7u;
    if (aligned + size > a->capacity) return NULL;
    void *ptr = a->base + aligned;
    a->offset = aligned + size;
    memset(ptr, 0, size);
    return ptr;
}

void sl_arena_reset(sl_arena *a)
{
    a->offset = 0;
}

/* ---------- Song ---------- */

sl_song song_create(sl_arena *a, size_t cap)
{
    sl_song s;
    s.waypoint_count = 0;
    s.waypoint_cap   = cap;
    s.waypoints      = (sl_waypoint *)sl_arena_alloc(a, cap * sizeof(sl_waypoint));
    return s;
}

void song_add_waypoint(sl_arena *a, sl_song *s, sl_waypoint w)
{
    if (s->waypoint_count >= s->waypoint_cap) {
        size_t new_cap = s->waypoint_cap * 2;
        sl_waypoint *nw = (sl_waypoint *)sl_arena_alloc(a, new_cap * sizeof(sl_waypoint));
        if (!nw) return;
        memcpy(nw, s->waypoints, s->waypoint_count * sizeof(sl_waypoint));
        s->waypoints = nw;
        s->waypoint_cap = new_cap;
    }
    s->waypoints[s->waypoint_count++] = w;
}

sl_song song_extract_sub(sl_arena *a, const sl_song *s, size_t from, size_t len)
{
    sl_song sub = song_create(a, len ? len : 1);
    for (size_t i = 0; i < len && (from + i) < s->waypoint_count; i++)
        sub.waypoints[sub.waypoint_count++] = s->waypoints[from + i];
    return sub;
}

sl_song song_reverse(sl_arena *a, const sl_song *s)
{
    sl_song r = song_create(a, s->waypoint_count ? s->waypoint_count : 1);
    for (size_t i = 0; i < s->waypoint_count; i++)
        r.waypoints[r.waypoint_count++] = s->waypoints[s->waypoint_count - 1 - i];
    return r;
}
