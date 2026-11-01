export declare function encodeMorton2D(x: number, y: number): bigint;
export declare function decodeMorton2D(code: bigint): { x: number; y: number };

export declare class SpatialIndex {
  constructor(filePath?: string, dbName?: string);
  put(x: number, y: number, ref: number): boolean;
  get(x: number, y: number): number | null;
  searchBox(minX: number, minY: number, maxX: number, maxY: number): number[];
}

export declare class IsletSpatialAxis {
  readonly name: string;
  constructor(filePath?: string, dbName?: string);
  registerPoint(ref: number, x: number, y: number): void;
  fill(params: { minX: number; minY: number; maxX: number; maxY: number }): number[];
}
