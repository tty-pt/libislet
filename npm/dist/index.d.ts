export declare function encodeMorton2D(x: number, y: number, options?: { wasm?: boolean }): bigint;
export declare function decodeMorton2D(code: bigint, options?: { wasm?: boolean }): { x: number; y: number };
export declare function encodeMorton3D(x: number, y: number, z: number, options?: { wasm?: boolean }): bigint;
export declare function decodeMorton3D(code: bigint, options?: { wasm?: boolean }): { x: number; y: number; z: number };

export declare class SpatialIndex {
  constructor(filePath?: string, dbName?: string);
  put(x: number, y: number, ref: number): boolean;
  get(x: number, y: number): number | null;
  del(x: number, y: number, ref?: number): boolean;
  searchBox(minX: number, minY: number, maxX: number, maxY: number): Array<{ x: number; y: number; ref: number }>;
  fillBox(minX: number, minY: number, maxX: number, maxY: number): number[];
  storeSpec(ref: number, pointListSpec: string): boolean;
  unstore(ref: number): boolean;
  close(): void;
}

export declare class IsletSpatialAxis {
  readonly name: string;
  constructor(filePath?: string, dbName?: string);
  registerPoint(ref: number, x: number, y: number): void;
  store(ref: number, pointListSpec: string): boolean;
  unstore(ref: number): boolean;
  fill(params: { minX: number; minY: number; maxX: number; maxY: number }): number[];
  close(): void;
}
