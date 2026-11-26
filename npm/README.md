<p align="center">
  <img src="https://raw.githubusercontent.com/tty-pt/libislet/main/assets/logo.png" alt="@tty-pt/islet logo" width="220" />
</p>

# @tty-pt/islet

[![npm version](https://img.shields.io/badge/npm-v1.6.2-CB3837?logo=npm)](#)
[![License: BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![TypeScript Ready](https://img.shields.io/badge/TypeScript-Ready-3178C6?logo=typescript)](#)
[![Node.js Native](https://img.shields.io/badge/Node.js-N--API-339933?logo=node.js)](#)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-Reactor-654FF0?logo=webassembly)](#)
[![Zero Dependencies](https://img.shields.io/badge/dependencies-0-success)](#)

> Universal Node.js and Browser bindings for **`libislet`**, the high-performance C99 spatial indexing engine and Morton Z-order curve coordinate mapper. Represents the canonical **Space Axis** in the Corm Recall Kernel. Canonical C headers: `<ttypt/islet.h>` and `<ttypt/islet_quad.h>`.

📖 **Full C & Spatial Index Documentation**: See the main [**libislet GitHub README**](https://github.com/tty-pt/libislet#readme) for 2D/3D Morton Z-order curve bit-interleaving, interval bounding boxes, and C API details.

---

## 1. Quickstart & Installation

```bash
npm install @tty-pt/islet
```

**Native OS Packages (`tty.pt`)**: Prebuilt native C packages (`libislet`) for Linux (`apt`, `apk`, `pacman`, `dnf`), macOS (`brew`), Windows (`winget`, MSYS2), and OpenBSD (`pkg_add`) are available via the [**tty.pt package repositories**](https://github.com/tty-pt/ci/blob/main/docs/install.md).

```typescript
import { encodeMorton2D, decodeMorton2D, SpatialIndex } from "@tty-pt/islet";

// 1. Map 2D coordinates to 64-bit Morton code preserving spatial locality
const code = encodeMorton2D(100, 200);
console.log("Morton code:", code);

// 2. Decode back to original coordinates
const { x, y } = decodeMorton2D(code);
console.log(`Decoded: (${x}, ${y})`); // (100, 200)

// 3. Spatial entity indexing & bounding box range queries
const index = new SpatialIndex("./floorplan.islet");
index.put(15, 25, 101 /* Desk 101 */);
index.put(40, 50, 102 /* Desk 102 */);

const deskId = index.get(15, 25); // 101
```

---

## 2. Practical Use Cases

- **Coworking & Blueprint Seat Selection**: Spatial neighbor calculations and coordinate lookup for desk and office blueprints.
- **Canvas & Viewport Culling**: Efficiently find and render only visible entities within a viewport bounding box at 120 FPS.
- **Geofencing & Proximity Queries**: Fast 1D range queries over 2D/3D coordinate spaces via Morton space-filling curves.

---

## 3. JavaScript / TypeScript API Reference

### `encodeMorton2D(x: number, y: number): bigint | number` / `decodeMorton2D(code: bigint | number): { x: number, y: number }`
Interleaves and reconstructs 2D coordinates `(x, y)` using a 64-bit Morton Z-order curve index. **Executes strictly in compiled WebAssembly / C**.

### `encodeMorton3D(x: number, y: number, z: number): bigint | number` / `decodeMorton3D(code: bigint | number): { x: number, y: number, z: number }`
Interleaves and reconstructs 3D coordinates `(x, y, z)` using a 64-bit Morton Z-order curve index.

### `SpatialIndex`
Spatial bounding-box and coordinate lookup index:
* `constructor(filePath?: string, dbName?: string)`: Opens database or creates in-memory index.
* `.put(x: number, y: number, ref: number): boolean`: Associates 2D coordinate `(x, y)` with entity reference `ref`.
* `.get(x: number, y: number): number | null`: Retrieves entity reference at exact coordinate.
* `.del(x: number, y: number, ref?: number): boolean`: Removes coordinate entry (`del2D`).
* `.searchBox(minX: number, minY: number, maxX: number, maxY: number): Array<{ x: number, y: number, ref: number }>`: Range query returning all entities within the axis-aligned bounding box.
* `.close(): void`: Closes database handle.

### `IsletSpatialAxis`
Plug-and-play Space Axis adapter for the Corm Recall Kernel (`@tty-pt/corm`):
* `.registerPoint(ref: number, x: number, y: number): boolean`
* `.unstore(ref: number): boolean`
* `.fill(params: { minX: number, minY: number, maxX: number, maxY: number }): number[]`

### `getIsletWasm(): WebAssembly.Instance`
Returns the WebAssembly instance loaded on the Stele shared-memory substrate.

---

## 4. Dual-Engine Runtime

- **Node.js**: Native C99 N-API addon (`build/Release/islet_node.node`) with hardware bitwise popcount/spread instructions.
- **Browser & Edge**: Precompiled `dist/islet.wasm` reactor with zero native compilation prerequisites.

---

## 5. C Development Headers & Native Integration

```makefile
CFLAGS += -I./node_modules/@tty-pt/islet/include
LDLIBS += -lislet
```

---

## License

BSD-2-Clause — Copyright (c) 2026, Paulo Quirino <quirinpa@gmail.com>. See [LICENSE](https://github.com/tty-pt/libislet/blob/main/LICENSE).
