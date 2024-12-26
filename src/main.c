// Includes.
#include <assert.h>
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

static uint64_t u64_min(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

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

//
// Stormcloud - Data.
//

typedef struct ScColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ScColor;

typedef struct ScData {
    uint64_t point_count;
    const ScVector3* positions;
    const ScColor* colors;
} ScData;

static void sc_data_load(ScData* sc_data, const char* pak_path) {
    // Timing.
    const uint64_t begin_time_ns = SDL_GetTicksNS();

    // Declarations.
    struct Header {
        uint64_t point_count;
        float min_x;
        float min_y;
        float min_z;
        float max_x;
        float max_y;
        float max_z;
        uint64_t encoded_size_xs;
        uint64_t encoded_size_ys;
        uint64_t encoded_size_zs;
        uint64_t encoded_size_rs;
        uint64_t encoded_size_gs;
        uint64_t encoded_size_bs;
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
    uint64_t decoded_size_xs = hdr.point_count * sizeof(float);
    uint64_t decoded_size_ys = hdr.point_count * sizeof(float);
    uint64_t decoded_size_zs = hdr.point_count * sizeof(float);
    uint64_t decoded_size_rs = hdr.point_count * sizeof(uint8_t);
    uint64_t decoded_size_gs = hdr.point_count * sizeof(uint8_t);
    uint64_t decoded_size_bs = hdr.point_count * sizeof(uint8_t);
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

    // Interleave.
    ScVector3* positions = malloc(hdr.point_count * sizeof(ScVector3));
    ScColor* colors = malloc(hdr.point_count * sizeof(ScColor));
    for (uint64_t i = 0; i < hdr.point_count; i++) {
        positions[i].x = xs[i];
        positions[i].y = ys[i];
        positions[i].z = zs[i];
        colors[i].r = rs[i];
        colors[i].g = gs[i];
        colors[i].b = bs[i];
    }
    free(xs);
    free(ys);
    free(zs);
    free(rs);
    free(gs);
    free(bs);

    // Output.
    sc_data->point_count = hdr.point_count;
    sc_data->positions = (const ScVector3*)positions;
    sc_data->colors = (const ScColor*)colors;

    // Timing.
    const uint64_t end_time_ns = SDL_GetTicksNS();
    const uint64_t elapsed_time_ns = end_time_ns - begin_time_ns;
    SC_LOG_INFO("Loaded %d points in %" PRIu64 " ms", hdr.point_count, elapsed_time_ns / 1000000);
}

static void sc_data_free(ScData* sc_data) {
    free((void*)sc_data->positions);
    free((void*)sc_data->colors);
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
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUShader* vertex_shader;
    SDL_GPUShader* fragment_shader;
    SDL_GPUGraphicsPipeline* pipeline;
} ScApp;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    // Arguments.
    SC_ASSERT(argc == 2);

    // Create app.
    ScApp* app = calloc(1, sizeof(ScApp));
    *appstate = app;

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

    // Vertex buffer.
    typedef struct ScVertex {
        ScVector3 position;
        uint32_t color;
    } ScVertex;
    uint32_t vertex_count = 6;
    {
        app->vertex_buffer = SDL_CreateGPUBuffer(
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
        ScVertex* vertex_buffer_data =
            SDL_MapGPUTransferBuffer(app->device, transfer_buffer, false);
        vertex_buffer_data[0] = (ScVertex) {.position = {0.0f, 0.0f, 0.0f}, .color = 0xff0000ff};
        vertex_buffer_data[1] = (ScVertex) {.position = {1.0f, 0.0f, 0.0f}, .color = 0xff0000ff};
        vertex_buffer_data[2] = (ScVertex) {.position = {0.0f, 0.0f, 0.0f}, .color = 0xff00ff00};
        vertex_buffer_data[3] = (ScVertex) {.position = {0.0f, 1.0f, 0.0f}, .color = 0xff00ff00};
        vertex_buffer_data[4] = (ScVertex) {.position = {0.0f, 0.0f, 0.0f}, .color = 0xffff0000};
        vertex_buffer_data[5] = (ScVertex) {.position = {0.0f, 0.0f, 1.0f}, .color = 0xffff0000};
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
                .buffer = app->vertex_buffer,
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
    app->pipeline = SDL_CreateGPUGraphicsPipeline(
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
            .depth_stencil_state = (SDL_GPUDepthStencilState) {0},
            .target_info =
                (SDL_GPUGraphicsPipelineTargetInfo) {
                    .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
                        .format = SDL_GetGPUSwapchainTextureFormat(app->device, app->window),
                        .blend_state = (SDL_GPUColorTargetBlendState) {0},
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID,
                    .has_depth_stencil_target = false,
                },
        }
    );
    SC_SDL_ASSERT(app->pipeline != NULL);

    // Load.
    sc_data_load(&app->data, argv[1]);

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

    // Render pass.
    if (swapchain != NULL) {
        SDL_GPUColorTargetInfo color_target_info = {0};
        color_target_info.texture = swapchain;
        color_target_info.clear_color = (SDL_FColor) {0.1f, 0.1f, 0.1f, 1.0f};
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &color_target_info, 1, NULL);
        SDL_BindGPUGraphicsPipeline(render_pass, app->pipeline);

        const float fov = sc_rad_from_deg(60.0f);
        const float aspect = (float)SC_WINDOW_WIDTH / (float)SC_WINDOW_HEIGHT;
        const ScMatrix4x4 perspective = sc_matrix4x4_perspective(fov, aspect, 0.1f, 100.0f);
        const ScMatrix4x4 view = sc_matrix4x4_look_at(
            (ScVector3) {5.0f, 5.0f, 2.5f},
            (ScVector3) {0.0f, 0.0f, 0.0f},
            (ScVector3) {0.0f, 0.0f, 1.0f}
        );
        const ScMatrix4x4 transform = sc_matrix4x4_multiply(view, perspective);
        SDL_PushGPUVertexUniformData(cmd, 0, &transform, sizeof(transform));
        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            &(SDL_GPUBufferBinding) {
                .buffer = app->vertex_buffer,
                .offset = 0,
            },
            1
        );
        SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);
        SDL_EndGPURenderPass(render_pass);
    }

    // Submit.
    SDL_SubmitGPUCommandBuffer(cmd);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    // Unpack.
    ScApp* app = (ScApp*)appstate;

    // Destroy.
    SDL_ReleaseGPUGraphicsPipeline(app->device, app->pipeline);
    SDL_ReleaseGPUShader(app->device, app->vertex_shader);
    SDL_ReleaseGPUShader(app->device, app->fragment_shader);
    SDL_ReleaseGPUBuffer(app->device, app->vertex_buffer);
    SDL_ReleaseWindowFromGPUDevice(app->device, app->window);
    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->device);

    // Free.
    sc_data_free(&app->data);
    free(app);

    // End.
    SC_ASSERT(result == SDL_APP_SUCCESS);
}
