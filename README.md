<p align="center">
  <img src="assets/logo.png" alt="libislet logo" width="240" />
</p>

# libislet

[![C99](https://img.shields.io/badge/C-C99-555?logo=c)](#)
[![BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![Spatial (Morton)](https://img.shields.io/badge/spatial-Morton-16A34A)](#)
[![Recall Kernel Space Axis](https://img.shields.io/badge/axis-space-blueviolet)](#)

> Multi-dimensional spatial database engine using Morton code (Z-order) indexing for lightning-fast 3D/4D coordinate and bounding-box queries in pure C. Canonical header is `<ttypt/pointcfg.h>`.

---

## 1. Quickstart

Index 3D/4D entity positions and query a bounding box in under 20 lines:

```c
#include <stdio.h>
#include <stdint.h>
#include <ttypt/pointcfg.h>

int main(void) {
    islet_init();
    uint32_t world = islet_open(NULL, NULL, 0xFFFF);

    // Index entity positions in 3D space: (x, y, z) -> entity_id
    int16_t pos[3] = {120, 45, -300};
    uint32_t entity_id = 99;
    Point3_2.put(world, pos, entity_id);

    // Query 50x50x50 bounding box around the entity
    int16_t box_min[3] = {100, 20, -320};
    uint16_t box_len[3] = {50, 50, 50};
    uint32_t iter = Point3_2.iter(world, box_min, box_len);

    int16_t hit_pos[3];
    uint32_t hit_id;
    while (Point3_2.next(hit_pos, &hit_id, iter)) {
        printf("Found entity %u at (%d, %d, %d)\n", hit_id, hit_pos[0], hit_pos[1], hit_pos[2]);
    }

    return 0;
}
```

Compile and link:
```sh
cc voxel_query.c $(pkg-config --cflags --libs libislet) -o voxel_query
```

---

## 2. Core API Reference

The recommended API surface uses configuration descriptors (`Point<D>_<B>`, e.g. `Point3_2` for 3D and `Point4_2` for 4D `int16_t` coordinates):

| Operation | Signature | Description |
| :--- | :--- | :--- |
| `Point3_2.put` | `Point3_2.put(db, pos[3], ref)` | Store a 32-bit reference at 3D coordinates |
| `Point3_2.get` | `Point3_2.get(db, pos[3])` | Retrieve reference stored at 3D coordinates |
| `Point3_2.iter` | `Point3_2.iter(db, min[3], len[3])` | Open a cursor across a rectangular 3D bounding box |
| `Point3_2.next` | `Point3_2.next(pos_out, ref_out, iter)` | Advance bounding-box cursor (1 valid, 0 done) |
| `Point3_2.fill_bbox`| `Point3_2.fill_bbox(db, min, len, set)` | Stream all 3D box refs directly into a sealed recall candidate set |
| `Point4_2.put` | `Point4_2.put(db, pos[4], ref)` | Store a 32-bit reference at 4D coordinates (space + scale/time) |
| `Point4_2.fill_bbox`| `Point4_2.fill_bbox(db, min, len, set)` | Stream all 4D hypercube refs into a sealed recall set |

👉 *For AVX2 SIMD Morton codecs, multi-value cells, and benchmarks, see [docs/PERF.md](docs/PERF.md).*

---

## 3. Morton Z-Order Spatial Indexing

`libislet` flattens multi-dimensional space into linear integers using **Morton Z-order space-filling curves**:

```
  Y ▲
  3 │  10   11 │  14   15
  2 │   8    9 │  12   13
    ├───┼────┼─┼───┼────┤
  1 │   2    3 │   6    7
  0 │   0    1 │   4    5
    └──────────┴────────► X
        0    1    2    3
```

- **Bits Interleaved**: Coordinates $(X, Y, Z)$ are bit-interleaved into a single 64-bit integer, preserving spatial proximity in linear memory.
- **Logarithmic Seeks**: 3D bounding box queries turn into fast binary searches along the curve, skipping aligned empty space in single jumps.
- **Sub-Microsecond Speed**: Point insertions and range queries execute at CPU cache speeds with zero heap allocations.

---

## 4. `corm` CLI Multi-Axis Integration

`libislet` attaches directly to the `corm` CLI tool as the `@islet` spatial axis:

```bash
# Ingest 3D coordinates into database:
corm -p "120,45,-300:player_99" world.db@islet:a:s

# Spatial bounding box query via corm CLI:
corm -X "islet" --islet="bbox:100,20,-320,50,50,50" world.db@islet:a:s
```

`libislet` functions as the canonical **Space Axis** for spatial queries and recall composition:
- **Core Storage Engine**: Backed by [libcorm](https://github.com/tty-pt/libcorm) for sorted Morton-code map indexing.
- **Cross-Axis Recall Kernel**: Composes spatial bounding boxes (`Point3_2.fill_bbox`, `Point4_2.fill_bbox`) with sibling axes: [libjoint](https://github.com/tty-pt/libjoint) (Time), [liblemma](https://github.com/tty-pt/liblemma) (Text), and [libsepal](https://github.com/tty-pt/libsepal) (Meaning).

---

## 5. Installation & Build

### Prebuilt Packages
Prebuilt packages are distributed from tty.pt for Linux (APT / Alpine / Arch / Fedora-RHEL), macOS (Homebrew), Windows (winget / MSYS2), and OpenBSD. Follow the [installation instructions](https://github.com/tty-pt/ci/blob/main/docs/install.md) and use **libislet** as the package name.

### Build from Source
```sh
make                  # builds lib/libislet.so
make test             # run unit, integration, and stress tests
sudo make install     # install headers and library to system
```

## License

BSD 2-Clause License. Copyright (c) 2025, tty-pt. See `LICENSE`.
