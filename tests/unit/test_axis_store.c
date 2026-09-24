/**
 * @file test_axis_store.c
 * @brief Tests for the phase-2A store half: `islet_del_value_N` family +
 *        `rec_axis_store` / `rec_axis_unstore` / `rec_axis_readback`.
 *
 * TDD slice 2A-4 (site mm-plan/PHASE-2-CLI.md): the CLI passes the whole
 * `-p` string as (ref, value) and the axis parses it entirely — a point
 * list "x,y[,z];x2,y2" (int16 lanes, dim from the first point). Store is
 * replace-in-place; unstore walks the `.ridx` rev manifest backwards
 * (O(cells-of-ref)); readback is the ref's cells NUL-joined, round-
 * trippable.
 */

#include "../test_common.h"
#include "../../include/ttypt/islet.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static void setup_once(void) {
    static int initialized = 0;
    if (!initialized) {
        islet_init();
        initialized = 1;
    }
}

/* Count the NUL-joined entries in a read-back blob. */
static size_t blob_entries(const char *blob, size_t n) {
    size_t i, c = 0;
    for (i = 0; i < n; i++)
        if (!blob[i])
            c++;
    return c;
}

TEST(store_readback_roundtrip) {
    setup_once();
    void *ctx = rec_axis_open(":test_store_roundtrip:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;

    ASSERT_EQ(rec_axis_store(ctx, NULL, 11, "1,2,3;4,5,6"), 0);

    char *blob = NULL;
    size_t n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 11, &blob, &n), 0);
    ASSERT_NOT_NULL(blob);
    ASSERT_EQ(n, (size_t)12); /* "1,2,3\0" + "4,5,6\0" */
    ASSERT_EQ(blob_entries(blob, n), (size_t)2);
    ASSERT(memcmp(blob, "1,2,3\0""4,5,6\0", 12) == 0);
    free(blob);

    /* Grid cross-checks: the ref is recorded at both cells. */
    {
        int16_t p1[3] = {1, 2, 3}, p2[3] = {4, 5, 6};
        ASSERT_EQ(islet_get_3(db, p1), (uint32_t)11);
        ASSERT_EQ(islet_get_3(db, p2), (uint32_t)11);

        rec_set_t *out = rec_set_new();
        int16_t s[3] = {0, 0, 0};
        uint16_t l[3] = {10, 10, 10};
        ASSERT_EQ(rec_axis_fill_bbox_3(db, s, l, out), 0);
        ASSERT_EQ(rec_set_count(out), (size_t)1);
        ASSERT_EQ(rec_set_at(out)[0], (rec_ref_t)11);
        rec_set_free(out);
    }

    ASSERT_EQ(rec_axis_unstore(ctx, 11), 0);

    blob = NULL; n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 11, &blob, &n), 0);
    ASSERT_NULL(blob);
    ASSERT_EQ(n, (size_t)0);

    {
        int16_t p1[3] = {1, 2, 3}, p2[3] = {4, 5, 6};
        ASSERT_EQ(islet_get_3(db, p1), (uint32_t)ISLET_MISS);
        ASSERT_EQ(islet_get_3(db, p2), (uint32_t)ISLET_MISS);
    }
}

TEST(unstore_idempotent_absent) {
    setup_once();
    void *ctx = rec_axis_open(":test_unstore_idem:1023");
    ASSERT_NOT_NULL(ctx);

    ASSERT_EQ(rec_axis_unstore(ctx, 424242), 0);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 77, "3,3"), 0);
    ASSERT_EQ(rec_axis_unstore(ctx, 77), 0);
    ASSERT_EQ(rec_axis_unstore(ctx, 77), 0);
}

TEST(readback_absent) {
    setup_once();
    void *ctx = rec_axis_open(":test_readback_absent:1023");
    ASSERT_NOT_NULL(ctx);

    char *blob = (char *)0x1;
    size_t n = 999;
    ASSERT_EQ(rec_axis_readback(ctx, 31337, &blob, &n), 0);
    ASSERT_NULL(blob);
    ASSERT_EQ(n, (size_t)0);
}

TEST(replace_in_place) {
    setup_once();
    void *ctx = rec_axis_open(":test_replace:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;

    ASSERT_EQ(rec_axis_store(ctx, NULL, 5, "1,1"), 0);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 5, "9,9"), 0);

    char *blob = NULL;
    size_t n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 5, &blob, &n), 0);
    ASSERT_NOT_NULL(blob);
    ASSERT_EQ(blob_entries(blob, n), (size_t)1);
    ASSERT_STREQ(blob, "9,9");
    free(blob);

    /* The old cell is gone; the box holds the ref exactly once. */
    {
        int16_t oldp[2] = {1, 1}, newp[2] = {9, 9};
        ASSERT_EQ(islet_get_2(db, oldp), (uint32_t)ISLET_MISS);
        ASSERT_EQ(islet_get_2(db, newp), (uint32_t)5);

        rec_set_t *out = rec_set_new();
        int16_t s[2] = {0, 0};
        uint16_t l[2] = {16, 16};
        ASSERT_EQ(rec_axis_fill_bbox_2(db, s, l, out), 0);
        ASSERT_EQ(rec_set_count(out), (size_t)1);
        rec_set_free(out);
    }

    /* Same value re-stated: idempotent, still one entry. */
    ASSERT_EQ(rec_axis_store(ctx, NULL, 5, "9,9"), 0);
    blob = NULL; n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 5, &blob, &n), 0);
    ASSERT_EQ(blob_entries(blob, n), (size_t)1);
    free(blob);
}

TEST(per_ref_isolation) {
    setup_once();
    void *ctx = rec_axis_open(":test_ref_isolation:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;

    ASSERT_EQ(rec_axis_store(ctx, NULL, 21, "0,0"), 0);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 22, "50,50"), 0);
    ASSERT_EQ(rec_axis_unstore(ctx, 21), 0);

    char *blob = NULL;
    size_t n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 21, &blob, &n), 0);
    ASSERT_NULL(blob);
    ASSERT_EQ(rec_axis_readback(ctx, 22, &blob, &n), 0);
    ASSERT_NOT_NULL(blob);
    ASSERT_STREQ(blob, "50,50");
    free(blob);

    {
        int16_t p1[2] = {0, 0}, p2[2] = {50, 50};
        ASSERT_EQ(islet_get_2(db, p1), (uint32_t)ISLET_MISS);
        ASSERT_EQ(islet_get_2(db, p2), (uint32_t)22);
    }
}

TEST(shared_cell) {
    setup_once();
    void *ctx = rec_axis_open(":test_shared_cell:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;
    int16_t p[3] = {7, 7, 7};

    ASSERT_EQ(rec_axis_store(ctx, NULL, 31, "7,7,7"), 0);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 32, "7,7,7"), 0);
    ASSERT_EQ(islet_cell_count_3(db, p), (uint32_t)2);

    /* Unstoring one ref removes only its own entry from the shared cell. */
    ASSERT_EQ(rec_axis_unstore(ctx, 31), 0);

    char *blob = NULL;
    size_t n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 31, &blob, &n), 0);
    ASSERT_NULL(blob);
    ASSERT_EQ(rec_axis_readback(ctx, 32, &blob, &n), 0);
    ASSERT_NOT_NULL(blob);
    ASSERT_STREQ(blob, "7,7,7");
    free(blob);

    ASSERT_EQ(islet_cell_count_3(db, p), (uint32_t)1);
    ASSERT_EQ(islet_get_3(db, p), (uint32_t)32);
}

TEST(dedup_within_call) {
    setup_once();
    void *ctx = rec_axis_open(":test_dedup:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;
    int16_t p[2] = {5, 6};

    ASSERT_EQ(rec_axis_store(ctx, NULL, 9, "5,6;5,6;5,6"), 0);

    char *blob = NULL;
    size_t n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 9, &blob, &n), 0);
    ASSERT_EQ(blob_entries(blob, n), (size_t)1);
    ASSERT_STREQ(blob, "5,6");
    free(blob);

    ASSERT_EQ(islet_cell_count_2(db, p), (uint32_t)1);
}

TEST(dims_one_two_four) {
    setup_once();
    void *ctx = rec_axis_open(":test_dims:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;
    char *blob = NULL;
    size_t n = 0;

    ASSERT_EQ(rec_axis_store(ctx, NULL, 101, "9"), 0);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 102, "-3,4"), 0);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 104, "1,2,3,4"), 0);

    ASSERT_EQ(rec_axis_readback(ctx, 101, &blob, &n), 0);
    ASSERT_STREQ(blob, "9");
    free(blob); blob = NULL;
    ASSERT_EQ(rec_axis_readback(ctx, 102, &blob, &n), 0);
    ASSERT_STREQ(blob, "-3,4");
    free(blob); blob = NULL;
    ASSERT_EQ(rec_axis_readback(ctx, 104, &blob, &n), 0);
    ASSERT_STREQ(blob, "1,2,3,4");
    free(blob); blob = NULL;

    {
        int16_t p1[1] = {9}, p2[2] = {-3, 4}, p4[4] = {1, 2, 3, 4};
        ASSERT_EQ(islet_get_1(db, p1), (uint32_t)101);
        ASSERT_EQ(islet_get_2(db, p2), (uint32_t)102);
        ASSERT_EQ(islet_get_4(db, p4), (uint32_t)104);
    }
}

TEST(einval_matrix) {
    setup_once();
    void *ctx = rec_axis_open(":test_einval:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;
    static const char *bad[] = {
        "abc", "1,", "1,,2", ";1,2", "1,2;", "1,2;;3,4",
        "1,2,3,4,5", "1,2;3", "99999,0", "0,-99999",
        "1.5,2", " 1,2", "1, 2", "1 2", "1,2,", ",1,2",
        "1;2,3", "0x10,2", "+", "-,2", "5;", ";5",
    };
    size_t i;

    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        errno = 0;
        ASSERT_EQ(rec_axis_store(ctx, NULL, 600 + (uint32_t)i,
                    bad[i]), -1);
        ASSERT_EQ(errno, EINVAL);
    }

    /* Rejected stores mutated nothing: no rev entries, cells empty. */
    {
        char *blob = NULL;
        size_t n = 0;
        ASSERT_EQ(rec_axis_readback(ctx, 600, &blob, &n), 0);
        ASSERT_NULL(blob);
        int16_t p[2] = {1, 2};
        ASSERT_EQ(islet_cell_count_2(db, p), (uint32_t)0);
    }
}

TEST(null_and_ref_contracts) {
    setup_once();
    void *ctx = rec_axis_open(":test_contracts:1023");
    ASSERT_NOT_NULL(ctx);
    char *blob = NULL;
    size_t n = 0;

    errno = 0;
    ASSERT_EQ(rec_axis_store(NULL, NULL, 1, "1,2"), -1);
    ASSERT_EQ(errno, EINVAL);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 1, NULL), -1);
    ASSERT_EQ(rec_axis_store(ctx, NULL, 1, ""), -1);
    ASSERT_EQ(rec_axis_store(ctx, NULL, UINT32_MAX, "1,2"), -1);
    ASSERT_EQ(rec_axis_unstore(NULL, 1), -1);
    ASSERT_EQ(rec_axis_readback(NULL, 1, &blob, &n), -1);
    ASSERT_EQ(rec_axis_readback(ctx, 1, NULL, &n), -1);
    ASSERT_EQ(rec_axis_readback(ctx, 1, &blob, NULL), -1);
}

TEST(overcap_erange) {
    setup_once();
    void *ctx = rec_axis_open(":test_overcap:1023");
    ASSERT_NOT_NULL(ctx);
    /* 1025 dim-1 points: one over ISLET_AXIS_MAX_POINTS. */
    char *big = malloc(6 * 1025 + 1);
    char *w = big;
    int i;
    ASSERT_NOT_NULL(big);
    for (i = 0; i <= 1024; i++)
        w += sprintf(w, "%s%d", i ? ";" : "", i);
    errno = 0;
    ASSERT_EQ(rec_axis_store(ctx, NULL, 700, big), -1);
    ASSERT_EQ(errno, ERANGE);
    free(big);

    /* Exactly at the cap: accepted. */
    {
        char *ok = malloc(6 * 1024 + 1);
        char *x = ok;
        char *blob = NULL;
        size_t n = 0;
        ASSERT_NOT_NULL(ok);
        for (i = 0; i < 1024; i++)
            x += sprintf(x, "%s%d", i ? ";" : "", i);
        ASSERT_EQ(rec_axis_store(ctx, NULL, 701, ok), 0);
        free(ok);
        ASSERT_EQ(rec_axis_readback(ctx, 701, &blob, &n), 0);
        ASSERT_EQ(blob_entries(blob, n), (size_t)1024);
        free(blob);
        ASSERT_EQ(rec_axis_unstore(ctx, 701), 0);
    }
}

TEST(del_value_native) {
    setup_once();
    uint32_t db = islet_open(NULL, "test_del_value", 1023);
    int16_t p[3] = {10, 20, 30};
    int16_t empty[3] = {0, 0, 0};

    islet_put_3(db, p, 7);
    islet_put_3(db, p, 9);
    islet_put_3(db, p, 8);
    ASSERT_EQ(islet_cell_count_3(db, p), (uint32_t)3);

    /* Remove the middle value: survivors keep relative order. */
    ASSERT_EQ(islet_del_value_3(db, p, 9), (uint32_t)1);
    ASSERT_EQ(islet_cell_count_3(db, p), (uint32_t)2);
    ASSERT_EQ(islet_get_3(db, p), (uint32_t)7);
    {
        uint32_t cur = islet_get_multi_3(db, p);
        uint32_t v;
        ASSERT(cur != CM_MISS);
        ASSERT(islet_cell_next(&v, cur));
        ASSERT_EQ(v, (uint32_t)7);
        ASSERT(islet_cell_next(&v, cur));
        ASSERT_EQ(v, (uint32_t)8);
        ASSERT(!islet_cell_next(&v, cur));
    }

    ASSERT_EQ(islet_del_value_3(db, p, 42), (uint32_t)0);
    ASSERT_EQ(islet_cell_count_3(db, p), (uint32_t)2);
    ASSERT_EQ(islet_del_value_3(db, empty, 7), (uint32_t)0);
    ASSERT_EQ(islet_del_value_2_32(db, (int32_t *)empty, 7),
            (uint32_t)0);
}

TEST(file_backed_sidecar) {
    setup_once();
    const char *grid = "/tmp/test_islet_axis_store.db";
    char ridx[256];
    struct stat st;

    snprintf(ridx, sizeof(ridx), "%s.ridx", grid);
    unlink(grid);
    unlink(ridx);

    {
        char spec[256];
        void *ctx;
        int16_t p[2] = {12, 34};
        char *blob = NULL;
        size_t n = 0;

        snprintf(spec, sizeof(spec), "%s::1023", grid);
        ctx = rec_axis_open(spec);
        ASSERT_NOT_NULL(ctx);

        ASSERT_EQ(rec_axis_store(ctx, NULL, 55, "12,34"), 0);
        corm_save();

        /* Both the grid and the rev sidecar hit the disk. */
        ASSERT(stat(grid, &st) == 0);
        ASSERT(st.st_size > 0);
        ASSERT(stat(ridx, &st) == 0);
        ASSERT(st.st_size > 0);

        ASSERT_EQ(rec_axis_readback(ctx, 55, &blob, &n), 0);
        ASSERT_NOT_NULL(blob);
        ASSERT_STREQ(blob, "12,34");
        free(blob);
        ASSERT_EQ(islet_get_2((uint32_t)(uintptr_t)ctx, p),
                (uint32_t)55);
    }

    unlink(grid);
    unlink(ridx);
}

TEST(readback_roundtrip_resubmit) {
    setup_once();
    void *ctx = rec_axis_open(":test_resubmit:1023");
    ASSERT_NOT_NULL(ctx);
    uint32_t db = (uint32_t)(uintptr_t)ctx;
    int16_t p1[2] = {8, 8}, p2[2] = {9, 9};

    ASSERT_EQ(rec_axis_store(ctx, NULL, 88, "8,8;9,9"), 0);

    char *blob = NULL, *again = NULL;
    size_t n = 0, m = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 88, &blob, &n), 0);
    ASSERT_NOT_NULL(blob);

    /* Feed the read-back entries straight back through store:
     * replace-in-place must converge to the identical blob, with each
     * cell still holding exactly one copy. */
    {
        char *work = malloc(n + 1);
        size_t i, e = blob_entries(blob, n);
        ASSERT_NOT_NULL(work);
        memcpy(work, blob, n);
        work[n] = '\0';
        for (i = 0; i + 1 < n; i++)
            if (!work[i])
                work[i] = ';';
        ASSERT_EQ(rec_axis_store(ctx, NULL, 88, work), 0);
        free(work);
        ASSERT_EQ(rec_axis_readback(ctx, 88, &again, &m), 0);
        ASSERT_EQ(m, n);
        ASSERT(memcmp(again, blob, n) == 0);
        ASSERT_EQ(e, (size_t)2);
        free(again);
        free(blob);
        ASSERT_EQ(islet_cell_count_2(db, p1), (uint32_t)1);
        ASSERT_EQ(islet_cell_count_2(db, p2), (uint32_t)1);
    }
}

TEST(raw_handle_lazy_rev) {
    setup_once();
    /* A grid opened outside rec_axis_open still serves stores through a
     * lazy in-memory rev (persistence-incomplete by design). */
    uint32_t db = islet_open(NULL, "test_axis_store_lazy", 1023);
    void *ctx = (void *)(uintptr_t)db;

    ASSERT_EQ(rec_axis_store(ctx, NULL, 3, "2,2"), 0);
    char *blob = NULL;
    size_t n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 3, &blob, &n), 0);
    ASSERT_NOT_NULL(blob);
    ASSERT_STREQ(blob, "2,2");
    free(blob);
    ASSERT_EQ(rec_axis_unstore(ctx, 3), 0);
    blob = NULL; n = 0;
    ASSERT_EQ(rec_axis_readback(ctx, 3, &blob, &n), 0);
    ASSERT_NULL(blob);
}

int main(void) {
    test_suite_begin("Islet Phase-2A Store/Unstore/Readback Tests");
    RUN_TEST(store_readback_roundtrip);
    RUN_TEST(unstore_idempotent_absent);
    RUN_TEST(readback_absent);
    RUN_TEST(replace_in_place);
    RUN_TEST(per_ref_isolation);
    RUN_TEST(shared_cell);
    RUN_TEST(dedup_within_call);
    RUN_TEST(dims_one_two_four);
    RUN_TEST(einval_matrix);
    RUN_TEST(null_and_ref_contracts);
    RUN_TEST(overcap_erange);
    RUN_TEST(del_value_native);
    RUN_TEST(file_backed_sidecar);
    RUN_TEST(readback_roundtrip_resubmit);
    RUN_TEST(raw_handle_lazy_rev);
    return test_suite_end();
}
