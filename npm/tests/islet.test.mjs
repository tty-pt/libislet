import test from "node:test";
import assert from "node:assert/strict";
import {
  encodeMorton2D,
  decodeMorton2D,
  encodeMorton3D,
  decodeMorton3D,
  SpatialIndex,
  IsletSpatialAxis,
} from "../dist/index.js";

test("encodeMorton2D and decodeMorton2D roundtrip preservation", () => {
  const points = [
    { x: 0, y: 0 },
    { x: 100, y: 200 },
    { x: -50, y: 75 },
    { x: 3000, y: -4000 }
  ];

  for (const pt of points) {
    const code = encodeMorton2D(pt.x, pt.y);
    assert.ok(typeof code === "bigint", "Code must be BigInt");
    const decoded = decodeMorton2D(code);
    assert.equal(decoded.x, pt.x, `x must match for (${pt.x}, ${pt.y})`);
    assert.equal(decoded.y, pt.y, `y must match for (${pt.x}, ${pt.y})`);
  }
});

test("encodeMorton3D and decodeMorton3D roundtrip preservation", () => {
  const points = [
    { x: 0, y: 0, z: 0 },
    { x: 10, y: 20, z: 30 },
    { x: -120, y: 45, z: -900 }
  ];

  for (const pt of points) {
    const code = encodeMorton3D(pt.x, pt.y, pt.z);
    assert.ok(typeof code === "bigint", "3D code must be BigInt");
    const decoded = decodeMorton3D(code);
    assert.deepEqual(decoded, pt);
  }
});

test("SpatialIndex put, get, searchBox, del, and close", () => {
  const idx = new SpatialIndex();
  idx.put(10, 20, 101);
  idx.put(30, 40, 102);
  idx.put(150, 250, 103);

  assert.equal(idx.get(10, 20), 101);
  assert.equal(idx.get(30, 40), 102);
  assert.equal(idx.get(99, 99), null);

  const hits = idx.searchBox(0, 0, 50, 50);
  assert.equal(hits.length, 2);
  const refs = hits.map((h) => h.ref).sort((a, b) => a - b);
  assert.deepEqual(refs, [101, 102]);

  idx.del(10, 20, 101);
  assert.equal(idx.get(10, 20), null);
  idx.close();
});

test("IsletSpatialAxis store, fill, and unstore", () => {
  const axis = new IsletSpatialAxis();
  assert.equal(axis.store(7, "10,10;20,20"), true);
  axis.registerPoint(8, 15, 15);

  const refs = axis.fill({ minX: 5, minY: 5, maxX: 25, maxY: 25 });
  assert.deepEqual(refs, [7, 8]);

  assert.equal(axis.unstore(7), true);
  const afterRefs = axis.fill({ minX: 5, minY: 5, maxX: 25, maxY: 25 });
  assert.deepEqual(afterRefs, [8]);
  axis.close();
});
