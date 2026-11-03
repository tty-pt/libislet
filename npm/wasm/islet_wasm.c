#include <stddef.h>
#include <stdint.h>
#include <ttypt/morton.h>
#include <ttypt/iso.h>

ISO_EXPORT(uint64_t, islet_wasm_encode2d, int32_t x, int32_t y) {
    int16_t p[2] = { (int16_t)x, (int16_t)y };
    return morton_set_2(p);
}

ISO_EXPORT(void, islet_wasm_decode2d, uint64_t code, int32_t *out) {
    int16_t p[2] = {0, 0};
    morton_get_2(p, code);
    out[0] = (int32_t)p[0];
    out[1] = (int32_t)p[1];
}

ISO_EXPORT(uint64_t, islet_wasm_encode3d, int32_t x, int32_t y, int32_t z) {
    int16_t p[3] = { (int16_t)x, (int16_t)y, (int16_t)z };
    return morton_set_3(p);
}

ISO_EXPORT(void, islet_wasm_decode3d, uint64_t code, int32_t *out) {
    int16_t p[3] = {0, 0, 0};
    morton_get_3(p, code);
    out[0] = (int32_t)p[0];
    out[1] = (int32_t)p[1];
    out[2] = (int32_t)p[2];
}
