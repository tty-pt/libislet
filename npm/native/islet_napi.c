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

/* Spatial DB Handle */
typedef struct {
    uint32_t hd;
    bool is_open;
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
        free(wrap);
    }
}

static napi_value IsletOpen(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    char path_buf[256] = {0};
    char db_buf[64] = "islet";
    if (argc >= 1) {
        size_t len = 0;
        napi_get_value_string_utf8(env, args[0], path_buf, sizeof(path_buf) - 1, &len);
    }
    if (argc >= 2) {
        size_t len = 0;
        napi_get_value_string_utf8(env, args[1], db_buf, sizeof(db_buf) - 1, &len);
    }

    islet_init();
    uint32_t hd = islet_open(path_buf[0] ? path_buf : NULL, db_buf, 0xFF);
    if (hd == ISLET_MISS || hd == 0) {
        napi_throw_error(env, NULL, "Failed to open islet database");
        return NULL;
    }

    IsletDbWrapper *wrap = (IsletDbWrapper *)calloc(1, sizeof(IsletDbWrapper));
    wrap->hd = hd;
    wrap->is_open = true;

    napi_value res;
    napi_create_object(env, &res);
    napi_wrap(env, res, wrap, IsletDbFinalizer, NULL, NULL);
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

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"encode2D", NULL, IsletEncode2D, NULL, NULL, NULL, napi_default, NULL},
        {"decode2D", NULL, IsletDecode2D, NULL, NULL, NULL, napi_default, NULL},
        {"open", NULL, IsletOpen, NULL, NULL, NULL, napi_default, NULL},
        {"put2D", NULL, IsletPut2D, NULL, NULL, NULL, napi_default, NULL},
        {"get2D", NULL, IsletGet2D, NULL, NULL, NULL, napi_default, NULL}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
