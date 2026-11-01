import { createRequire } from "node:module";
import path from "node:path";

let nativeBinding = null;
try {
  const req = createRequire(import.meta.url);
  try {
    nativeBinding = req("../build/Release/islet_node.node");
  } catch {}
  if (!nativeBinding && typeof process !== "undefined" && process.platform && process.arch) {
    try {
      nativeBinding = req(`@ttypt/islet-${process.platform}-${process.arch}`);
    } catch {}
  }
  if (!nativeBinding) {
    try {
      nativeBinding = req(path.join(process.cwd(), "build/Release/islet_node.node"));
    } catch {}
  }
} catch {
  // In-memory JS / WASM fallback
}

function getSharedWasm() {
  if (globalThis.__ttypt_shared_wasm__) {
    return globalThis.__ttypt_shared_wasm__;
  }
  try {
    const req = createRequire(import.meta.url);
    const qsysPaths = [
      path.join(path.dirname(import.meta.url.replace("file://", "")), "../../libqsys/npm/dist/index.js"),
      path.join(process.cwd(), "node_modules/@ttypt/qsys/dist/index.js"),
      "@ttypt/qsys"
    ];
    for (const p of qsysPaths) {
      try {
        const mod = req(p);
        if (mod && mod.getSharedWasm) {
          return mod.getSharedWasm();
        }
      } catch {}
    }
  } catch {}
  return null;
}

/**
 * Encode 2D point (x, y) into 64-bit Morton Z-order code
 */
export function encodeMorton2D(x, y) {
  if (nativeBinding && nativeBinding.encode2D) {
    return nativeBinding.encode2D(x, y);
  }
  const wasm = getSharedWasm();
  if (wasm) {
    return wasm.islet_wasm_encode2d(x, y);
  }

  // JS Morton-2D fallback
  let ux = BigInt((x + 32768) & 0xffff);
  let uy = BigInt((y + 32768) & 0xffff);

  function spread(v) {
    let x = v & 0xffffn;
    x = (x | (x << 16n)) & 0x0000ffff0000ffffn;
    x = (x | (x << 8n)) & 0x00ff00ff00ff00ffn;
    x = (x | (x << 4n)) & 0x0f0f0f0f0f0f0f0fn;
    x = (x | (x << 2n)) & 0x3333333333333333n;
    x = (x | (x << 1n)) & 0x5555555555555555n;
    return x;
  }

  return spread(ux) | (spread(uy) << 1n);
}

/**
 * Decode 64-bit Morton Z-order code into 2D point { x, y }
 */
export function decodeMorton2D(code) {
  if (nativeBinding && nativeBinding.decode2D) {
    return nativeBinding.decode2D(code);
  }
  const wasm = getSharedWasm();
  if (wasm) {
    const ptr = wasm.malloc(8);
    wasm.islet_wasm_decode2d(BigInt(code), ptr);
    const arr = new Int32Array(wasm.memory.buffer, ptr, 2);
    const res = { x: arr[0], y: arr[1] };
    wasm.free(ptr);
    return res;
  }

  let c = BigInt(code);
  function compact(c, shift) {
    let x = (c >> shift) & 0x5555555555555555n;
    x = (x ^ (x >> 1n)) & 0x3333333333333333n;
    x = (x ^ (x >> 2n)) & 0x0f0f0f0f0f0f0f0fn;
    x = (x ^ (x >> 4n)) & 0x00ff00ff00ff00ffn;
    x = (x ^ (x >> 8n)) & 0x0000ffff0000ffffn;
    x = (x ^ (x >> 16n)) & 0x00000000ffffffffn;
    return Number(x);
  }

  return {
    x: compact(c, 0n) - 32768,
    y: compact(c, 1n) - 32768
  };
}

/**
 * SpatialIndex: 2D Point and Bounding-Box Index
 */
export class SpatialIndex {
  constructor(filePath = "", dbName = "islet") {
    this.filePath = filePath;
    this.dbName = dbName;
    this.handle = null;
    this._memPoints = new Map();

    if (nativeBinding && nativeBinding.open) {
      this.handle = nativeBinding.open(this.filePath, this.dbName);
    }
  }

  put(x, y, ref) {
    if (nativeBinding && this.handle) {
      return nativeBinding.put2D(this.handle, x, y, ref);
    }
    const key = `${x},${y}`;
    this._memPoints.set(key, ref);
    return true;
  }

  get(x, y) {
    if (nativeBinding && this.handle) {
      return nativeBinding.get2D(this.handle, x, y);
    }
    const key = `${x},${y}`;
    return this._memPoints.get(key) || null;
  }

  searchBox(minX, minY, maxX, maxY) {
    const results = [];
    for (const [key, ref] of this._memPoints.entries()) {
      const [px, py] = key.split(",").map(Number);
      if (px >= minX && px <= maxX && py >= minY && py <= maxY) {
        results.push(ref);
      }
    }
    return results;
  }
}

/**
 * Pluggable CormAxis implementation for the Corm Recall Kernel
 */
export class IsletSpatialAxis {
  constructor(filePath = "", dbName = "islet") {
    this.name = "spatial";
    this.index = new SpatialIndex(filePath, dbName);
  }

  registerPoint(ref, x, y) {
    this.index.put(x, y, ref);
  }

  fill(params) {
    return this.index.searchBox(params.minX, params.minY, params.maxX, params.maxY);
  }
}
