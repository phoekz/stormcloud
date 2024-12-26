// Includes.
#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

//
// Stormcloud - Utilities.
//

#define SC_UNUSED(x) (void)(x)
#define SC_ASSERT(expr) \
    if (!(expr)) {      \
        abort();        \
    }
#define SC_LOG_INFO(fmt, ...) SDL_Log(fmt, ##__VA_ARGS__)
#define SC_LOG_ERROR(fmt, ...) SDL_LogError(SDL_LOG_CATEGORY_ERROR, fmt, ##__VA_ARGS__)
#define SC_SDL_ASSERT(expr)                            \
    if (!(expr)) {                                     \
        SC_LOG_ERROR("SDL error: %s", SDL_GetError()); \
        abort();                                       \
    }

//
// Stormcloud - Math.
//

#define SC_PI SDL_PI_F

float sc_rad_from_deg(float deg) {
    return deg * (SC_PI / 180.0f);
}

typedef struct ScVector3 {
    float x, y, z;
} ScVector3;

typedef struct ScMatrix4x4 {
    float m11, m12, m13, m14;
    float m21, m22, m23, m24;
    float m31, m32, m33, m34;
    float m41, m42, m43, m44;
} ScMatrix4x4;

typedef struct ScBounds3 {
    ScVector3 min;
    ScVector3 max;
} ScBounds3;

ScVector3 sc_vector3_sub(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

ScVector3 sc_vector3_normalize(ScVector3 vec) {
    const float len = SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
    return (ScVector3) {
        vec.x / len,
        vec.y / len,
        vec.z / len,
    };
}

float sc_vector3_dot(ScVector3 lhs, ScVector3 rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

ScVector3 sc_vector3_cross(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.y * rhs.z - rhs.y * lhs.z,
        -(lhs.x * rhs.z - rhs.x * lhs.z),
        lhs.x * rhs.y - rhs.x * lhs.y,
    };
}

ScMatrix4x4 sc_matrix4x4_identity() {
    // clang-format off
    return (ScMatrix4x4) {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    // clang-format on
}

ScMatrix4x4 sc_matrix4x4_multiply(ScMatrix4x4 lhs, ScMatrix4x4 rhs) {
    ScMatrix4x4 m;
    m.m11 = (lhs.m11 * rhs.m11) + (lhs.m12 * rhs.m21) + (lhs.m13 * rhs.m31) + (lhs.m14 * rhs.m41);
    m.m12 = (lhs.m11 * rhs.m12) + (lhs.m12 * rhs.m22) + (lhs.m13 * rhs.m32) + (lhs.m14 * rhs.m42);
    m.m13 = (lhs.m11 * rhs.m13) + (lhs.m12 * rhs.m23) + (lhs.m13 * rhs.m33) + (lhs.m14 * rhs.m43);
    m.m14 = (lhs.m11 * rhs.m14) + (lhs.m12 * rhs.m24) + (lhs.m13 * rhs.m34) + (lhs.m14 * rhs.m44);
    m.m21 = (lhs.m21 * rhs.m11) + (lhs.m22 * rhs.m21) + (lhs.m23 * rhs.m31) + (lhs.m24 * rhs.m41);
    m.m22 = (lhs.m21 * rhs.m12) + (lhs.m22 * rhs.m22) + (lhs.m23 * rhs.m32) + (lhs.m24 * rhs.m42);
    m.m23 = (lhs.m21 * rhs.m13) + (lhs.m22 * rhs.m23) + (lhs.m23 * rhs.m33) + (lhs.m24 * rhs.m43);
    m.m24 = (lhs.m21 * rhs.m14) + (lhs.m22 * rhs.m24) + (lhs.m23 * rhs.m34) + (lhs.m24 * rhs.m44);
    m.m31 = (lhs.m31 * rhs.m11) + (lhs.m32 * rhs.m21) + (lhs.m33 * rhs.m31) + (lhs.m34 * rhs.m41);
    m.m32 = (lhs.m31 * rhs.m12) + (lhs.m32 * rhs.m22) + (lhs.m33 * rhs.m32) + (lhs.m34 * rhs.m42);
    m.m33 = (lhs.m31 * rhs.m13) + (lhs.m32 * rhs.m23) + (lhs.m33 * rhs.m33) + (lhs.m34 * rhs.m43);
    m.m34 = (lhs.m31 * rhs.m14) + (lhs.m32 * rhs.m24) + (lhs.m33 * rhs.m34) + (lhs.m34 * rhs.m44);
    m.m41 = (lhs.m41 * rhs.m11) + (lhs.m42 * rhs.m21) + (lhs.m43 * rhs.m31) + (lhs.m44 * rhs.m41);
    m.m42 = (lhs.m41 * rhs.m12) + (lhs.m42 * rhs.m22) + (lhs.m43 * rhs.m32) + (lhs.m44 * rhs.m42);
    m.m43 = (lhs.m41 * rhs.m13) + (lhs.m42 * rhs.m23) + (lhs.m43 * rhs.m33) + (lhs.m44 * rhs.m43);
    m.m44 = (lhs.m41 * rhs.m14) + (lhs.m42 * rhs.m24) + (lhs.m43 * rhs.m34) + (lhs.m44 * rhs.m44);
    return m;
}

ScMatrix4x4 sc_matrix4x4_perspective(float fov, float aspect, float znear, float zfar) {
    const float num = 1.0f / ((float)SDL_tanf(fov * 0.5f));
    // clang-format off
    return (ScMatrix4x4) {
        num / aspect, 0, 0, 0,
        0, num, 0, 0,
        0, 0, zfar / (znear - zfar), -1,
        0, 0, (znear * zfar) / (znear - zfar), 0,
    };
    // clang-format on
}

ScMatrix4x4 sc_matrix4x4_look_at(ScVector3 origin, ScVector3 target, ScVector3 up) {
    const ScVector3 target_to_origin = {
        origin.x - target.x,
        origin.y - target.y,
        origin.z - target.z,
    };
    const ScVector3 vec_a = sc_vector3_normalize(target_to_origin);
    const ScVector3 vec_b = sc_vector3_normalize(sc_vector3_cross(up, vec_a));
    const ScVector3 vec_c = sc_vector3_cross(vec_a, vec_b);
    // clang-format off
    return (ScMatrix4x4) {
        vec_b.x, vec_c.x, vec_a.x, 0,
        vec_b.y, vec_c.y, vec_a.y, 0,
        vec_b.z, vec_c.z, vec_a.z, 0,
        -sc_vector3_dot(vec_b, origin), -sc_vector3_dot(vec_c, origin), -sc_vector3_dot(vec_a, origin), 1,
    };
    // clang-format on
}

ScBounds3 sc_bounds3_new() {
    return (ScBounds3) {
        .min = {FLT_MAX, FLT_MAX, FLT_MAX},
        .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX},
    };
}

ScVector3 sc_bounds3_center(ScBounds3 bounds) {
    return (ScVector3) {
        (bounds.min.x + bounds.max.x) * 0.5f,
        (bounds.min.y + bounds.max.y) * 0.5f,
        (bounds.min.z + bounds.max.z) * 0.5f,
    };
}

ScBounds3 sc_bounds3_extend(ScBounds3 bounds, ScVector3 point) {
    bounds.min.x = SDL_min(bounds.min.x, point.x);
    bounds.min.y = SDL_min(bounds.min.y, point.y);
    bounds.min.z = SDL_min(bounds.min.z, point.z);
    bounds.max.x = SDL_max(bounds.max.x, point.x);
    bounds.max.y = SDL_max(bounds.max.y, point.y);
    bounds.max.z = SDL_max(bounds.max.z, point.z);
    return bounds;
}

//
// Stormcloud - Data.
//

typedef struct ScColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} ScColor;

typedef struct ScPoint {
    ScVector3 position;
    ScColor color;
} ScPoint;

typedef struct ScData {
    ScBounds3 bounds;
    uint32_t point_count;
    const ScPoint* points;
} ScData;

static void sc_data_load(ScData* sc_data, const char* pak_path) {
    // Timing.
    const uint64_t begin_time_ns = SDL_GetTicksNS();

    // Declarations.
    struct Header {
        uint32_t point_count;
        float min_x;
        float min_y;
        float min_z;
        float max_x;
        float max_y;
        float max_z;
        uint32_t encoded_size_xs;
        uint32_t encoded_size_ys;
        uint32_t encoded_size_zs;
        uint32_t encoded_size_rs;
        uint32_t encoded_size_gs;
        uint32_t encoded_size_bs;
    };

    // Load header.
    FILE* file = fopen(pak_path, "rb");
    char magic[8];
    fread(magic, 1, sizeof(magic), file);
    SC_ASSERT(strncmp(magic, "TOKYOPAK", sizeof(magic)) == 0);
    struct Header hdr = {0};
    fread(&hdr, 1, sizeof(hdr), file);

    // Load.
    uint8_t* encoded_xs = malloc(hdr.encoded_size_xs);
    uint8_t* encoded_ys = malloc(hdr.encoded_size_ys);
    uint8_t* encoded_zs = malloc(hdr.encoded_size_zs);
    uint8_t* encoded_rs = malloc(hdr.encoded_size_rs);
    uint8_t* encoded_gs = malloc(hdr.encoded_size_gs);
    uint8_t* encoded_bs = malloc(hdr.encoded_size_bs);
    fread(encoded_xs, 1, hdr.encoded_size_xs, file);
    fread(encoded_ys, 1, hdr.encoded_size_ys, file);
    fread(encoded_zs, 1, hdr.encoded_size_zs, file);
    fread(encoded_rs, 1, hdr.encoded_size_rs, file);
    fread(encoded_gs, 1, hdr.encoded_size_gs, file);
    fread(encoded_bs, 1, hdr.encoded_size_bs, file);
    SC_ASSERT(fgetc(file) == EOF);
    fclose(file);
    file = NULL;

    // Decode.
    uint32_t decoded_size_xs = hdr.point_count * sizeof(float);
    uint32_t decoded_size_ys = hdr.point_count * sizeof(float);
    uint32_t decoded_size_zs = hdr.point_count * sizeof(float);
    uint32_t decoded_size_rs = hdr.point_count * sizeof(uint8_t);
    uint32_t decoded_size_gs = hdr.point_count * sizeof(uint8_t);
    uint32_t decoded_size_bs = hdr.point_count * sizeof(uint8_t);
    float* xs = malloc(decoded_size_xs);
    float* ys = malloc(decoded_size_ys);
    float* zs = malloc(decoded_size_zs);
    uint8_t* rs = malloc(decoded_size_rs);
    uint8_t* gs = malloc(decoded_size_gs);
    uint8_t* bs = malloc(decoded_size_bs);
    size_t result_xs = ZSTD_decompress(xs, decoded_size_xs, encoded_xs, hdr.encoded_size_xs);
    size_t result_ys = ZSTD_decompress(ys, decoded_size_ys, encoded_ys, hdr.encoded_size_ys);
    size_t result_zs = ZSTD_decompress(zs, decoded_size_zs, encoded_zs, hdr.encoded_size_zs);
    size_t result_rs = ZSTD_decompress(rs, decoded_size_rs, encoded_rs, hdr.encoded_size_rs);
    size_t result_gs = ZSTD_decompress(gs, decoded_size_gs, encoded_gs, hdr.encoded_size_gs);
    size_t result_bs = ZSTD_decompress(bs, decoded_size_bs, encoded_bs, hdr.encoded_size_bs);
    SC_ASSERT(!ZSTD_isError(result_xs) && result_xs == decoded_size_xs);
    SC_ASSERT(!ZSTD_isError(result_ys) && result_ys == decoded_size_ys);
    SC_ASSERT(!ZSTD_isError(result_zs) && result_zs == decoded_size_zs);
    SC_ASSERT(!ZSTD_isError(result_rs) && result_rs == decoded_size_rs);
    SC_ASSERT(!ZSTD_isError(result_gs) && result_gs == decoded_size_gs);
    SC_ASSERT(!ZSTD_isError(result_bs) && result_bs == decoded_size_bs);
    free(encoded_xs);
    free(encoded_ys);
    free(encoded_zs);
    free(encoded_rs);
    free(encoded_gs);
    free(encoded_bs);

    // Pack.
    ScPoint* points = malloc(hdr.point_count * sizeof(ScPoint));
    for (uint32_t i = 0; i < hdr.point_count; i++) {
        points[i].position.x = xs[i];
        points[i].position.y = ys[i];
        points[i].position.z = zs[i];
        points[i].color.r = rs[i];
        points[i].color.g = gs[i];
        points[i].color.b = bs[i];
        points[i].color.a = 255;
    }
    free(xs);
    free(ys);
    free(zs);
    free(rs);
    free(gs);
    free(bs);

    // Bounds.
    ScBounds3 bounds = (ScBounds3) {
        .min = (ScVector3) {0.0f, 0.0f, 0.0f},
        .max = sc_vector3_sub(
            (ScVector3) {hdr.max_x, hdr.max_y, hdr.max_z},
            (ScVector3) {hdr.min_x, hdr.min_y, hdr.min_z}
        ),
    };
    ScVector3 center = sc_bounds3_center(bounds);

    // Re-center.
    for (uint32_t i = 0; i < hdr.point_count; i++) {
        points[i].position = sc_vector3_sub(points[i].position, center);
    }
    bounds.min = sc_vector3_sub(bounds.min, center);
    bounds.max = sc_vector3_sub(bounds.max, center);

    // Output.
    sc_data->bounds = bounds;
    sc_data->point_count = (uint32_t)hdr.point_count;
    sc_data->points = (const ScPoint*)points;

    // Timing.
    const uint64_t end_time_ns = SDL_GetTicksNS();
    const uint64_t elapsed_time_ns = end_time_ns - begin_time_ns;
    SC_LOG_INFO("Loaded %d points in %" PRIu64 " ms", hdr.point_count, elapsed_time_ns / 1000000);
}

static void sc_data_free(ScData* sc_data) {
    free((void*)sc_data->points);
}

//
// Stormcloud - Main.
//

#define SC_WINDOW_WIDTH 1280
#define SC_WINDOW_HEIGHT 800

typedef struct ScApp {
    ScData data;
    SDL_Window* window;
    SDL_GPUDevice* device;
    SDL_GPUTexture* depth_stencil_texture;
    SDL_GPUBuffer* point_buffer;
    SDL_GPUBuffer* line_buffer;
    uint32_t line_vertex_count;
    SDL_GPUShader* vertex_shader;
    SDL_GPUShader* fragment_shader;
    SDL_GPUGraphicsPipeline* point_pipeline;
    SDL_GPUGraphicsPipeline* line_pipeline;
} ScApp;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    // Arguments.
    SC_ASSERT(argc == 2);

    // Create app.
    ScApp* app = calloc(1, sizeof(ScApp));
    *appstate = app;

    // Load.
    sc_data_load(&app->data, argv[1]);

    // SDL.
    SDL_SetAppMetadata("stormcloud", "1.0.0", "com.phoekz.stormcloud");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SC_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Window & device.
    app->window = SDL_CreateWindow("stormcloud", SC_WINDOW_WIDTH, SC_WINDOW_HEIGHT, 0);
    if (app->window == NULL) {
        SC_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    app->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, false, "direct3d12");
    if (app->device == NULL) {
        SC_LOG_ERROR("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_ClaimWindowForGPUDevice(app->device, app->window)) {
        SC_LOG_ERROR("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    const SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
    const SDL_GPUSwapchainComposition composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR;
    if (!SDL_WindowSupportsGPUPresentMode(app->device, app->window, present_mode)) {
        SC_LOG_ERROR("SDL_WindowSupportsGPUPresentMode failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetGPUSwapchainParameters(app->device, app->window, composition, present_mode)) {
        SC_LOG_ERROR("SDL_SetGPUSwapchainParameters failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Depth stencil texture.
    SDL_GPUTextureFormat depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    {
        app->depth_stencil_texture = SDL_CreateGPUTexture(
            app->device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = depth_stencil_format,
                .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                .width = SC_WINDOW_WIDTH,
                .height = SC_WINDOW_HEIGHT,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            }
        );
    }

    // Vertex buffer - points.
    {
        const uint32_t vertex_count = app->data.point_count;
        app->point_buffer = SDL_CreateGPUBuffer(
            app->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = vertex_count * sizeof(ScPoint),
            }
        );
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            app->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = vertex_count * sizeof(ScPoint),
            }
        );
        ScPoint* data = SDL_MapGPUTransferBuffer(app->device, transfer_buffer, false);
        memcpy(data, app->data.points, vertex_count * sizeof(ScPoint));
        SDL_UnmapGPUTransferBuffer(app->device, transfer_buffer);
        SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(app->device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd);
        SDL_UploadToGPUBuffer(
            copy_pass,
            &(SDL_GPUTransferBufferLocation) {
                .transfer_buffer = transfer_buffer,
                .offset = 0,
            },
            &(SDL_GPUBufferRegion) {
                .buffer = app->point_buffer,
                .offset = 0,
                .size = vertex_count * sizeof(ScPoint),
            },
            false
        );
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(app->device, transfer_buffer);
    }

    // Vertex buffer - lines.
    typedef struct ScVertex {
        ScVector3 position;
        uint32_t color;
    } ScVertex;
    {
        uint32_t vertex_count = 0;
        vertex_count += 6;
        vertex_count += 2 * 12;
        app->line_vertex_count = vertex_count;
        app->line_buffer = SDL_CreateGPUBuffer(
            app->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = vertex_count * sizeof(ScVertex),
            }
        );
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            app->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = vertex_count * sizeof(ScVertex),
            }
        );
        ScVertex* data = SDL_MapGPUTransferBuffer(app->device, transfer_buffer, false);
        {
            *data++ = (ScVertex) {.position = {0.0f, 0.0f, 0.0f}, .color = 0xff0000ff};
            *data++ = (ScVertex) {.position = {1.0f, 0.0f, 0.0f}, .color = 0xff0000ff};
            *data++ = (ScVertex) {.position = {0.0f, 0.0f, 0.0f}, .color = 0xff00ff00};
            *data++ = (ScVertex) {.position = {0.0f, 1.0f, 0.0f}, .color = 0xff00ff00};
            *data++ = (ScVertex) {.position = {0.0f, 0.0f, 0.0f}, .color = 0xffff0000};
            *data++ = (ScVertex) {.position = {0.0f, 0.0f, 1.0f}, .color = 0xffff0000};
        }
        {
            // bounding box lines
            ScBounds3 bounds = app->data.bounds;
            ScVector3 mn = bounds.min;
            ScVector3 mx = bounds.max;
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mn.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mn.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mn.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mx.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mx.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mx.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mx.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mn.y, mn.z}, .color = 0xffffffff};

            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mn.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mn.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mn.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mx.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mx.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mx.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mx.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mn.y, mx.z}, .color = 0xffffffff};

            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mn.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mn.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mn.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mn.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mx.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mx.x, mx.y, mx.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mx.y, mn.z}, .color = 0xffffffff};
            *data++ = (ScVertex) {.position = (ScVector3) {mn.x, mx.y, mx.z}, .color = 0xffffffff};
        }
        SDL_UnmapGPUTransferBuffer(app->device, transfer_buffer);
        SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(app->device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd);
        SDL_UploadToGPUBuffer(
            copy_pass,
            &(SDL_GPUTransferBufferLocation) {
                .transfer_buffer = transfer_buffer,
                .offset = 0,
            },
            &(SDL_GPUBufferRegion) {
                .buffer = app->line_buffer,
                .offset = 0,
                .size = vertex_count * sizeof(ScVertex),
            },
            false
        );
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(app->device, transfer_buffer);
    }

    // Shaders.
    size_t vertex_code_size;
    size_t fragment_code_size;
    void* vertex_code = SDL_LoadFile("src/shaders/dxil/basic.vert", &vertex_code_size);
    void* fragment_code = SDL_LoadFile("src/shaders/dxil/basic.frag", &fragment_code_size);
    SC_ASSERT(vertex_code != NULL && vertex_code_size > 0);
    SC_ASSERT(fragment_code != NULL && fragment_code_size > 0);
    app->vertex_shader = SDL_CreateGPUShader(
        app->device,
        &(SDL_GPUShaderCreateInfo) {
            .code = vertex_code,
            .code_size = vertex_code_size,
            .entrypoint = "vs_main",
            .format = SDL_GPU_SHADERFORMAT_DXIL,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers = 0,
            .num_storage_textures = 0,
            .num_storage_buffers = 0,
            .num_uniform_buffers = 1,
        }
    );
    app->fragment_shader = SDL_CreateGPUShader(
        app->device,
        &(SDL_GPUShaderCreateInfo) {
            .code = fragment_code,
            .code_size = fragment_code_size,
            .entrypoint = "fs_main",
            .format = SDL_GPU_SHADERFORMAT_DXIL,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 0,
            .num_storage_textures = 0,
            .num_storage_buffers = 0,
            .num_uniform_buffers = 0,
        }
    );
    SC_SDL_ASSERT(app->vertex_shader != NULL);
    SC_SDL_ASSERT(app->fragment_shader != NULL);
    SDL_free(vertex_code);
    SDL_free(fragment_code);

    // Pipeline.
    app->point_pipeline = SDL_CreateGPUGraphicsPipeline(
        app->device,
        &(SDL_GPUGraphicsPipelineCreateInfo) {
            .vertex_shader = app->vertex_shader,
            .fragment_shader = app->fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState) {
                    .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {{
                        .slot = 0,
                        .pitch = sizeof(ScPoint),
                        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                        .instance_step_rate = 0,
                    }},
                    .num_vertex_buffers = 1,
                    .vertex_attributes =
                        (SDL_GPUVertexAttribute[]) {
                            {
                                .location = 0,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                .offset = 0,
                            },
                            {
                                .location = 1,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                                .offset = sizeof(ScVector3),
                            },
                        },
                    .num_vertex_attributes = 2,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST,
            .rasterizer_state =
                (SDL_GPURasterizerState) {
                    .fill_mode = SDL_GPU_FILLMODE_FILL,
                    .cull_mode = SDL_GPU_CULLMODE_NONE,
                    .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
                    .depth_bias_constant_factor = 0.0f,
                    .depth_bias_clamp = 0.0f,
                    .depth_bias_slope_factor = 0.0f,
                    .enable_depth_bias = false,
                    .enable_depth_clip = false,
                },
            .multisample_state =
                (SDL_GPUMultisampleState) {
                    .sample_count = 1,
                    .sample_mask = 0,
                    .enable_mask = 0,
                },
            .depth_stencil_state =
                (SDL_GPUDepthStencilState) {
                    .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
                    .back_stencil_state = (SDL_GPUStencilOpState) {0},
                    .front_stencil_state = (SDL_GPUStencilOpState) {0},
                    .compare_mask = 0,
                    .write_mask = 0,
                    .enable_depth_test = true,
                    .enable_depth_write = true,
                    .enable_stencil_test = false,
                },
            .target_info =
                (SDL_GPUGraphicsPipelineTargetInfo) {
                    .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
                        .format = SDL_GetGPUSwapchainTextureFormat(app->device, app->window),
                        .blend_state = (SDL_GPUColorTargetBlendState) {0},
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = depth_stencil_format,
                    .has_depth_stencil_target = true,
                },
        }
    );
    app->line_pipeline = SDL_CreateGPUGraphicsPipeline(
        app->device,
        &(SDL_GPUGraphicsPipelineCreateInfo) {
            .vertex_shader = app->vertex_shader,
            .fragment_shader = app->fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState) {
                    .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {{
                        .slot = 0,
                        .pitch = sizeof(ScVertex),
                        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                        .instance_step_rate = 0,
                    }},
                    .num_vertex_buffers = 1,
                    .vertex_attributes =
                        (SDL_GPUVertexAttribute[]) {
                            {
                                .location = 0,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                .offset = 0,
                            },
                            {
                                .location = 1,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                                .offset = sizeof(ScVector3),
                            },
                        },
                    .num_vertex_attributes = 2,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,
            .rasterizer_state =
                (SDL_GPURasterizerState) {
                    .fill_mode = SDL_GPU_FILLMODE_FILL,
                    .cull_mode = SDL_GPU_CULLMODE_NONE,
                    .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
                    .depth_bias_constant_factor = 0.0f,
                    .depth_bias_clamp = 0.0f,
                    .depth_bias_slope_factor = 0.0f,
                    .enable_depth_bias = false,
                    .enable_depth_clip = false,
                },
            .multisample_state =
                (SDL_GPUMultisampleState) {
                    .sample_count = 1,
                    .sample_mask = 0,
                    .enable_mask = 0,
                },
            .depth_stencil_state =
                (SDL_GPUDepthStencilState) {
                    .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
                    .back_stencil_state = (SDL_GPUStencilOpState) {0},
                    .front_stencil_state = (SDL_GPUStencilOpState) {0},
                    .compare_mask = 0,
                    .write_mask = 0,
                    .enable_depth_test = true,
                    .enable_depth_write = true,
                    .enable_stencil_test = false,
                },
            .target_info =
                (SDL_GPUGraphicsPipelineTargetInfo) {
                    .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
                        .format = SDL_GetGPUSwapchainTextureFormat(app->device, app->window),
                        .blend_state = (SDL_GPUColorTargetBlendState) {0},
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = depth_stencil_format,
                    .has_depth_stencil_target = true,
                },
        }
    );
    SC_SDL_ASSERT(app->point_pipeline != NULL);
    SC_SDL_ASSERT(app->line_pipeline != NULL);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    // Unpack.
    ScApp* app = (ScApp*)appstate;
    SC_UNUSED(app);

    // Handle events.
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    // Unpack.
    ScApp* app = (ScApp*)appstate;

    // Command buffer.
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(app->device);
    if (cmd == NULL) {
        SC_LOG_ERROR("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return -1;
    }

    // Swapchain.
    SDL_GPUTexture* swapchain = NULL;
    if (!SDL_AcquireGPUSwapchainTexture(cmd, app->window, &swapchain, NULL, NULL)) {
        SC_LOG_ERROR("SDL_AcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return -1;
    }

    // Skip rendering, if no swapchain.
    if (swapchain == NULL) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return SDL_APP_CONTINUE;
    }

    // Camera.
    const float fov = sc_rad_from_deg(60.0f);
    const float aspect = (float)SC_WINDOW_WIDTH / (float)SC_WINDOW_HEIGHT;
    const float radius = 350.0f;
    const float camera_height = 200.0f;
    const float camera_origin_x = SDL_cosf(SDL_GetTicks() * 0.00025f) * radius;
    const float camera_origin_y = SDL_sinf(SDL_GetTicks() * 0.00025f) * radius;
    const ScMatrix4x4 perspective = sc_matrix4x4_perspective(fov, aspect, 0.1f, 1000.0f);
    const ScMatrix4x4 view = sc_matrix4x4_look_at(
        (ScVector3) {camera_origin_x, camera_origin_y, camera_height},
        (ScVector3) {0.0f, 0.0f, 0.0f},
        (ScVector3) {0.0f, 0.0f, 1.0f}
    );
    const ScMatrix4x4 transform = sc_matrix4x4_multiply(view, perspective);

    // Render pass - begin.
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
        cmd,
        &(SDL_GPUColorTargetInfo) {
            .texture = swapchain,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = (SDL_FColor) {0.05f, 0.05f, 0.05f, 1.0f},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = false,
            .cycle_resolve_texture = false,
        },
        1,
        &(SDL_GPUDepthStencilTargetInfo) {
            .texture = app->depth_stencil_texture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
            .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = false,
            .clear_stencil = 0,
        }
    );

    // Draw - points.
    {
        SDL_BindGPUGraphicsPipeline(render_pass, app->point_pipeline);
        SDL_PushGPUVertexUniformData(cmd, 0, &transform, sizeof(transform));
        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            &(SDL_GPUBufferBinding) {
                .buffer = app->point_buffer,
                .offset = 0,
            },
            1
        );
        SDL_DrawGPUPrimitives(render_pass, app->data.point_count, 1, 0, 0);
    }

    // Draw - lines.
    {
        SDL_BindGPUGraphicsPipeline(render_pass, app->line_pipeline);
        SDL_PushGPUVertexUniformData(cmd, 0, &transform, sizeof(transform));
        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            &(SDL_GPUBufferBinding) {
                .buffer = app->line_buffer,
                .offset = 0,
            },
            1
        );
        SDL_DrawGPUPrimitives(render_pass, app->line_vertex_count, 1, 0, 0);
    }

    // Render pass - end.
    SDL_EndGPURenderPass(render_pass);

    // Submit.
    SDL_SubmitGPUCommandBuffer(cmd);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    // Unpack.
    ScApp* app = (ScApp*)appstate;

    // Destroy.
    SDL_ReleaseGPUGraphicsPipeline(app->device, app->point_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(app->device, app->line_pipeline);
    SDL_ReleaseGPUShader(app->device, app->vertex_shader);
    SDL_ReleaseGPUShader(app->device, app->fragment_shader);
    SDL_ReleaseGPUBuffer(app->device, app->line_buffer);
    SDL_ReleaseGPUBuffer(app->device, app->point_buffer);
    SDL_ReleaseGPUTexture(app->device, app->depth_stencil_texture);
    SDL_ReleaseWindowFromGPUDevice(app->device, app->window);
    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->device);

    // Free.
    sc_data_free(&app->data);
    free(app);

    // End.
    SC_ASSERT(result == SDL_APP_SUCCESS);
}
