## 1.1.0

- **Renamed `libgeo` → `libislet`**: the `geo_*` API surface is now `islet_*` (`include/ttypt/islet.h`).
- **BREAKING — per-dimension public API**: runtime-`dim` functions are gone — `morton_set(p, dim)` is now `morton_set_1..4(p)`, `morton_get` is `morton_get_1..4`, every `point_*` is `point_*_1..4`, and every database op is `islet_put_N`/`islet_set_N`/`islet_get_N`/`islet_del_N`/`islet_del_all_N`/`islet_cell_count_N`/`islet_get_multi_N`/`islet_iter_N`/`rec_axis_fill_bbox_N` (N = 1..4, no dim argument). Invalid dims are now unrepresentable. Morton code values unchanged (1D/2D/3D bit-identical to v0.5.0).
- **New 2D × 32-bit dense config** (`islet_*_2_32`, `morton_set_2_32`/`morton_get_2_32`, `point_*_2_32`): `int32_t` lanes (−2147483648..2147483647), dense stride-2 codec filling all 64 key bits; box walker, Z-interval skip, fill, iterate and multi-value chains all support 32-bit lanes with `int32_t` start and lengths, advanced with `islet_next32`.
- **Point config objects** (`include/ttypt/pointcfg.h`): exported const structs `Point<D>_<B>` (`Point1_2`…`Point4_2` on int16 lanes, `Point2_4` on int32 lanes), each with ~20 addressable member functions — codec, point arith, `put/get/replace/del/del_all/cell_count`, `get_multi`/`islet_cell_next`, `iter`, `fill_bbox`. The flat API stays exported as the tight-loop fast path.
- **`islet_ops[1..4]`**: runtime-dim dispatch table (`islet_ops[0]` is NULL) covering codec, point, scatter, iterate and fill operations.
- **Bulk decode**: `morton_get_bulk()` / `morton_get_bulk4()`, decode-side counterparts to `morton_set_bulk`/`morton_set_bulk4` (closes the encode-only asymmetry).
- **BMI2 codec fast path**: `ISLET_USE_PDEP` (default 1, opt out with `-DISLET_USE_PDEP=0`) — PDEP/PEXT spread/compact kernels when compiled with `-mbmi2`, measured 2–4× on isolated encode/decode/round-trip; portable scalar fallback retained.
- **Recall-kernel alignment**: refs unified to `rec_ref_t` (`uint32_t`), kernel-based axis registration and storage adapters (`rec_axis_store`/`rec_axis_unstore`/`rec_axis_readback`), axis parameters exposed as CLI parameters, plus new 2D/4D unit, property and stress test coverage.
- `libqmap` → `libcorm` rename.
- **Monomorphized box walker**: `islet_box_walk_1..4` stamped from one macro with literal dims (`islet_jump_over_gap` inlined so the literal folds its k-loop chain); cursors/collections carry a per-dim `point_copy`. Measured parity vs the generic walker on this VM (see `docs/PERF.md`); kept for the structural win at ~19 KB extra `.text`.
- **4D support** (dim=4): dense stride-4 Morton codec using the full 64-bit key space; box walker, Z-interval skip, fill, and SIMD bulk all generalized (skip-cube span now `2^(D·k)`); 1D/2D/3D codes unchanged.
- **Tunable audit**: new `ISLET_SMALLDIM_UNROLL` / `ISLET_4D_UNROLL` (default 0, measure-first) and `morton_set_bulk4()` batch encoder; `ISLET_PACKED_CURI` and `ISLET_3D_POINT_COPY` retired after measured-parity results; remaining tunables promoted to unconditional — only `ISLET_SIMD_MORTON` (batch API gate) stays tunable. New `docs/PERF.md` documents per-flag verdicts and the VM-drift benchmarking caveat, plus the single-dimension-per-database convention.

## [0.5.0] - 2026-09-10
- Kernel form (requires libcorm >= 0.8.0): maps open CM_SORTED|CM_MULTIVALUE
  - Multi-value cells: islet_put appends; new islet_set (replace), islet_get_multi /
    islet_cell_next (chain read in insertion order), islet_cell_count, islet_del_all
  - islet_get returns the first value; islet_del removes the first value
  - New rec_axis_fill_bbox(): stream a bounding box into a sealed rec_set_t
    (sorted + deduped), never materializing the box volume; ISLET_FILL_MAX_VOL
    (1M cells) cap with -1 errors; plain int return
- Raw iterator restructured: flat (point, value) list in morton-discovery
  order (matches the long-standing doc claim), grown on demand — no more
  volume-sized array, sparse queries over huge boxes stay cheap
- Z-interval skip: box walks ratchet a skip floor past the largest
  box-disjoint aligned cube at each false positive (provably sound; replaces
  the dead COMPUTE_BMLM path, whose constants were wrong); islet_last_scan_count
  diagnostic proves engagement
- Docs: capacity claims fixed (mask = initial table, map auto-grows, no
  termination); islet.pc version synced
- Tests: 10 MV unit + 11 fill unit + 7 range unit (brute-force oracle,
  mutation-validated) + parity integration + MV persistence + randomized
  fill/range property oracles + fill/sparse benches; asan/ubsan clean

## [0.4.1] - 2026-02-23
- Fix islet_iter segfaults: add bounds checking in islet_search()
- Fix 2D/1D Morton encoding: use dimension-specific pack functions
- Fix ISLET_MISS: now returns UINT32_MAX (was incorrectly defined as 64-bit)

## [0.4.0] - 2026-02-23
- Add comprehensive test suite
  - Unit tests: 65 tests for core functions (morton, point, islet)
  - Integration tests: 13 tests for iteration, persistence, queries
  - Property-based tests: 12K+ assertions for morton invariants
  - Stress tests: 11 tests for capacity and large datasets
  - Benchmarks: 4 benchmarks for performance measurement
  - Fuzz tests: 3 harnesses with libFuzzer support
- Add 'make test' target to root Makefile
- Update README with testing documentation

## [0.3.0] - 2026-02-23
- Update to libcorm 0.6.0
  - Improved pointer stability via allocation reuse optimization
  - File loading no longer requires CM_MIRROR flag
  - Enhanced documentation and bug fixes

## [0.2.0] - 2025-10-24
- Update to libcorm 0.5.0 (BTREE support)
