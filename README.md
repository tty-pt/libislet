<p align="center">
  <img src="assets/logo.png" alt="libislet logo" width="240" />
</p>

# libislet

[![C99](https://img.shields.io/badge/C-C99-555?logo=c)](#)
[![BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![Spatial (Morton)](https://img.shields.io/badge/spatial-Morton-16A34A)](#)
[![Recall Kernel Space Axis](https://img.shields.io/badge/axis-space-blueviolet)](#)

> Multi-dimensional spatial database engine using Morton code (Z-order) indexing for lightning-fast 3D coordinate and bounding-box queries in pure C.

---

## Recall Kernel (Space Axis)

`libislet` functions as the canonical **Space Axis** for spatial queries and recall composition:
- **Core Storage Engine**: Backed by [libcorm](https://github.com/tty-pt/libcorm) for sorted Morton-code map indexing.
- **Cross-Axis Recall Kernel**: Composes spatial bounding boxes (`Point3_2.fill_bbox`) with sibling axes: [libjoint](https://github.com/tty-pt/libit) (Time), [liblemma](https://github.com/tty-pt/liblemma) (Text), and [libsepal](https://github.com/tty-pt/libsepal) (Meaning).

---

## Quickstart

Index 50,000 player positions in a 3D voxel game world and query a 50×50×50 region in microseconds:

```c
#include <stdio.h>
#include <stdint.h>
#include <ttypt/pointcfg.h>

int main(void) {
    islet_init();
    uint32_t world = islet_open(NULL, NULL, 0xFFFF);

    // Index player positions in 3D space: (x, y, z) -> player_id
    int16_t pos[3] = {120, 45, -300};
    uint32_t player_id = 99;
    Point3_2.put(world, pos, player_id);

    // Query 50x50x50 bounding box around the player
    int16_t box_min[3] = {100, 20, -320};
    uint16_t box_len[3] = {50, 50, 50};
    uint32_t iter = Point3_2.iter(world, box_min, box_len);

    int16_t hit_pos[3];
    uint32_t hit_id;
    while (Point3_2.next(hit_pos, &hit_id, iter)) {
        printf("Found player %u at (%d, %d, %d)\n", hit_id, hit_pos[0], hit_pos[1], hit_pos[2]);
    }

    return 0;
}
```

Compile and link:
```sh
cc voxel_query.c $(pkg-config --cflags --libs islet)
```

---

## Core API (The Essential 20%)

The recommended API surface uses configuration objects (`Point<D>_<B>`, e.g. `Point3_2` for 3D `int16_t` coordinates):

| Operation | Signature | Description |
| :--- | :--- | :--- |
| `Point3_2.put` | `Point3_2.put(db, pos[3], ref)` | Store a 32-bit reference at 3D coordinates |
| `Point3_2.get` | `Point3_2.get(db, pos[3])` | Retrieve the reference stored at coordinates |
| `Point3_2.iter` | `Point3_2.iter(db, min[3], len[3])` | Open a cursor across a rectangular 3D bounding box |
| `Point3_2.next` | `Point3_2.next(pos_out, ref_out, iter)` | Advance bounding-box cursor (1 valid, 0 done) |
| `Point3_2.fill_bbox`| `Point3_2.fill_bbox(db, min, len, set)` | Stream all box refs directly into a sealed recall set |

👉 *For AVX2 SIMD Morton codecs, 4D lane configs, and benchmark numbers, see [docs/PERF.md](docs/PERF.md).*

---

## Design Philosophy: Locality Preserved

* **Z-Order Morton Indexing**: Maps N-dimensional coordinates into a linear 64-bit integer while preserving spatial locality, enabling O(log n) tree seeks.
* **Z-Interval Skip**: Instead of decoding every point along a curve, empty aligned bounding cubes are skipped in single jumps.
* **Dense 16-Bit Lanes**: Full 3D coordinate ranges (-32,768 to 32,767 per axis) pack completely into a 64-bit integer, fitting CPU registers and cache lines without pointer indirection.

---

## Installation & Build

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
