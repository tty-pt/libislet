import test from "node:test";
import assert from "node:assert/strict";
import { encodeMorton2D, decodeMorton2D, SpatialIndex } from "../dist/index.js";

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

test("SpatialIndex put and get", () => {
  const idx = new SpatialIndex();
  idx.put(10, 20, 101);
  idx.put(30, 40, 102);

  assert.equal(idx.get(10, 20), 101);
  assert.equal(idx.get(30, 40), 102);
  assert.equal(idx.get(99, 99), null);
});
