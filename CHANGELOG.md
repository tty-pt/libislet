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
