# songline-math-c

A C99 library implementing Aboriginal songline navigation as edge/embedded knowledge graphs.

Songlines are navigable knowledge graphs — paths through information encoded as songs. This library implements them for edge/embedded targets (Jetson, RISC-V, ARM).

## Modules

| Header | Description |
|--------|-------------|
| `songline.h` | Core types: Waypoint, Verse, Song. Arena allocator. |
| `navigation.h` | Songline pathfinding, dreamtime fallback, navigability scoring. |
| `corroboree.h` | Convergence hub detection, union-find clustering, modularity scoring. |
| `tradition.h` | Tradition evolution: mutation, recombination, decay. Fitness = navigability. |
| `songline_api.h` | Unified API: graph building + full analysis in one call. |

## Requirements

- C99 compiler (gcc, clang, etc.)
- No external dependencies
- Uses only: `math.h`, `stdlib.h`, `string.h`, `stdbool.h`, `stdint.h`, `stdio.h`

## Building

```bash
make          # builds libsongline.a
make test     # builds and runs 46 tests
make clean    # remove build artifacts
```

## Architecture

- **Arena allocator** — all memory through a single bump allocator; no malloc at runtime
- **Zero dependencies** — pure C99, suitable for embedded (Jetson, RISC-V, ARM)
- **Bidirectional traversal** — verses are treated as undirected edges
- **High-weight preference** — navigation prefers well-known (high-weight) paths

## Quick Start

```c
#include "songline_api.h"

uint8_t buf[1024 * 1024];
sl_arena a;
sl_arena_init(&a, buf, sizeof(buf));

sl_song s = song_create(&a, 64);
song_add_waypoint(&a, &s, (sl_waypoint){1, 0.0, 0.0, "origin"});
song_add_waypoint(&a, &s, (sl_waypoint){2, 1.0, 0.0, "waterhole"});
song_add_waypoint(&a, &s, (sl_waypoint){3, 2.0, 0.0, "camp"});

sl_graph g = songline_build_graph(&a, &s);
sl_analysis an = songline_analyze(&a, &g, 1, 3);

printf("Path length: %zu\n", an.path.count);
printf("Navigability: %.2f\n", an.nav_score);
```

## License

MIT
