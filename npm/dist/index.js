import { loadWasmModule, loadNativeAddon, getSubstrate } from "@tty-pt/stele";

const nativeBinding = loadNativeAddon(import.meta.url, "islet");
let wasmInstance = null;

export function getIsletWasm() {
  if (wasmInstance) return wasmInstance;
  wasmInstance = loadWasmModule(new URL("./islet.wasm", import.meta.url));
  return wasmInstance;
}

/**
 * Encode 2D point (x, y) into 64-bit Morton Z-order code strictly via WebAssembly
 */
export function encodeMorton2D(x, y, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.encode2D) {
    return nativeBinding.encode2D(x, y);
  }
  const wasm = getIsletWasm();
  return wasm.exports.islet_wasm_encode2d(x, y);
}

/**
 * Decode 64-bit Morton Z-order code into 2D point { x, y } strictly via WebAssembly
 */
export function decodeMorton2D(code, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.decode2D) {
    return nativeBinding.decode2D(code);
  }
  const wasm = getIsletWasm();
  const ptr = wasm.exports.malloc(8);
  wasm.exports.islet_wasm_decode2d(BigInt(code), ptr);
  const arr = new Int32Array(getSubstrate().memory.buffer, ptr, 2);
  const res = { x: arr[0], y: arr[1] };
  wasm.exports.free(ptr);
  return res;
}

/**
 * Encode 3D point (x, y, z) into 64-bit Morton Z-order code strictly via WebAssembly
 */
export function encodeMorton3D(x, y, z) {
  const wasm = getIsletWasm();
  return wasm.exports.islet_wasm_encode3d(x, y, z);
}

/**
 * Decode 64-bit Morton Z-order code into 3D point { x, y, z } strictly via WebAssembly
 */
export function decodeMorton3D(code) {
  const wasm = getIsletWasm();
  const ptr = wasm.exports.malloc(12);
  wasm.exports.islet_wasm_decode3d(BigInt(code), ptr);
  const arr = new Int32Array(getSubstrate().memory.buffer, ptr, 3);
  const res = { x: arr[0], y: arr[1], z: arr[2] };
  wasm.exports.free(ptr);
  return res;
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
    const code = String(encodeMorton2D(x, y));
    this._memPoints.set(code, ref);
    return true;
  }

  get(x, y) {
    if (nativeBinding && this.handle) {
      return nativeBinding.get2D(this.handle, x, y);
    }
    const code = String(encodeMorton2D(x, y));
    return this._memPoints.get(code) || null;
  }

  searchBox(minX, minY, maxX, maxY) {
    const results = [];
    for (const [code, ref] of this._memPoints.entries()) {
      const pt = decodeMorton2D(BigInt(code));
      if (pt.x >= minX && pt.x <= maxX && pt.y >= minY && pt.y <= maxY) {
        results.push({ x: pt.x, y: pt.y, ref });
      }
    }
    return results;
  }

  close() {
    if (nativeBinding && this.handle) {
      nativeBinding.close(this.handle);
      this.handle = null;
    }
    this._memPoints.clear();
  }
}
