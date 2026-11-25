#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ttypt/islet_quad.h>

static void test_basic_and_churn(void)
{
	islet_quad_map_t m;
	assert(islet_quad_map_init(&m, 512));
	assert(m.cap == 1024);

	/* Seed 512 live entries */
	uint64_t ring[512];
	for (uint32_t i = 0; i < 512; i++) {
		ring[i] = morton_quad_pack(0, (uint8_t)(i & 7), (int32_t)i - 256, (int32_t)(i * 3) - 512);
		assert(islet_quad_map_put(&m, ring[i], i + 100u));
	}
	assert(m.count == 512);

	/* 100,000 LRU evictions + re-insertions (would saturate every bucket with tombstones before) */
	for (uint32_t step = 0; step < 100000; step++) {
		uint32_t slot = step & 511u;
		uint64_t old_k = ring[slot];
		assert(islet_quad_map_del(&m, old_k));
		uint32_t dummy = 0;
		assert(!islet_quad_map_get(&m, old_k, &dummy));

		int32_t cx = (int32_t)((step + 512u) % 60000u) - 30000;
		int32_t cy = (int32_t)(((step + 512u) * 7u) % 60000u) - 30000;
		uint64_t new_k = morton_quad_pack(1, (uint8_t)(step & 15), cx, cy);
		ring[slot] = new_k;
		assert(islet_quad_map_put(&m, new_k, step + 1000u));
	}

	islet_quad_map_stats_t st;
	islet_quad_map_inspect(&m, &st);
	assert(st.cap == 1024);
	assert(st.live_entries == 512);
	assert(st.tombstones == 0);

	/* Verify every single live key is intact */
	uint64_t hit_steps = 0;
	for (uint32_t i = 0; i < 512; i++) {
		uint32_t val = 0, steps = 0;
		assert(islet_quad_map_get_probed(&m, ring[i], &val, &steps));
		assert(steps >= 1 && steps < m.cap);
		hit_steps += steps;
	}

	/* Verify negative lookups stay O(1) after 100k churn */
	uint64_t miss_steps = 0;
	for (uint32_t i = 0; i < 10000; i++) {
		uint64_t miss_k = morton_quad_pack(9, 2, (int32_t)i - 5000, (int32_t)(i * 13) - 20000);
		uint32_t val = 0, steps = 0;
		assert(!islet_quad_map_get_probed(&m, miss_k, &val, &steps));
		assert(steps >= 1 && steps < 64);
		miss_steps += steps;
	}

	double avg_hit = (double)hit_steps / 512.0;
	double avg_miss = (double)miss_steps / 10000.0;
	printf("[test_quad_map] after 100k churn: live=%zu tombs=%zu max_cluster=%u avg_hit=%.2f avg_miss=%.2f\n",
	       st.live_entries, st.tombstones, st.max_cluster_len, avg_hit, avg_miss);
	assert(avg_hit < 3.0);
	assert(avg_miss < 3.5);

	/* Delete all 512 entries and confirm empty */
	for (uint32_t i = 0; i < 512; i++)
		assert(islet_quad_map_del(&m, ring[i]));
	islet_quad_map_inspect(&m, &st);
	assert(st.live_entries == 0);
	assert(st.tombstones == 0);
	assert(st.max_cluster_len == 0);

	islet_quad_map_destroy(&m);
}

static void test_full_table_delete(void)
{
	islet_quad_map_t m;
	assert(islet_quad_map_init(&m, 32));
	assert(m.cap == 64);

	/* Fill 100% of buckets */
	for (uint32_t i = 0; i < 64; i++) {
		uint64_t k = morton_quad_pack(3, 0, (int32_t)i, 0);
		assert(islet_quad_map_put(&m, k, i));
	}
	assert(m.count == 64);

	/* Delete every key in reverse order (exercises wrap-around and full-table termination) */
	for (int i = 63; i >= 0; i--) {
		uint64_t k = morton_quad_pack(3, 0, i, 0);
		assert(islet_quad_map_del(&m, k));
		/* Remaining keys must still be retrievable */
		for (int j = 0; j < i; j++) {
			uint32_t v = 0;
			assert(islet_quad_map_get(&m, morton_quad_pack(3, 0, j, 0), &v));
			assert(v == (uint32_t)j);
		}
	}
	assert(m.count == 0);
	islet_quad_map_destroy(&m);
}

int main(void)
{
	test_basic_and_churn();
	test_full_table_delete();
	printf("PASS: test_quad_map\n");
	return 0;
}
