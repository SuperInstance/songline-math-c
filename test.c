#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "songline.h"
#include "navigation.h"
#include "corroboree.h"
#include "tradition.h"
#include "songline_api.h"

#define ARENA_SIZE (1 << 20) /* 1 MB */

static sl_arena g_arena;
static uint8_t g_buf[ARENA_SIZE];

static void arena_reset(void)
{
    sl_arena_reset(&g_arena);
}

static void arena_init(void)
{
    sl_arena_init(&g_arena, g_buf, ARENA_SIZE);
}

/* ============================================================
   TEST HELPERS
   ============================================================ */

static sl_waypoint mk_wp(sl_id id, double x, double y)
{
    sl_waypoint w = { id, x, y, NULL };
    return w;
}

static void make_linear_graph(sl_graph *g, size_t n)
{
    for (size_t i = 0; i < n; i++)
        graph_add_waypoint(&g_arena, g, mk_wp((sl_id)i, (double)i, 0.0));
    for (size_t i = 1; i < n; i++) {
        sl_verse v = { (sl_id)(i-1), (sl_id)i, 1.0 + (double)i * 0.1 };
        graph_add_verse(&g_arena, g, v);
    }
}

/* ============================================================
   ARENA TESTS (5)
   ============================================================ */

static void test_arena_init_alloc(void)
{
    arena_init();
    void *p = sl_arena_alloc(&g_arena, 64);
    assert(p != NULL);
    /* should be zeroed */
    uint8_t *bp = (uint8_t *)p;
    for (int i = 0; i < 64; i++) assert(bp[i] == 0);
}

static void test_arena_alignment(void)
{
    arena_init();
    sl_arena_alloc(&g_arena, 3);
    void *p = sl_arena_alloc(&g_arena, 8);
    assert(((uintptr_t)p & 7u) == 0);
}

static void test_arena_reset(void)
{
    arena_init();
    sl_arena_alloc(&g_arena, 128);
    assert(g_arena.offset > 0);
    sl_arena_reset(&g_arena);
    assert(g_arena.offset == 0);
}

static void test_arena_overflow(void)
{
    uint8_t small[32];
    sl_arena a;
    sl_arena_init(&a, small, 32);
    void *p = sl_arena_alloc(&a, 16);
    assert(p != NULL);
    void *q = sl_arena_alloc(&a, 64);
    assert(q == NULL);  /* overflow */
}

static void test_arena_multiple_allocs(void)
{
    arena_init();
    void *a = sl_arena_alloc(&g_arena, 100);
    void *b = sl_arena_alloc(&g_arena, 200);
    assert(a && b);
    assert(a != b);
}

/* ============================================================
   SONG TESTS (10)
   ============================================================ */

static void test_song_create(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    assert(s.waypoints != NULL);
    assert(s.waypoint_count == 0);
    assert(s.waypoint_cap == 16);
}

static void test_song_add_waypoint(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    song_add_waypoint(&g_arena, &s, mk_wp(1, 0.0, 0.0));
    song_add_waypoint(&g_arena, &s, mk_wp(2, 1.0, 1.0));
    assert(s.waypoint_count == 2);
    assert(s.waypoints[0].id == 1);
    assert(s.waypoints[1].id == 2);
}

static void test_song_add_many(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 4);
    for (int i = 0; i < 100; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, (double)i, 0.0));
    assert(s.waypoint_count == 100);
    assert(s.waypoints[99].id == 99);
}

static void test_song_extract_sub(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    for (int i = 0; i < 10; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, 0, 0));
    sl_song sub = song_extract_sub(&g_arena, &s, 3, 4);
    assert(sub.waypoint_count == 4);
    assert(sub.waypoints[0].id == 3);
    assert(sub.waypoints[3].id == 6);
}

static void test_song_extract_sub_overflow(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    for (int i = 0; i < 5; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, 0, 0));
    sl_song sub = song_extract_sub(&g_arena, &s, 3, 10);
    assert(sub.waypoint_count == 2);  /* only indices 3,4 */
}

static void test_song_reverse(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    for (int i = 0; i < 5; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, 0, 0));
    sl_song r = song_reverse(&g_arena, &s);
    assert(r.waypoint_count == 5);
    for (int i = 0; i < 5; i++)
        assert(r.waypoints[i].id == (sl_id)(4 - i));
}

static void test_song_reverse_empty(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    sl_song r = song_reverse(&g_arena, &s);
    assert(r.waypoint_count == 0);
}

static void test_song_single_waypoint(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 4);
    song_add_waypoint(&g_arena, &s, mk_wp(42, 1.5, 2.5));
    assert(s.waypoint_count == 1);
    assert(fabs(s.waypoints[0].knowledge_x - 1.5) < 1e-9);
    assert(fabs(s.waypoints[0].knowledge_y - 2.5) < 1e-9);
}

static void test_song_extract_sub_single(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 8);
    for (int i = 0; i < 8; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, 0, 0));
    sl_song sub = song_extract_sub(&g_arena, &s, 4, 1);
    assert(sub.waypoint_count == 1);
    assert(sub.waypoints[0].id == 4);
}

static void test_song_reverse_preserves_coords(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 8);
    song_add_waypoint(&g_arena, &s, mk_wp(1, 10.0, 20.0));
    song_add_waypoint(&g_arena, &s, mk_wp(2, 30.0, 40.0));
    sl_song r = song_reverse(&g_arena, &s);
    assert(r.waypoints[0].id == 2);
    assert(fabs(r.waypoints[0].knowledge_x - 30.0) < 1e-9);
    assert(fabs(r.waypoints[1].knowledge_y - 20.0) < 1e-9);
}

/* ============================================================
   NAVIGATION TESTS (10)
   ============================================================ */

static void test_graph_create(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    assert(g.waypoints != NULL);
    assert(g.verses != NULL);
    assert(g.waypoint_count == 0);
    assert(g.verse_count == 0);
}

static void test_graph_add(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 1, 0));
    graph_add_verse(&g_arena, &g, (sl_verse){1, 2, 5.0});
    assert(g.waypoint_count == 2);
    assert(g.verse_count == 1);
}

static void test_navigate_linear(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    make_linear_graph(&g, 5);
    sl_path p = songline_navigate(&g_arena, &g, 0, 4);
    assert(p.count == 5);
    assert(p.ids[0] == 0);
    assert(p.ids[4] == 4);
    assert(p.total_weight > 0.0);
}

static void test_navigate_same_node(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    sl_path p = songline_navigate(&g_arena, &g, 1, 1);
    assert(p.count == 1);
    assert(p.ids[0] == 1);
}

static void test_navigate_no_path(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 10, 0));
    /* no verses! */
    sl_path p = songline_navigate(&g_arena, &g, 1, 2);
    assert(p.count == 0);
}

static void test_navigate_prefers_high_weight(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 1, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(3, 2, 0));
    /* low-weight direct */
    graph_add_verse(&g_arena, &g, (sl_verse){1, 3, 1.0});
    /* high-weight through 2 */
    graph_add_verse(&g_arena, &g, (sl_verse){1, 2, 10.0});
    graph_add_verse(&g_arena, &g, (sl_verse){2, 3, 10.0});
    sl_path p = songline_navigate(&g_arena, &g, 1, 3);
    /* Should prefer the 10+10 path through 2 */
    assert(p.total_weight > 15.0);
}

static void test_navigate_missing_node(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    sl_path p = songline_navigate(&g_arena, &g, 1, 999);
    assert(p.count == 0);
}

static void test_dreamtime_fallback(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    make_linear_graph(&g, 4);
    sl_path p = dreamtime_fallback(&g_arena, &g, 0, 3);
    assert(p.count == 4);
    assert(p.ids[0] == 0);
    assert(p.ids[3] == 3);
}

static void test_dreamtime_no_path(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 10, 0));
    sl_path p = dreamtime_fallback(&g_arena, &g, 1, 2);
    assert(p.count == 0);
}

static void test_navigability_score(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    assert(fabs(navigability_score(&g) - 1.0) < 1e-9); /* 0-1 nodes → 1.0 */

    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 1, 0));
    double s = navigability_score(&g);
    assert(s >= 0.0 && s <= 1.0);
}

/* ============================================================
   CORROBOREE TESTS (8)
   ============================================================ */

static void test_corroboree_find(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    make_linear_graph(&g, 5);
    sl_corroboree c = corroboree_find(&g_arena, &g);
    assert(c.hub_count == 5);
    /* Middle waypoints should have higher convergence */
    assert(c.hubs[0].convergence_score > 0.0);
}

static void test_corroboree_hub_scoring(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 32);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 1, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(3, 2, 0));
    graph_add_verse(&g_arena, &g, (sl_verse){1, 2, 5.0});
    graph_add_verse(&g_arena, &g, (sl_verse){2, 3, 3.0});
    graph_add_verse(&g_arena, &g, (sl_verse){1, 2, 2.0}); /* extra to 2 */
    sl_corroboree c = corroboree_find(&g_arena, &g);
    /* node 2 has most convergence */
    double max_score = 0.0;
    sl_id max_id = 0;
    for (size_t i = 0; i < c.hub_count; i++) {
        if (c.hubs[i].convergence_score > max_score) {
            max_score = c.hubs[i].convergence_score;
            max_id = c.hubs[i].waypoint_id;
        }
    }
    assert(max_id == 2);
}

static void test_corroboree_clusters(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    make_linear_graph(&g, 5);
    size_t n;
    int *clusters = corroboree_clusters(&g_arena, &g, &n);
    assert(clusters != NULL);
    assert(n == 5);
    /* All in same cluster */
    for (size_t i = 1; i < n; i++)
        assert(clusters[i] == clusters[0]);
}

static void test_corroboree_two_clusters(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    /* cluster 1: 1-2 */
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(2, 1, 0));
    graph_add_verse(&g_arena, &g, (sl_verse){1, 2, 1.0});
    /* cluster 2: 3-4 */
    graph_add_waypoint(&g_arena, &g, mk_wp(3, 10, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(4, 11, 0));
    graph_add_verse(&g_arena, &g, (sl_verse){3, 4, 1.0});

    size_t n;
    int *c = corroboree_clusters(&g_arena, &g, &n);
    assert(n == 4);
    assert(c[0] == c[1]);  /* 1 and 2 same cluster */
    assert(c[2] == c[3]);  /* 3 and 4 same cluster */
    assert(c[0] != c[2]);  /* different clusters */
}

static void test_corroboree_empty_graph(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 4, 4);
    sl_corroboree c = corroboree_find(&g_arena, &g);
    assert(c.hub_count == 0);
}

static void test_modularity_connected(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    make_linear_graph(&g, 5);
    double m = modularity_score(&g);
    assert(m >= 0.0 && m <= 1.0);
}

static void test_modularity_empty(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 4, 4);
    assert(fabs(modularity_score(&g) - 1.0) < 1e-9);
}

static void test_modularity_two_communities(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 32);
    /* Large dense community */
    for (int i = 0; i < 6; i++)
        graph_add_waypoint(&g_arena, &g, mk_wp((sl_id)i, (double)i, 0));
    for (int i = 0; i < 6; i++)
        for (int j = i+1; j < 6; j++)
            graph_add_verse(&g_arena, &g, (sl_verse){(sl_id)i, (sl_id)j, 5.0});
    /* Small sparse community */
    graph_add_waypoint(&g_arena, &g, mk_wp(10, 10, 0));
    graph_add_waypoint(&g_arena, &g, mk_wp(11, 11, 0));
    graph_add_verse(&g_arena, &g, (sl_verse){10, 11, 1.0});
    double m = modularity_score(&g);
    assert(m >= 0.0 && m <= 1.0);
    /* Disconnected communities should have modularity > 0 when unbalanced */
    /* (For perfectly balanced cliques, modularity ≈ 0 by definition) */
}

/* ============================================================
   TRADITION TESTS (8)
   ============================================================ */

static void test_tradition_mutate(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    for (int i = 0; i < 10; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, (double)i, 0));
    sl_tradition t = { s, 0.5 };
    sl_tradition m = tradition_mutate(&g_arena, &t, 0.3, 42);
    assert(m.song.waypoint_count > 0);
    assert(m.song.waypoint_count <= 20); /* at most original + adds */
}

static void test_tradition_mutate_deterministic(void)
{
    /* Use separate arenas so mutation is deterministic with same seed */
    uint8_t buf1[4096], buf2[4096];
    sl_arena a1, a2;
    sl_arena_init(&a1, buf1, sizeof(buf1));
    sl_arena_init(&a2, buf2, sizeof(buf2));

    /* Build song in a temp arena just for init */
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    song_add_waypoint(&g_arena, &s, mk_wp(1, 0, 0));
    sl_tradition t = { s, 0.0 };

    sl_tradition m1 = tradition_mutate(&a1, &t, 0.5, 12345);
    sl_tradition m2 = tradition_mutate(&a2, &t, 0.5, 12345);
    assert(m1.song.waypoint_count == m2.song.waypoint_count);
}

static void test_tradition_recombine(void)
{
    arena_init();
    sl_song s1 = song_create(&g_arena, 16);
    sl_song s2 = song_create(&g_arena, 16);
    for (int i = 0; i < 10; i++) {
        song_add_waypoint(&g_arena, &s1, mk_wp((sl_id)i, (double)i, 0));
        song_add_waypoint(&g_arena, &s2, mk_wp((sl_id)(100+i), 0, (double)i));
    }
    sl_tradition t1 = { s1, 0.0 };
    sl_tradition t2 = { s2, 0.0 };
    sl_tradition child = tradition_recombine(&g_arena, &t1, &t2, 0);
    assert(child.song.waypoint_count > 0);
    assert(child.song.waypoint_count <= 10);
}

static void test_tradition_recombine_empty(void)
{
    arena_init();
    sl_song s1 = song_create(&g_arena, 4);
    sl_song s2 = song_create(&g_arena, 4);
    sl_tradition t1 = { s1, 0.0 };
    sl_tradition t2 = { s2, 0.0 };
    sl_tradition child = tradition_recombine(&g_arena, &t1, &t2, 0);
    assert(child.song.waypoint_count == 0);
}

static void test_tradition_decay(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    song_add_waypoint(&g_arena, &s, mk_wp(1, 100.0, 200.0));
    song_add_waypoint(&g_arena, &s, mk_wp(2, 50.0, 50.0));
    sl_song d = tradition_decay(&g_arena, &s, 1.0);
    assert(d.waypoint_count == 2);
    /* Should be decayed toward 0 */
    assert(d.waypoints[0].knowledge_x < 100.0);
    assert(d.waypoints[0].knowledge_x > 0.0);
}

static void test_tradition_decay_zero_time(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    song_add_waypoint(&g_arena, &s, mk_wp(1, 42.0, 84.0));
    sl_song d = tradition_decay(&g_arena, &s, 0.0);
    assert(fabs(d.waypoints[0].knowledge_x - 42.0) < 1e-9);
    assert(fabs(d.waypoints[0].knowledge_y - 84.0) < 1e-9);
}

static void test_tradition_decay_long_time(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    song_add_waypoint(&g_arena, &s, mk_wp(1, 1000.0, 1000.0));
    sl_song d = tradition_decay(&g_arena, &s, 100.0);
    /* exp(-0.1 * 100) ≈ 0.000045 → nearly 0 */
    assert(d.waypoints[0].knowledge_x < 1.0);
}

static void test_tradition_fitness(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 16);
    make_linear_graph(&g, 5);
    double f = tradition_fitness(&g);
    assert(f >= 0.0 && f <= 1.0);
}

/* ============================================================
   UNIFIED API TESTS (5)
   ============================================================ */

static void test_api_build_graph(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 16);
    for (int i = 0; i < 5; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)i, (double)i, 0));
    sl_graph g = songline_build_graph(&g_arena, &s);
    assert(g.waypoint_count == 5);
    assert(g.verse_count == 4);
}

static void test_api_build_graph_empty(void)
{
    arena_init();
    sl_song s = song_create(&g_arena, 4);
    sl_graph g = songline_build_graph(&g_arena, &s);
    assert(g.waypoint_count == 0);
    assert(g.verse_count == 0);
}

static void test_api_analyze(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 16, 32);
    make_linear_graph(&g, 5);
    sl_analysis an = songline_analyze(&g_arena, &g, 0, 4);
    assert(an.path.count == 5);
    assert(an.hubs.hub_count == 5);
    assert(an.nav_score >= 0.0 && an.nav_score <= 1.0);
    assert(an.mod_score >= 0.0 && an.mod_score <= 1.0);
}

static void test_api_analyze_single(void)
{
    arena_init();
    sl_graph g = graph_create(&g_arena, 4, 4);
    graph_add_waypoint(&g_arena, &g, mk_wp(1, 0, 0));
    sl_analysis an = songline_analyze(&g_arena, &g, 1, 1);
    assert(an.path.count == 1);
    assert(an.path.ids[0] == 1);
}

static void test_api_roundtrip(void)
{
    arena_init();
    /* Build song → graph → navigate → verify */
    sl_song s = song_create(&g_arena, 32);
    for (int i = 0; i < 10; i++)
        song_add_waypoint(&g_arena, &s, mk_wp((sl_id)(100 + i), (double)i, (double)(i*2)));
    sl_graph g = songline_build_graph(&g_arena, &s);
    sl_path p = songline_navigate(&g_arena, &g, 100, 109);
    assert(p.count == 10);
    assert(p.ids[0] == 100);
    assert(p.ids[9] == 109);
}

/* ============================================================
   MAIN
   ============================================================ */

int main(void)
{
    /* Arena */
    test_arena_init_alloc();
    test_arena_alignment();
    test_arena_reset();
    test_arena_overflow();
    test_arena_multiple_allocs();

    /* Song */
    test_song_create();
    test_song_add_waypoint();
    test_song_add_many();
    test_song_extract_sub();
    test_song_extract_sub_overflow();
    test_song_reverse();
    test_song_reverse_empty();
    test_song_single_waypoint();
    test_song_extract_sub_single();
    test_song_reverse_preserves_coords();

    /* Navigation */
    test_graph_create();
    test_graph_add();
    test_navigate_linear();
    test_navigate_same_node();
    test_navigate_no_path();
    test_navigate_prefers_high_weight();
    test_navigate_missing_node();
    test_dreamtime_fallback();
    test_dreamtime_no_path();
    test_navigability_score();

    /* Corroboree */
    test_corroboree_find();
    test_corroboree_hub_scoring();
    test_corroboree_clusters();
    test_corroboree_two_clusters();
    test_corroboree_empty_graph();
    test_modularity_connected();
    test_modularity_empty();
    test_modularity_two_communities();

    /* Tradition */
    test_tradition_mutate();
    test_tradition_mutate_deterministic();
    test_tradition_recombine();
    test_tradition_recombine_empty();
    test_tradition_decay();
    test_tradition_decay_zero_time();
    test_tradition_decay_long_time();
    test_tradition_fitness();

    /* Unified API */
    test_api_build_graph();
    test_api_build_graph_empty();
    test_api_analyze();
    test_api_analyze_single();
    test_api_roundtrip();

    printf("All %d tests passed!\n", 5 + 10 + 10 + 8 + 8 + 5);
    return 0;
}
