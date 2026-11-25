#ifndef TTYPT_ISLET_QUAD_H
#define TTYPT_ISLET_QUAD_H

/**
 * @file islet_quad.h
 * @brief Generic Spatial Quadtree Hash Table & 2D Morton Key Primitives
 *
 * Provides:
 * - Hierarchical 2D Morton (Z-order) quadtree key encoding, decoding, parent,
 *   child, and quadrant operations across multi-world levels [0..16].
 * - A lightweight, power-of-two open-addressed spatial hash map keyed by 64-bit
 *   hierarchical Morton keys, supporting O(1) lookup, insertion, deletion, and
 *   nearest-populated-ancestor queries without heap re-allocations during frame
 *   updates.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. 2D Morton / Z-Order Hierarchical Key Encoding
 * ============================================================================ */

#ifndef MORTON_QUAD_DEFINED
#define MORTON_QUAD_DEFINED

static inline int32_t morton_fdiv(int32_t a, int32_t b)
{
	int32_t q = a / b;
	int32_t r = a % b;
	if (r != 0 && ((a < 0) ^ (b < 0)))
		q--;
	return q;
}

static inline uint32_t morton_spread_dense16(uint16_t v)
{
#if defined(__BMI2__)
	return (uint32_t)__builtin_ia32_pdep_di(v, 0x55555555ULL);
#else
	uint32_t x = v;
	x = (x | (x << 8)) & 0x00FF00FFu;
	x = (x | (x << 4)) & 0x0F0F0F0Fu;
	x = (x | (x << 2)) & 0x33333333u;
	x = (x | (x << 1)) & 0x55555555u;
	return x;
#endif
}

static inline uint16_t morton_compact_dense16(uint32_t v)
{
#if defined(__BMI2__)
	return (uint16_t)__builtin_ia32_pext_di(v, 0x55555555ULL);
#else
	uint32_t x = v & 0x55555555u;
	x = (x | (x >> 1)) & 0x33333333u;
	x = (x | (x >> 2)) & 0x0F0F0F0Fu;
	x = (x | (x >> 4)) & 0x00FF00FFu;
	x = (x | (x >> 8)) & 0x0000FFFFu;
	return (uint16_t)x;
#endif
}

#define MORTON_QUAD_BIAS     32768

static inline uint64_t morton_quad_pack(uint16_t world, uint8_t level, int32_t cell_x, int32_t cell_y)
{
	int32_t clamped_x = cell_x < -32768 ? -32768 : (cell_x > 32767 ? 32767 : cell_x);
	int32_t clamped_y = cell_y < -32768 ? -32768 : (cell_y > 32767 ? 32767 : cell_y);
	uint16_t ux = (uint16_t)(clamped_x + MORTON_QUAD_BIAS);
	uint16_t uy = (uint16_t)(clamped_y + MORTON_QUAD_BIAS);
	uint32_t m = morton_spread_dense16(ux) | (morton_spread_dense16(uy) << 1);
	uint64_t key = ((uint64_t)(world & 0x3FFF) << 40) | ((uint64_t)(level & 0x1F) << 32) | (uint64_t)m;
	return key;
}

static inline void morton_quad_unpack(uint64_t key, uint16_t *world, uint8_t *level, int32_t *cell_x, int32_t *cell_y)
{
	uint8_t lvl = (uint8_t)((key >> 32) & 0x1F);
	uint32_t m = (uint32_t)(key & 0xFFFFFFFFu);
	uint16_t ux = morton_compact_dense16(m);
	uint16_t uy = morton_compact_dense16(m >> 1);
	if (world) *world = (uint16_t)((key >> 40) & 0x3FFF);
	if (level) *level = lvl;
	if (cell_x) *cell_x = (int32_t)ux - MORTON_QUAD_BIAS;
	if (cell_y) *cell_y = (int32_t)uy - MORTON_QUAD_BIAS;
}

static inline uint64_t morton_quad_key(uint16_t world, uint8_t level, int32_t ox, int32_t oy, int32_t base_w, int32_t base_h)
{
	int32_t w = base_w << (level < 16 ? level : 16);
	int32_t h = base_h << (level < 16 ? level : 16);
	int32_t cx = (w > 0) ? morton_fdiv(ox, w) : 0;
	int32_t cy = (h > 0) ? morton_fdiv(oy, h) : 0;
	return morton_quad_pack(world, level, cx, cy);
}

static inline void morton_quad_decode(uint64_t key, int32_t base_w, int32_t base_h,
                                      uint16_t *world, uint8_t *level, int32_t *ox, int32_t *oy)
{
	int32_t cx = 0, cy = 0;
	uint8_t lvl = 0;
	morton_quad_unpack(key, world, &lvl, &cx, &cy);
	if (level) *level = lvl;
	int32_t w = base_w << (lvl < 16 ? lvl : 16);
	int32_t h = base_h << (lvl < 16 ? lvl : 16);
	if (ox) *ox = cx * w;
	if (oy) *oy = cy * h;
}

static inline uint64_t morton_quad_parent(uint64_t key)
{
	uint16_t world = 0;
	uint8_t lvl = 0;
	int32_t cx = 0, cy = 0;
	morton_quad_unpack(key, &world, &lvl, &cx, &cy);
	if (lvl >= 16) return key;
	return morton_quad_pack(world, (uint8_t)(lvl + 1), morton_fdiv(cx, 2), morton_fdiv(cy, 2));
}

static inline uint64_t morton_quad_child(uint64_t key, uint8_t quadrant)
{
	uint16_t world = 0;
	uint8_t lvl = 0;
	int32_t cx = 0, cy = 0;
	morton_quad_unpack(key, &world, &lvl, &cx, &cy);
	if (lvl == 0) return key;
	int32_t child_cx = (cx << 1) | (quadrant & 1);
	int32_t child_cy = (cy << 1) | ((quadrant >> 1) & 1);
	return morton_quad_pack(world, (uint8_t)(lvl - 1), child_cx, child_cy);
}

static inline uint8_t morton_quad_quadrant(uint64_t key)
{
	int32_t cx = 0, cy = 0;
	morton_quad_unpack(key, NULL, NULL, &cx, &cy);
	return (uint8_t)((cx & 1) | ((cy & 1) << 1));
}

#endif /* MORTON_QUAD_DEFINED */

/* ============================================================================
 * 2. Open-Addressed Spatial Quadtree Hash Map
 * ============================================================================ */

#define ISLET_QUAD_EMPTY     0u
#define ISLET_QUAD_TOMBSTONE 1u

typedef struct {
	uint64_t *keys;
	uint32_t *vals;
	uint8_t  *states;
	size_t    cap;
	size_t    mask;
	size_t    count;
} islet_quad_map_t;

static inline uint64_t islet_quad_hash64(uint64_t x)
{
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	x = x ^ (x >> 31);
	return x;
}

static inline bool islet_quad_map_init(islet_quad_map_t *m, size_t min_cap)
{
	if (!m) return false;
	size_t cap = 64;
	while (cap < min_cap * 2)
		cap <<= 1;

	m->keys = (uint64_t *)calloc(cap, sizeof(uint64_t));
	m->vals = (uint32_t *)calloc(cap, sizeof(uint32_t));
	m->states = (uint8_t *)calloc(cap, sizeof(uint8_t));
	if (!m->keys || !m->vals || !m->states) {
		free(m->keys);
		free(m->vals);
		free(m->states);
		memset(m, 0, sizeof(*m));
		return false;
	}
	m->cap = cap;
	m->mask = cap - 1;
	m->count = 0;
	return true;
}

static inline void islet_quad_map_destroy(islet_quad_map_t *m)
{
	if (!m) return;
	free(m->keys);
	free(m->vals);
	free(m->states);
	memset(m, 0, sizeof(*m));
}

static inline void islet_quad_map_clear(islet_quad_map_t *m)
{
	if (!m || !m->states) return;
	memset(m->states, 0, m->cap * sizeof(uint8_t));
	m->count = 0;
}

typedef struct {
	size_t   cap;
	size_t   live_entries;
	size_t   tombstones;
	uint32_t max_cluster_len;
	size_t   bytes_allocated;
} islet_quad_map_stats_t;

static inline bool islet_quad_map_get_probed(const islet_quad_map_t *m, uint64_t key,
                                             uint32_t *out_val, uint32_t *out_steps)
{
	if (!m || !m->states || m->cap == 0) {
		if (out_steps) *out_steps = 0;
		return false;
	}
	size_t idx = (size_t)(islet_quad_hash64(key) & m->mask);
	for (size_t step = 0; step < m->cap; step++) {
		uint8_t st = m->states[idx];
		if (st == ISLET_QUAD_EMPTY) {
			if (out_steps) *out_steps = (uint32_t)(step + 1);
			return false;
		}
		if (st == 2u && m->keys[idx] == key) {
			if (out_val) *out_val = m->vals[idx];
			if (out_steps) *out_steps = (uint32_t)(step + 1);
			return true;
		}
		idx = (idx + 1) & m->mask;
	}
	if (out_steps) *out_steps = (uint32_t)m->cap;
	return false;
}

static inline bool islet_quad_map_get(const islet_quad_map_t *m, uint64_t key, uint32_t *out_val)
{
	return islet_quad_map_get_probed(m, key, out_val, NULL);
}

static inline bool islet_quad_map_put(islet_quad_map_t *m, uint64_t key, uint32_t val)
{
	if (!m || !m->states || m->cap == 0) return false;
	size_t idx = (size_t)(islet_quad_hash64(key) & m->mask);
	size_t first_tomb = m->cap;

	for (size_t step = 0; step < m->cap; step++) {
		uint8_t st = m->states[idx];
		if (st == ISLET_QUAD_EMPTY) {
			size_t target = (first_tomb < m->cap) ? first_tomb : idx;
			m->keys[target] = key;
			m->vals[target] = val;
			m->states[target] = 2u;
			m->count++;
			return true;
		}
		if (st == ISLET_QUAD_TOMBSTONE) {
			if (first_tomb == m->cap) first_tomb = idx;
		} else if (m->keys[idx] == key) {
			m->vals[idx] = val;
			return true;
		}
		idx = (idx + 1) & m->mask;
	}

	if (first_tomb < m->cap) {
		size_t target = first_tomb;
		m->keys[target] = key;
		m->vals[target] = val;
		m->states[target] = 2u;
		m->count++;
		return true;
	}
	return false;
}

static inline bool islet_quad_map_del(islet_quad_map_t *m, uint64_t key)
{
	if (!m || !m->states || m->cap == 0) return false;
	size_t idx = (size_t)(islet_quad_hash64(key) & m->mask);
	for (size_t step = 0; step < m->cap; step++) {
		uint8_t st = m->states[idx];
		if (st == ISLET_QUAD_EMPTY)
			return false;
		if (st == 2u && m->keys[idx] == key) {
			/* Backward-shift deletion: closes the hole without tombstones
			 * so negative lookups never degrade across sustained LRU churn. */
			size_t hole = idx;
			size_t cur = (hole + 1) & m->mask;
			for (size_t s = 1; s < m->cap && m->states[cur] == 2u; s++) {
				size_t home = (size_t)(islet_quad_hash64(m->keys[cur]) & m->mask);
				if (((hole - home) & m->mask) < ((cur - home) & m->mask)) {
					m->keys[hole] = m->keys[cur];
					m->vals[hole] = m->vals[cur];
					hole = cur;
				}
				cur = (cur + 1) & m->mask;
			}
			m->states[hole] = ISLET_QUAD_EMPTY;
			if (m->count > 0) m->count--;
			return true;
		}
		idx = (idx + 1) & m->mask;
	}
	return false;
}

static inline void islet_quad_map_inspect(const islet_quad_map_t *m, islet_quad_map_stats_t *out)
{
	if (!out) return;
	memset(out, 0, sizeof(*out));
	if (!m || !m->states || m->cap == 0) return;

	out->cap = m->cap;
	out->live_entries = m->count;
	out->bytes_allocated = m->cap * (sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));

	uint32_t run = 0;
	uint32_t max_run = 0;
	size_t tombs = 0;
	/* Scan twice cap (stopping cleanly after wrap) to capture wrap-around clusters */
	for (size_t i = 0; i < m->cap; i++) {
		uint8_t st = m->states[i];
		if (st == ISLET_QUAD_TOMBSTONE) {
			tombs++;
			run++;
		} else if (st == 2u) {
			run++;
		} else {
			if (run > max_run) max_run = run;
			run = 0;
		}
	}
	if (run > 0 && run < m->cap) {
		for (size_t i = 0; i < m->cap && m->states[i] != ISLET_QUAD_EMPTY; i++)
			run++;
	}
	if (run > max_run) max_run = run;
	out->tombstones = tombs;
	out->max_cluster_len = max_run;
}

/* Hierarchical ancestor query: walks from key's level up to max_level to find finest cached node */
static inline bool islet_quad_map_find_ancestor(const islet_quad_map_t *m, uint64_t leaf_key,
                                                uint8_t max_level, uint32_t *out_val, uint8_t *out_level)
{
	if (!m) return false;
	uint64_t k = leaf_key;
	uint8_t lvl = (uint8_t)((k >> 32) & 0x1F);
	while (lvl <= max_level) {
		if (islet_quad_map_get(m, k, out_val)) {
			if (out_level) *out_level = lvl;
			return true;
		}
		if (lvl >= max_level || lvl >= 16)
			break;
		k = morton_quad_parent(k);
		lvl++;
	}
	return false;
}

#ifdef __cplusplus
}
#endif

#endif /* TTYPT_ISLET_QUAD_H */
