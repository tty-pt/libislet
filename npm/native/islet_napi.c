#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <ttypt/corm.h>
#include <ttypt/islet.h>
#include <ttypt/morton.h>

/* Islet Morton 2D encode */
static napi_value IsletEncode2D(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected (x, y)");
        return NULL;
    }

    int32_t x = 0, y = 0;
    napi_get_value_int32(env, args[0], &x);
    napi_get_value_int32(env, args[1], &y);

    int16_t p[2] = { (int16_t)x, (int16_t)y };
    uint64_t code = morton_set_2(p);

    napi_value res;
    napi_create_bigint_uint64(env, code, &res);
    return res;
}

/* Islet Morton 2D decode */
static napi_value IsletDecode2D(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected (code)");
        return NULL;
    }

    uint64_t code = 0;
    bool lossless = false;
    napi_get_value_bigint_uint64(env, args[0], &code, &lossless);

    int16_t pos[2] = {0, 0};
    morton_get_2(pos, code);

    napi_value res;
    napi_create_object(env, &res);

    napi_value x_val, y_val;
    napi_create_int32(env, (int32_t)pos[0], &x_val);
    napi_create_int32(env, (int32_t)pos[1], &y_val);

    napi_set_named_property(env, res, "x", x_val);
    napi_set_named_property(env, res, "y", y_val);
    return res;
}

/* Islet Morton 3D encode */
static napi_value IsletEncode3D(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Expected (x, y, z)");
        return NULL;
    }

    int32_t x = 0, y = 0, z = 0;
    napi_get_value_int32(env, args[0], &x);
    napi_get_value_int32(env, args[1], &y);
    napi_get_value_int32(env, args[2], &z);

    int16_t p[3] = { (int16_t)x, (int16_t)y, (int16_t)z };
    uint64_t code = morton_set_3(p);

    napi_value res;
    napi_create_bigint_uint64(env, code, &res);
    return res;
}

/* Islet Morton 3D decode */
static napi_value IsletDecode3D(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected (code)");
        return NULL;
    }

    uint64_t code = 0;
    bool lossless = false;
    napi_get_value_bigint_uint64(env, args[0], &code, &lossless);

    int16_t pos[3] = {0, 0, 0};
    morton_get_3(pos, code);

    napi_value res;
    napi_create_object(env, &res);

    napi_value x_val, y_val, z_val;
    napi_create_int32(env, (int32_t)pos[0], &x_val);
    napi_create_int32(env, (int32_t)pos[1], &y_val);
    napi_create_int32(env, (int32_t)pos[2], &z_val);

    napi_set_named_property(env, res, "x", x_val);
    napi_set_named_property(env, res, "y", y_val);
    napi_set_named_property(env, res, "z", z_val);
    return res;
}

/* Spatial DB Handle */
typedef struct {
    uint32_t hd;
    bool is_open;
    char *path_copy;
    char *db_copy;
} IsletDbWrapper;

static void IsletDbFinalizer(napi_env env, void *finalize_data, void *finalize_hint) {
    (void)env;
    (void)finalize_hint;
    IsletDbWrapper *wrap = (IsletDbWrapper *)finalize_data;
    if (wrap) {
        if (wrap->is_open && wrap->hd != 0 && wrap->hd != ISLET_MISS) {
            corm_close(wrap->hd);
            wrap->is_open = false;
        }
        free(wrap->path_copy);
        free(wrap->db_copy);
        free(wrap);
    }
}

static napi_value IsletOpen(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    char path_buf[512] = {0};
    char db_buf[128] = "islet";
    if (argc >= 1) {
        size_t len = 0;
        napi_get_value_string_utf8(env, args[0], path_buf, sizeof(path_buf) - 1, &len);
    }
    if (argc >= 2) {
        size_t len = 0;
        napi_get_value_string_utf8(env, args[1], db_buf, sizeof(db_buf) - 1, &len);
    }

    IsletDbWrapper *wrap = (IsletDbWrapper *)calloc(1, sizeof(IsletDbWrapper));
    if (!wrap) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    if (path_buf[0]) {
        wrap->path_copy = strdup(path_buf);
    }
    wrap->db_copy = strdup(db_buf[0] ? db_buf : "islet");

    islet_init();
    uint32_t hd = islet_open(wrap->path_copy, wrap->db_copy, 0xFF);
    if (hd == ISLET_MISS) {
        free(wrap->path_copy);
        free(wrap->db_copy);
        free(wrap);
        napi_throw_error(env, NULL, "Failed to open islet database");
        return NULL;
    }

    wrap->hd = hd;
    wrap->is_open = true;

    napi_value res;
    napi_create_object(env, &res);
    napi_wrap(env, res, wrap, IsletDbFinalizer, NULL, NULL);
    return res;
}

static napi_value IsletClose(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected (db)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (wrap && wrap->is_open) {
        if (wrap->path_copy) {
            corm_save();
        }
        corm_close(wrap->hd);
        wrap->is_open = false;
    }

    napi_value res;
    napi_get_boolean(env, true, &res);
    return res;
}

static napi_value IsletPut2D(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value args[4];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 4) {
        napi_throw_type_error(env, NULL, "Expected (db, x, y, ref)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    int32_t x = 0, y = 0;
    uint32_t ref = 0;
    napi_get_value_int32(env, args[1], &x);
    napi_get_value_int32(env, args[2], &y);
    napi_get_value_uint32(env, args[3], &ref);

    int16_t p[2] = { (int16_t)x, (int16_t)y };
    islet_put_2(wrap->hd, p, ref);

    napi_value res;
    napi_get_boolean(env, true, &res);
    return res;
}

static napi_value IsletGet2D(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Expected (db, x, y)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    int32_t x = 0, y = 0;
    napi_get_value_int32(env, args[1], &x);
    napi_get_value_int32(env, args[2], &y);

    int16_t p[2] = { (int16_t)x, (int16_t)y };
    uint32_t val = islet_get_2(wrap->hd, p);

    if (val == ISLET_MISS) {
        napi_value null_v;
        napi_get_null(env, &null_v);
        return null_v;
    }

    napi_value res;
    napi_create_uint32(env, val, &res);
    return res;
}

static napi_value IsletDel2D(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value args[4];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Expected (db, x, y[, ref])");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    int32_t x = 0, y = 0;
    napi_get_value_int32(env, args[1], &x);
    napi_get_value_int32(env, args[2], &y);
    int16_t p[2] = { (int16_t)x, (int16_t)y };

    if (argc >= 4) {
        napi_valuetype vtype;
        napi_typeof(env, args[3], &vtype);
        if (vtype == napi_number) {
            uint32_t ref = 0;
            napi_get_value_uint32(env, args[3], &ref);
            uint32_t removed = islet_del_value_2(wrap->hd, p, ref);
            napi_value res;
            napi_get_boolean(env, removed > 0, &res);
            return res;
        }
    }

    islet_del_2(wrap->hd, p);
    napi_value res;
    napi_get_boolean(env, true, &res);
    return res;
}

static napi_value IsletSearchBox2D(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value args[5];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 5) {
        napi_throw_type_error(env, NULL, "Expected (db, minX, minY, maxX, maxY)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    int32_t min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    napi_get_value_int32(env, args[1], &min_x);
    napi_get_value_int32(env, args[2], &min_y);
    napi_get_value_int32(env, args[3], &max_x);
    napi_get_value_int32(env, args[4], &max_y);

    if (max_x < min_x) { int32_t t = min_x; min_x = max_x; max_x = t; }
    if (max_y < min_y) { int32_t t = min_y; min_y = max_y; max_y = t; }

    int16_t s[2] = { (int16_t)min_x, (int16_t)min_y };
    uint16_t l[2] = { (uint16_t)(max_x - min_x), (uint16_t)(max_y - min_y) };

    uint32_t cur = islet_iter_2(wrap->hd, s, l);

    napi_value arr;
    napi_create_array(env, &arr);

    int16_t pt[2] = {0, 0};
    uint32_t ref = 0;
    uint32_t idx = 0;
    while (islet_next(pt, &ref, cur)) {
        napi_value item, vx, vy, vref;
        napi_create_object(env, &item);
        napi_create_int32(env, (int32_t)pt[0], &vx);
        napi_create_int32(env, (int32_t)pt[1], &vy);
        napi_create_uint32(env, ref, &vref);
        napi_set_named_property(env, item, "x", vx);
        napi_set_named_property(env, item, "y", vy);
        napi_set_named_property(env, item, "ref", vref);
        napi_set_element(env, arr, idx++, item);
    }

    return arr;
}

static napi_value IsletFillBox2D(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value args[5];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 5) {
        napi_throw_type_error(env, NULL, "Expected (db, minX, minY, maxX, maxY)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    int32_t min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    napi_get_value_int32(env, args[1], &min_x);
    napi_get_value_int32(env, args[2], &min_y);
    napi_get_value_int32(env, args[3], &max_x);
    napi_get_value_int32(env, args[4], &max_y);

    if (max_x < min_x) { int32_t t = min_x; min_x = max_x; max_x = t; }
    if (max_y < min_y) { int32_t t = min_y; min_y = max_y; max_y = t; }

    int16_t s[2] = { (int16_t)min_x, (int16_t)min_y };
    uint16_t l[2] = { (uint16_t)(max_x - min_x), (uint16_t)(max_y - min_y) };

    rec_set_t *cands = rec_set_new();
    if (!cands) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }

    if (rec_axis_fill_bbox_2(wrap->hd, s, l, cands) != 0) {
        /* Fallback to islet_iter_2 if box volume > ISLET_FILL_MAX_VOL */
        uint32_t cur = islet_iter_2(wrap->hd, s, l);
        int16_t pt[2];
        uint32_t ref;
        while (islet_next(pt, &ref, cur)) {
            rec_set_push(cands, (rec_ref_t)ref);
        }
        rec_set_seal(cands);
    }

    size_t count = rec_set_count(cands);
    const rec_ref_t *refs = rec_set_at(cands);

    napi_value arr;
    napi_create_array_with_length(env, count, &arr);
    for (size_t i = 0; i < count; i++) {
        napi_value vref;
        napi_create_uint32(env, (uint32_t)refs[i], &vref);
        napi_set_element(env, arr, (uint32_t)i, vref);
    }

    rec_set_free(cands);
    return arr;
}

static napi_value IsletAxisStore(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Expected (db, ref, pointListSpec)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    uint32_t ref = 0;
    napi_get_value_uint32(env, args[1], &ref);

    size_t len = 0;
    napi_get_value_string_utf8(env, args[2], NULL, 0, &len);
    char *val = (char *)malloc(len + 1);
    if (!val) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    napi_get_value_string_utf8(env, args[2], val, len + 1, &len);

    int rc = rec_axis_store((void *)(uintptr_t)wrap->hd, NULL, (rec_ref_t)ref, val);
    free(val);

    napi_value res;
    napi_get_boolean(env, rc == 0, &res);
    return res;
}

static napi_value IsletAxisUnstore(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected (db, ref)");
        return NULL;
    }

    IsletDbWrapper *wrap = NULL;
    napi_unwrap(env, args[0], (void **)&wrap);
    if (!wrap || !wrap->is_open) {
        napi_throw_error(env, NULL, "Islet DB closed or invalid");
        return NULL;
    }

    uint32_t ref = 0;
    napi_get_value_uint32(env, args[1], &ref);

    int rc = rec_axis_unstore((void *)(uintptr_t)wrap->hd, (rec_ref_t)ref);
    napi_value res;
    napi_get_boolean(env, rc == 0, &res);
    return res;
}

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"encode2D", NULL, IsletEncode2D, NULL, NULL, NULL, napi_default, NULL},
        {"decode2D", NULL, IsletDecode2D, NULL, NULL, NULL, napi_default, NULL},
        {"encode3D", NULL, IsletEncode3D, NULL, NULL, NULL, napi_default, NULL},
        {"decode3D", NULL, IsletDecode3D, NULL, NULL, NULL, napi_default, NULL},
        {"open", NULL, IsletOpen, NULL, NULL, NULL, napi_default, NULL},
        {"close", NULL, IsletClose, NULL, NULL, NULL, napi_default, NULL},
        {"put2D", NULL, IsletPut2D, NULL, NULL, NULL, napi_default, NULL},
        {"get2D", NULL, IsletGet2D, NULL, NULL, NULL, napi_default, NULL},
        {"del2D", NULL, IsletDel2D, NULL, NULL, NULL, napi_default, NULL},
        {"searchBox", NULL, IsletSearchBox2D, NULL, NULL, NULL, napi_default, NULL},
        {"fillBox", NULL, IsletFillBox2D, NULL, NULL, NULL, napi_default, NULL},
        {"axisStore", NULL, IsletAxisStore, NULL, NULL, NULL, napi_default, NULL},
        {"axisUnstore", NULL, IsletAxisUnstore, NULL, NULL, NULL, napi_default, NULL}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
