/*
 * test_axis_roundtrip.c — 2A-5: file-backed store, cross-process round-trip.
 *
 * Self-exec'ing harness — orchestrator fork+exec's itself per phase (fresh
 * process, so reopen reads what the prior phase's corm_save()+destructor
 * flushed to disk; never corm_close). Proves islet file-backed lifecycle
 * including the .ridx rev rehydration never yet tested cross-process:
 *
 *   seed     → rec_axis_store(11,"1,2;4,5") and (12,"7,8") via file-backed ctx
 *   verify1  → reopen (fresh proc) → readback(11)=1,2 NUL 4,5, fill_bbox full={11,12} tight={11}
 *   unstore  → reopen → rec_axis_unstore(11)
 *   verify2  → reopen → readback(11) NULL, fill_bbox full={12}
 */

#include "../test_common.h"
#include "../../include/ttypt/islet.h"
#include <ttypt/corm.h>
#include <ttypt/rec.h>

#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define DB_PATH   "/tmp/test_islet_roundtrip.db"
#define RIDX_PATH "/tmp/test_islet_roundtrip.db.ridx"
#define SPEC      DB_PATH "::1023"

static void
unlink_stale(void)
{
	unlink(DB_PATH);
	unlink(RIDX_PATH);
}

static int
has_ref(rec_set_t *s, rec_ref_t want)
{
	size_t n = rec_set_count(s);
	const rec_ref_t *a = rec_set_at(s);
	for (size_t i = 0; i < n; i++)
		if (a[i] == want)
			return 1;
	return 0;
}

static int
phase_seed(void)
{
	islet_init();
	void *ctx = rec_axis_open(SPEC);
	if (!ctx)
		return 1;
	uint32_t db = (uint32_t)(uintptr_t)ctx;
	(void)db;
	if (rec_axis_store(ctx, NULL, 11, "1,2;4,5") != 0)
		return 1;
	if (rec_axis_store(ctx, NULL, 12, "7,8") != 0)
		return 1;
	{
		char *blob = NULL;
		size_t n = 0;
		if (rec_axis_readback(ctx, 11, &blob, &n) != 0 || !blob)
			return 1;
		free(blob);
	}
	corm_save();
	return 0;
}

static int
phase_verify1(void)
{
	islet_init();
	void *ctx = rec_axis_open(SPEC);
	if (!ctx)
		return 1;
	uint32_t db = (uint32_t)(uintptr_t)ctx;

	/* .ridx rehydration: readback after fresh open must equal what was stored */
	{
		char *blob = NULL;
		size_t n = 0;
		if (rec_axis_readback(ctx, 11, &blob, &n) != 0)
			return 1;
		if (!blob || n != 8) /* "1,2\0" (4) + "4,5\0" (4) */
			return 1;
		if (memcmp(blob, "1,2\0004,5\000", 8) != 0)
			return 1;
		free(blob);
	}
	{
		char *blob = NULL;
		size_t n = 0;
		if (rec_axis_readback(ctx, 12, &blob, &n) != 0)
			return 1;
		if (!blob || strcmp(blob, "7,8") != 0)
			return 1;
		free(blob);
	}
	/* fill_bbox probes */
	{
		rec_set_t *s = rec_set_new();
		int16_t origin[2] = {0, 0};
		uint16_t len[2] = {10, 10};
		if (!s)
			return 1;
		if (rec_axis_fill_bbox_2(db, origin, len, s) != 0)
			return 1;
		if (rec_set_count(s) != 2)
			return 1;
		if (!has_ref(s, 11) || !has_ref(s, 12))
			return 1;
		rec_set_free(s);
	}
	{
		rec_set_t *s = rec_set_new();
		int16_t origin[2] = {2, 2};
		uint16_t len[2] = {5, 5}; /* contains (4,5) but not (7,8) nor (1,2) */
		if (!s)
			return 1;
		if (rec_axis_fill_bbox_2(db, origin, len, s) != 0)
			return 1;
		if (rec_set_count(s) != 1 || !has_ref(s, 11))
			return 1;
		rec_set_free(s);
	}
	return 0;
}

static int
phase_unstore(void)
{
	islet_init();
	void *ctx = rec_axis_open(SPEC);
	if (!ctx)
		return 1;
	if (rec_axis_unstore(ctx, 11) != 0)
		return 1;
	/* isolation: 12 still readable in this phase (pre-save) */
	{
		char *blob = NULL;
		size_t n = 0;
		if (rec_axis_readback(ctx, 12, &blob, &n) != 0 || !blob)
			return 1;
		free(blob);
	}
	corm_save();
	return 0;
}

static int
phase_verify2(void)
{
	islet_init();
	void *ctx = rec_axis_open(SPEC);
	if (!ctx)
		return 1;
	uint32_t db = (uint32_t)(uintptr_t)ctx;

	{
		char *blob = (char *)0x1;
		size_t n = 1;
		if (rec_axis_readback(ctx, 11, &blob, &n) != 0)
			return 1;
		if (blob != NULL || n != 0)
			return 1;
	}
	{
		rec_set_t *s = rec_set_new();
		int16_t origin[2] = {0, 0};
		uint16_t len[2] = {10, 10};
		if (!s)
			return 1;
		if (rec_axis_fill_bbox_2(db, origin, len, s) != 0)
			return 1;
		if (rec_set_count(s) != 1 || !has_ref(s, 12))
			return 1;
		rec_set_free(s);
	}
	/* tight box that previously held 11 now empty */
	{
		rec_set_t *s = rec_set_new();
		int16_t origin[2] = {2, 2};
		uint16_t len[2] = {5, 5};
		if (!s)
			return 1;
		if (rec_axis_fill_bbox_2(db, origin, len, s) != 0)
			return 1;
		if (rec_set_count(s) != 0)
			return 1;
		rec_set_free(s);
	}
	return 0;
}

static int
run_phase(const char *prog, const char *mode)
{
	pid_t pid;
	int st = 1;
	pid = fork();
	if (pid < 0)
		return 1;
	if (pid == 0) {
		execl(prog, prog, mode, (char *)NULL);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return 1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

int
main(int argc, char **argv)
{
	const char *mode = argc > 1 ? argv[1] : NULL;
	if (mode) {
		if (strcmp(mode, "seed") == 0)
			return phase_seed();
		if (strcmp(mode, "verify1") == 0)
			return phase_verify1();
		if (strcmp(mode, "unstore") == 0)
			return phase_unstore();
		if (strcmp(mode, "verify2") == 0)
			return phase_verify2();
		fprintf(stderr, "unknown mode: %s\n", mode);
		return 2;
	}
	printf("=== 2A-5 islet file-backed cross-process round-trip ===\n");
	unlink_stale();
	if (run_phase(argv[0], "seed") != 0) {
		printf("seed phase failed\n");
		return 1;
	}
	if (run_phase(argv[0], "verify1") != 0) {
		printf("reopen → query finds ref: FAILED\n");
		return 1;
	}
	printf("reopen → query finds ref: ok (.ridx rehydrated, fill_bbox correct)\n");
	if (run_phase(argv[0], "unstore") != 0) {
		printf("unstore phase failed\n");
		return 1;
	}
	if (run_phase(argv[0], "verify2") != 0) {
		printf("reopen → absent: FAILED\n");
		return 1;
	}
	printf("reopen → absent after unstore: ok\n");
	unlink_stale();
	printf("ALL ROUND-TRIP PHASES PASSED\n");
	return 0;
}
