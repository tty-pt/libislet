import { loadWasmModule, loadNativeAddon, getSubstrate } from "@tty-pt/stele";

const nativeBinding = loadNativeAddon(import.meta.url, "islet");
let wasmInstance = null;

export function getIsletWasm() {
  if (wasmInstance) return wasmInstance;
  wasmInstance = loadWasmModule(new URL("./islet.wasm", import.meta.url));
  return wasmInstance;
}

/**
 * Encode 2D point (x, y) into 64-bit Morton Z-order code
 */
export function encodeMorton2D(x, y, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.encode2D) {
    return nativeBinding.encode2D(x, y);
  }
  const wasm = getIsletWasm();
  return wasm.exports.islet_wasm_encode2d(x, y);
}

/**
 * Decode 64-bit Morton Z-order code into 2D point { x, y }
 */
export function decodeMorton2D(code, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.decode2D) {
    return nativeBinding.decode2D(BigInt(code));
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
 * Encode 3D point (x, y, z) into 64-bit Morton Z-order code
 */
export function encodeMorton3D(x, y, z, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.encode3D) {
    return nativeBinding.encode3D(x, y, z);
  }
  const wasm = getIsletWasm();
  return wasm.exports.islet_wasm_encode3d(x, y, z);
}

/**
 * Decode 64-bit Morton Z-order code into 3D point { x, y, z }
 */
export function decodeMorton3D(code, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.decode3D) {
    return nativeBinding.decode3D(BigInt(code));
  }
  const wasm = getIsletWasm();
  const ptr = wasm.exports.malloc(12);
  wasm.exports.islet_wasm_decode3d(BigInt(code), ptr);
  const arr = new Int32Array(getSubstrate().memory.buffer, ptr, 3);
  const res = { x: arr[0], y: arr[1], z: arr[2] };
  wasm.exports.free(ptr);
  return res;
}

/**
 * SpatialIndex: 2D Point and Bounding-Box Index backed by libislet C engine
 */
export class SpatialIndex {
  constructor(filePath = "", dbName = "islet") {
    if (!nativeBinding || !nativeBinding.open) {
      throw new Error("@tty-pt/islet native addon is required for SpatialIndex");
    }
    this.filePath = filePath;
    this.dbName = dbName;
    this.handle = nativeBinding.open(this.filePath, this.dbName);
  }

  put(x, y, ref) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    return nativeBinding.put2D(this.handle, x, y, ref);
  }

  get(x, y) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    return nativeBinding.get2D(this.handle, x, y);
  }

  del(x, y, ref) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    if (ref !== undefined) {
      return nativeBinding.del2D(this.handle, x, y, ref);
    }
    return nativeBinding.del2D(this.handle, x, y);
  }

  searchBox(minX, minY, maxX, maxY) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    return nativeBinding.searchBox(this.handle, minX, minY, maxX, maxY);
  }

  fillBox(minX, minY, maxX, maxY) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    return nativeBinding.fillBox(this.handle, minX, minY, maxX, maxY);
  }

  storeSpec(ref, pointListSpec) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    return nativeBinding.axisStore(this.handle, ref, pointListSpec);
  }

  unstore(ref) {
    if (!this.handle) throw new Error("SpatialIndex is closed");
    return nativeBinding.axisUnstore(this.handle, ref);
  }

  close() {
    if (this.handle) {
      nativeBinding.close(this.handle);
      this.handle = null;
    }
  }
}

/**
 * IsletSpatialAxis: Recall axis adapter backed by libislet C engine
 */
export class IsletSpatialAxis {
  constructor(filePath = "", dbName = "islet") {
    this.name = "islet";
    this.index = new SpatialIndex(filePath, dbName);
  }

  registerPoint(ref, x, y) {
    this.index.put(x, y, ref);
  }

  store(ref, pointListSpec) {
    return this.index.storeSpec(ref, pointListSpec);
  }

  unstore(ref) {
    return this.index.unstore(ref);
  }

  fill(params) {
    const { minX = 0, minY = 0, maxX = 0, maxY = 0 } = params || {};
    return this.index.fillBox(minX, minY, maxX, maxY);
  }

  close() {
    this.index.close();
  }
}
