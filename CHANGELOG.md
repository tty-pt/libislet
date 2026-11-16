## [1.6.2] - 2026-11-15

- **Quadtree Spatial Headers & Morton Box Emptiness (`include/ttypt/islet_quad.h`, `npm/include/ttypt/islet_quad.h`, `include/ttypt/islet.h`)**:
  - Packaged `<ttypt/islet_quad.h>` (`morton_quad_pack`, `morton_quad_unpack`, `morton_quad_parent`, `morton_quad_child`, `islet_quad_map_*`) and `islet_box_is_empty` in the published release.

## [1.6.1] - 2026-11-14

- **Packaging (`npm/package.json`)**:
  - Explicitly included `"prebuilds"` in `npm/package.json` `"files"`.

## [1.6.0] - 2026-11-14

- **Complete Node-API Bindings, `IsletSpatialAxis` & Quadtree Hash Primitives (`include/ttypt/islet_quad.h`, `npm/native/islet_napi.c`, `npm/dist/index.js`)**:
  - Bound `encode3D`, `decode3D`, `searchBox` (`islet_iter_2` / `islet_next`), `del2D`, `close`, and `axisStore`/`axisUnstore` in `islet_napi.c`.
  - Exported `IsletSpatialAxis` and removed `_memPoints` JS `Map` fallback in `SpatialIndex`.
  - Added `<ttypt/islet_quad.h>` header-only 2D hierarchical Morton quadtree key packing (`morton_quad_pack`, `morton_quad_unpack`, `morton_quad_parent`, `morton_quad_child`) and open-addressed power-of-two spatial hash table (`islet_quad_map_*`) across levels `[0..16]`.
  - Requires `@tty-pt/corm` >=2.4.0 (`^2.4.0`) and `@tty-pt/stele` >=1.7.0 (`^1.7.0`).

## [1.5.2] - 2026-11-13

- **Packaging**:
  - Requires `@tty-pt/corm` >=2.3.3 (`^2.3.3`) and `@tty-pt/stele` >=1.6.2 (`^1.6.2`).
  - The npm tarball is built once by CI, with a self-contained native addon per platform (`prebuilds/<os>-<cpu>/`) next to `islet.wasm`, and published as verified.

## [1.5.1] - 2026-11-13

- **Disjoint Global-Base & Shared-Memory Reactor (`islet.wasm`)**:
  - Rebuilt `islet.wasm` shared-memory reactor at a disjoint `--global-base=1048576` to prevent static data segment collisions across co-loaded WASM modules.
  - Upgraded `@tty-pt/corm` to `>=2.3.2` (`^2.3.2`) and `@tty-pt/stele` to `>=1.6.1` (`^1.6.1`).

## [1.5.0] - 2026-11-13

- **Minor Version Bump**:
  - Bumped version to 1.5.0 across `islet.pc`, `npm/islet.pc`, and `npm/package.json`.

## [1.4.1] - 2026-11-13

- **Version & Pkg-Config Sync**:
  - Synced `islet.pc` and `npm/package.json` to 1.4.1.

## [1.4.0] - 2026-11-13

- **Substrate & Corm v2.3.1 Alignment**:
  - Upgraded dependencies to @tty-pt/corm@^2.3.1 (>=2.3.1) and @tty-pt/stele@^1.6.0 (>=1.6.0).
### Added
- **Morton Box Emptiness Test (`include/ttypt/islet.h`, `src/libislet.c`)**:
  - Implemented `islet_box_is_empty` for rapid Morton box spatial query tests.
  - Added unit test suite in `tests/unit/test_islet_core.c`.

## [1.3.2] - 2026-11-03

- **Standalone `@tty-pt/islet` Package & Pure WebAssembly Engine**:
  - Published `@tty-pt/islet` providing Node-API native bindings and pure WebAssembly fallback (`islet.wasm`).
  - Added `npm/wasm/islet_wasm.c` exposing 2D/3D Morton encoding (`morton_set_2`, `morton_get_2`, `morton_set_3`, `morton_get_3`) using `ISO_EXPORT`.
  - Adopted strict `peerDependencies` (`@tty-pt/stele: ">=1.5.0"`, `@tty-pt/corm: ">=2.2.0"`) and `devDependencies` (`@tty-pt/stele: "^1.5.2"`, `@tty-pt/corm: "^2.2.4"`).
  - Migrated build to centralized `make wasm` rule in `external/mk/include.mk` with `-ffreestanding -flto -O3` and post-link `wasm-opt -O3` optimization.

## [1.3.0]

- **Build Portability & Linker Purity**:
  - Removed hardcoded `/home/quirinpa/.local/lib` RUNPATH and in-tree paths from root and test Makefiles.
  - Standardized portable `-I` include prefixes across test suites.

## [1.2.0]

- **Cormr Recall Kernel Integration**: Updated headers and tests to consume `<ttypt/corm.h>` and `<ttypt/cormr.h>`, aligning with the domain-free Recall Kernel architecture.
- **Lemma Text Indexing Alignment**: Updated ecosystem references and test fixtures from `libstoma` to `liblemma`.
- **4D Morton Dimension Support**: Added comprehensive 4D Morton encoding property tests, fast-path benchmarks, and multi-value cell enhancements.

## [1.1.0]

- **Renamed `libgeo` → `libislet`**: the `geo_*` API surface is now `islet_*` (`include/ttypt/islet.h`).
- **BREAKING — per-dimension public API**: runtime-`dim` functions are gone — `morton_set(p, dim)` is now `morton_set_1..4(p)`, `morton_get` is `morton_get_1..4`, every `point_*` is `point_*_1..4`, and every database op is `islet_put_N`/`islet_set_N`/`islet_get_N`/`islet_del_N`/`islet_del_all_N`/`islet_cell_count_N`/`islet_get_multi_N`/`islet_iter_N`/`rec_axis_fill_bbox_N` (N = 1..4, no dim argument). Invalid dims are now unrepresentable. Morton code values unchanged (1D/2D/3D bit-identical to v0.5.0).
- **New 2D × 32-bit dense config** (`islet_*_2_32`, `morton_set_2_32`/`morton_get_2_32`, `point_*_2_32`): `int32_t` lanes (−2147483648..2147483647), dense stride-2 codec filling all 64 key bits; box walker, Z-interval skip, fill, iterate and multi-value chains all support 32-bit lanes with `int32_t` start and lengths, advanced with `islet_next32`.
- **Point config objects** (`include/ttypt/pointcfg.h`): exported const structs `Point<D>_<B>` (`Point1_2`…`Point4_2` on int16 lanes, `Point2_4` on int32 lanes), each with ~20 addressable member functions — codec, point arith, `put/get/replace/del/del_all/cell_count`, `get_multi`/`islet_cell_next`, `iter`, `fill_bbox`. The flat API stays exported as the tight-loop fast path.
- **`islet_ops[1..4]`**: runtime-dim dispatch table (`islet_ops[0]` is NULL) covering codec, point, scatter, iterate and fill operations.
- **Bulk decode**: `morton_get_bulk()` / `morton_get_bulk4()`, decode-side counterparts to `morton_set_bulk`/`morton_set_bulk4` (closes the encode-only asymmetry).
- **BMI2 codec fast path**: `ISLET_USE_PDEP` (default 1, opt out with `-DISLET_USE_PDEP=0`) — PDEP/PEXT spread/compact kernels when compiled with `-mbmi2`, measured 2–4× on isolated encode/decode/round-trip; portable scalar fallback retained.
- **Recall-kernel alignment**: refs unified to `rec_ref_t` (`uint32_t`), kernel-based axis registration and storage adapters (`rec_axis_store`/`rec_axis_unstore`/`rec_axis_readback`), axis parameters exposed as CLI parameters, plus new 2D/4D unit, property and stress test coverage.
- `libqmap` → `libcorm` rename.
