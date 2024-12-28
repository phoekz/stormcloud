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
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
} ScMatrix4x4;

typedef struct ScBounds3 {
    ScVector3 min;
    ScVector3 max;
} ScBounds3;

ScVector3 sc_vector3_add(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
}

ScVector3 sc_vector3_sub(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

float sc_vector3_length(ScVector3 vec) {
    return SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

ScVector3 sc_vector3_normalized(ScVector3 vec) {
    const float length = sc_vector3_length(vec);
    return (ScVector3) {
        vec.x / length,
        vec.y / length,
        vec.z / length,
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
    m.m00 = (lhs.m00 * rhs.m00) + (lhs.m01 * rhs.m10) + (lhs.m02 * rhs.m20) + (lhs.m03 * rhs.m30);
    m.m01 = (lhs.m00 * rhs.m01) + (lhs.m01 * rhs.m11) + (lhs.m02 * rhs.m21) + (lhs.m03 * rhs.m31);
    m.m02 = (lhs.m00 * rhs.m02) + (lhs.m01 * rhs.m12) + (lhs.m02 * rhs.m22) + (lhs.m03 * rhs.m32);
    m.m03 = (lhs.m00 * rhs.m03) + (lhs.m01 * rhs.m13) + (lhs.m02 * rhs.m23) + (lhs.m03 * rhs.m33);
    m.m10 = (lhs.m10 * rhs.m00) + (lhs.m11 * rhs.m10) + (lhs.m12 * rhs.m20) + (lhs.m13 * rhs.m30);
    m.m11 = (lhs.m10 * rhs.m01) + (lhs.m11 * rhs.m11) + (lhs.m12 * rhs.m21) + (lhs.m13 * rhs.m31);
    m.m12 = (lhs.m10 * rhs.m02) + (lhs.m11 * rhs.m12) + (lhs.m12 * rhs.m22) + (lhs.m13 * rhs.m32);
    m.m13 = (lhs.m10 * rhs.m03) + (lhs.m11 * rhs.m13) + (lhs.m12 * rhs.m23) + (lhs.m13 * rhs.m33);
    m.m20 = (lhs.m20 * rhs.m00) + (lhs.m21 * rhs.m10) + (lhs.m22 * rhs.m20) + (lhs.m23 * rhs.m30);
    m.m21 = (lhs.m20 * rhs.m01) + (lhs.m21 * rhs.m11) + (lhs.m22 * rhs.m21) + (lhs.m23 * rhs.m31);
    m.m22 = (lhs.m20 * rhs.m02) + (lhs.m21 * rhs.m12) + (lhs.m22 * rhs.m22) + (lhs.m23 * rhs.m32);
    m.m23 = (lhs.m20 * rhs.m03) + (lhs.m21 * rhs.m13) + (lhs.m22 * rhs.m23) + (lhs.m23 * rhs.m33);
    m.m30 = (lhs.m30 * rhs.m00) + (lhs.m31 * rhs.m10) + (lhs.m32 * rhs.m20) + (lhs.m33 * rhs.m30);
    m.m31 = (lhs.m30 * rhs.m01) + (lhs.m31 * rhs.m11) + (lhs.m32 * rhs.m21) + (lhs.m33 * rhs.m31);
    m.m32 = (lhs.m30 * rhs.m02) + (lhs.m31 * rhs.m12) + (lhs.m32 * rhs.m22) + (lhs.m33 * rhs.m32);
    m.m33 = (lhs.m30 * rhs.m03) + (lhs.m31 * rhs.m13) + (lhs.m32 * rhs.m23) + (lhs.m33 * rhs.m33);
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
    const ScVector3 vec_a = sc_vector3_normalized(target_to_origin);
    const ScVector3 vec_b = sc_vector3_normalized(sc_vector3_cross(up, vec_a));
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

ScVector3 sc_bounds3_extents(ScBounds3 bounds) {
    return (ScVector3) {
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z,
    };
}

ScVector3 sc_bounds3_center(ScBounds3 bounds) {
    return (ScVector3) {
        (bounds.min.x + bounds.max.x) * 0.5f,
        (bounds.min.y + bounds.max.y) * 0.5f,
        (bounds.min.z + bounds.max.z) * 0.5f,
    };
}

//
// Stormcloud - Octree.
//

typedef struct ScOctreeNode {
    uint32_t index;
    uint32_t level;
    int32_t min_x;
    int32_t min_y;
    int32_t min_z;
    int32_t max_x;
    int32_t max_y;
    int32_t max_z;
    uint32_t mask;
    uint32_t pad_0;
    uint32_t pad_1;
    uint32_t pad_2;
    uint64_t point_count;
    uint64_t point_offset;
    uint32_t octants[8];
} ScOctreeNode;

typedef struct ScOctreeNodeInstance {
    int32_t min_x;
    int32_t min_y;
    int32_t min_z;
    int32_t max_x;
    int32_t max_y;
    int32_t max_z;
} ScOctreeNodeInstance;

typedef struct ScOctreePoint {
    uint32_t position;
    uint32_t color;
} ScOctreePoint;

typedef struct ScOctree {
    ScOctreeNode* nodes;
    ScOctreeNodeInstance* instances;
    uint64_t node_count;
    ScOctreePoint* points;
    uint64_t point_count;
    ScBounds3 point_bounds;
} ScOctree;

static void sc_octree_new(ScOctree* octree, const char* file_path) {
    // Timing.
    const uint64_t begin_time_ns = SDL_GetTicksNS();

    // Load header.
    FILE* file = fopen(file_path, "rb");
    char magic[8];
    fread(magic, 1, sizeof(magic), file);
    SC_ASSERT(strncmp(magic, "TOKYOOCT", sizeof(magic)) == 0);
    uint64_t node_count;
    fread(&node_count, 1, sizeof(node_count), file);
    SC_LOG_INFO("Node count: %llu", node_count);
    uint64_t point_count;
    fread(&point_count, 1, sizeof(point_count), file);
    SC_LOG_INFO("Point count: %llu", point_count);
    ScBounds3 point_bounds;
    fread(&point_bounds, 1, sizeof(point_bounds), file);
    ScVector3 point_bounds_extents = sc_bounds3_extents(point_bounds);
    ScVector3 point_bounds_center = sc_bounds3_center(point_bounds);
    SC_LOG_INFO(
        "Point bounds min: %f, %f, %f",
        point_bounds.min.x,
        point_bounds.min.y,
        point_bounds.min.z
    );
    SC_LOG_INFO(
        "Point bounds max: %f, %f, %f",
        point_bounds.max.x,
        point_bounds.max.y,
        point_bounds.max.z
    );
    SC_LOG_INFO(
        "Point bounds extents: %f, %f, %f",
        point_bounds_extents.x,
        point_bounds_extents.y,
        point_bounds_extents.z
    );
    SC_LOG_INFO(
        "Point bounds center: %f, %f, %f",
        point_bounds_center.x,
        point_bounds_center.y,
        point_bounds_center.z
    );

    // Load nodes.
    ScOctreeNode* nodes = malloc(node_count * sizeof(ScOctreeNode));
    fread(nodes, 1, node_count * sizeof(ScOctreeNode), file);

    // Load points.
    ScOctreePoint* points = malloc(point_count * sizeof(ScOctreePoint));
    fread(points, 1, point_count * sizeof(ScOctreePoint), file);

    // Create instances.
    ScOctreeNodeInstance* instances = malloc(node_count * sizeof(ScOctreeNodeInstance));
    for (uint64_t i = 0; i < node_count; ++i) {
        const ScOctreeNode* node = &nodes[i];
        instances[i] = (ScOctreeNodeInstance) {
            .min_x = node->min_x,
            .min_y = node->min_y,
            .min_z = node->min_z,
            .max_x = node->max_x,
            .max_y = node->max_y,
            .max_z = node->max_z,
        };
    }

    // Output.
    octree->nodes = nodes;
    octree->instances = instances;
    octree->node_count = node_count;
    octree->points = points;
    octree->point_count = point_count;
    octree->point_bounds = point_bounds;

    // Timing.
    const uint64_t end_time_ns = SDL_GetTicksNS();
    const uint64_t elapsed_time_ns = end_time_ns - begin_time_ns;
    SC_LOG_INFO("Loaded %d points in %" PRIu64 " ms", point_count, elapsed_time_ns / 1000000);
}

static void sc_octree_free(ScOctree* octree) {
    free(octree->nodes);
    free(octree->instances);
    free(octree->points);
}

//
// Stormcloud - GPU utilities.
//

typedef struct ScGpuShaderCreateInfo {
    const char* file_path;
    const char* entry_point;
    SDL_GPUShaderStage shader_stage;
    uint32_t uniform_buffer_count;
} ScGpuShaderCreateInfo;

static SDL_GPUShader*
sc_gpu_shader_new(SDL_GPUDevice* device, const ScGpuShaderCreateInfo* create_info) {
    size_t code_size;
    void* code = SDL_LoadFile(create_info->file_path, &code_size);
    SC_ASSERT(code != NULL && code_size > 0);
    SDL_GPUShader* shader = SDL_CreateGPUShader(
        device,
        &(SDL_GPUShaderCreateInfo) {
            .code = code,
            .code_size = code_size,
            .entrypoint = create_info->entry_point,
            .format = SDL_GPU_SHADERFORMAT_DXIL,
            .stage = create_info->shader_stage,
            .num_samplers = 0,
            .num_storage_textures = 0,
            .num_storage_buffers = 0,
            .num_uniform_buffers = create_info->uniform_buffer_count,
        }
    );
    SC_SDL_ASSERT(shader != NULL);
    SDL_free(code);
    return shader;
}

//
// Stormcloud - Debug draw.
//

typedef struct ScDebugDrawVertex {
    ScVector3 position;
    uint32_t color;
} ScDebugDrawVertex;

typedef struct ScDebugDrawCreateInfo {
    SDL_GPUTextureFormat color_format;
    SDL_GPUTextureFormat depth_stencil_format;
} ScDebugDrawCreateInfo;

typedef struct ScDebugRenderInfo {
    SDL_GPUDevice* device;
    SDL_GPUCommandBuffer* command_buffer;
    SDL_GPURenderPass* render_pass;
    SDL_GPUViewport viewport;
    ScMatrix4x4 transform;
} ScDebugRenderInfo;

typedef struct ScDebugDraw {
    uint32_t line_count;
    uint32_t line_capacity;
    uint32_t line_byte_count;
    ScDebugDrawVertex* lines;

    SDL_GPUTransferBuffer* line_transfer_buffer;
    SDL_GPUBuffer* line_buffer;

    SDL_GPUGraphicsPipeline* line_pipeline;
} ScDebugDraw;

static void
sc_ddraw_new(SDL_GPUDevice* device, const ScDebugDrawCreateInfo* create_info, ScDebugDraw* ddraw) {
    // Buffers.
    ddraw->line_count = 0;
    ddraw->line_capacity = 1024;
    ddraw->line_byte_count = ddraw->line_capacity * sizeof(ScDebugDrawVertex);
    ddraw->lines = malloc(ddraw->line_byte_count);
    ddraw->line_transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = ddraw->line_byte_count,
        }
    );
    ddraw->line_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = ddraw->line_byte_count,
        }
    );

    // Shaders.
    SDL_GPUShader* vertex_shader = sc_gpu_shader_new(
        device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/ddraw.vert",
            .entry_point = "vs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* fragment_shader = sc_gpu_shader_new(
        device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/ddraw.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .uniform_buffer_count = 1,
        }
    );

    // Pipeline.
    ddraw->line_pipeline = SDL_CreateGPUGraphicsPipeline(
        device,
        &(SDL_GPUGraphicsPipelineCreateInfo) {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState) {
                    .vertex_buffer_descriptions =
                        &(SDL_GPUVertexBufferDescription) {
                            .slot = 0,
                            .pitch = sizeof(ScDebugDrawVertex),
                            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                            .instance_step_rate = 0,

                        },
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
                        .format = create_info->color_format,
                        .blend_state = (SDL_GPUColorTargetBlendState) {0},
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = create_info->depth_stencil_format,
                    .has_depth_stencil_target = true,
                },
        }
    );

    // Release shaders.
    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);
}

static void sc_ddraw_line(ScDebugDraw* ddraw, ScVector3 a, ScVector3 b, uint32_t color) {
    // Validation.
    SC_ASSERT(ddraw->line_count + 2 <= ddraw->line_capacity);

    // Write.
    ScDebugDrawVertex* vertices = &ddraw->lines[ddraw->line_count];
    vertices[0] = (ScDebugDrawVertex) {.position = a, .color = color};
    vertices[1] = (ScDebugDrawVertex) {.position = b, .color = color};
    ddraw->line_count += 2;
}

static void sc_ddraw_render(ScDebugDraw* ddraw, ScDebugRenderInfo* render_info) {
    // Early out.
    if (ddraw->line_count == 0) {
        return;
    }

    // Unpack.
    SDL_GPUDevice* device = render_info->device;
    SDL_GPUCommandBuffer* command_buffer = render_info->command_buffer;
    SDL_GPURenderPass* render_pass = render_info->render_pass;
    SDL_GPUViewport viewport = render_info->viewport;

    // Copy to device.
    const uint32_t line_byte_count = ddraw->line_count * sizeof(ScDebugDrawVertex);
    ScDebugDrawVertex* dst = SDL_MapGPUTransferBuffer(device, ddraw->line_transfer_buffer, false);
    memcpy(dst, ddraw->lines, line_byte_count);
    SDL_UnmapGPUTransferBuffer(device, ddraw->line_transfer_buffer);

    // Device commands.
    SDL_GPUCommandBuffer* upload_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_buffer);
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = ddraw->line_transfer_buffer,
            .offset = 0,
        },
        &(SDL_GPUBufferRegion) {
            .buffer = ddraw->line_buffer,
            .offset = 0,
            .size = line_byte_count,
        },
        false
    );
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_buffer);

    // Render.
    SDL_BindGPUGraphicsPipeline(render_pass, ddraw->line_pipeline);
    SDL_BindGPUVertexBuffers(
        render_pass,
        0,
        (SDL_GPUBufferBinding[]) {
            {
                .buffer = ddraw->line_buffer,
                .offset = 0,
            },
        },
        1
    );
    SDL_SetGPUViewport(render_pass, &viewport);
    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &render_info->transform,
        sizeof(render_info->transform)
    );
    SDL_DrawGPUPrimitives(render_pass, ddraw->line_count, 1, 0, 0);

    // Reset.
    ddraw->line_count = 0;
}

static void sc_ddraw_free(SDL_GPUDevice* device, ScDebugDraw* ddraw) {
    SDL_ReleaseGPUGraphicsPipeline(device, ddraw->line_pipeline);
    SDL_ReleaseGPUBuffer(device, ddraw->line_buffer);
    SDL_ReleaseGPUTransferBuffer(device, ddraw->line_transfer_buffer);
    free(ddraw->lines);
}

//
// Stormcloud - App.
//

#define SC_WINDOW_WIDTH 1920
#define SC_WINDOW_HEIGHT 1200

typedef struct ScApp {
    ScOctree octree;
    ScDebugDraw ddraw;

    SDL_Window* window;
    SDL_GPUDevice* device;
    SDL_GPUTexture* depth_stencil_texture;

    SDL_GPUBuffer* point_buffer;
    SDL_GPUBuffer* node_buffer;

    SDL_GPUBuffer* bounds_buffer;
    uint32_t bounds_vertex_count;

    SDL_GPUGraphicsPipeline* point_pipeline;
    SDL_GPUGraphicsPipeline* bounds_pipeline;
} ScApp;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    // Arguments.
    SC_ASSERT(argc == 2);

    // Create app.
    ScApp* app = calloc(1, sizeof(ScApp));
    *appstate = app;

    // Octree.
    sc_octree_new(&app->octree, argv[1]);

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

    // Debug draw.
    sc_ddraw_new(
        app->device,
        &(ScDebugDrawCreateInfo) {
            .color_format = SDL_GetGPUSwapchainTextureFormat(app->device, app->window),
            .depth_stencil_format = depth_stencil_format,
        },
        &app->ddraw
    );

    // Vertex buffer - points.
    {
        const uint32_t vertex_count = (uint32_t)app->octree.point_count;
        const uint32_t vertex_byte_count = vertex_count * sizeof(ScOctreePoint);
        app->point_buffer = SDL_CreateGPUBuffer(
            app->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = vertex_byte_count,
            }
        );
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            app->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = vertex_byte_count,
            }
        );
        ScOctreePoint* data = SDL_MapGPUTransferBuffer(app->device, transfer_buffer, false);
        memcpy(data, app->octree.points, vertex_byte_count);
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
                .size = vertex_byte_count,
            },
            false
        );
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(app->device, transfer_buffer);
    }

    // Vertex buffer - nodes.
    {
        const uint32_t node_count = (uint32_t)app->octree.node_count;
        const uint32_t node_byte_count = node_count * sizeof(ScOctreeNodeInstance);
        app->node_buffer = SDL_CreateGPUBuffer(
            app->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = node_byte_count,
            }
        );
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            app->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = node_byte_count,
            }
        );
        ScOctreeNodeInstance* data = SDL_MapGPUTransferBuffer(app->device, transfer_buffer, false);
        memcpy(data, app->octree.instances, node_byte_count);
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
                .buffer = app->node_buffer,
                .offset = 0,
                .size = node_byte_count,
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
        const uint32_t vertex_count = 2 * 12;
        const uint32_t vertex_byte_count = vertex_count * sizeof(ScVertex);
        app->bounds_vertex_count = vertex_count;
        app->bounds_buffer = SDL_CreateGPUBuffer(
            app->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = vertex_byte_count,
            }
        );
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            app->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = vertex_byte_count,
            }
        );
        ScVertex* data = SDL_MapGPUTransferBuffer(app->device, transfer_buffer, false);
        const ScVector3 mn = {0.0f, 0.0f, 0.0f};
        const ScVector3 mx = {1.0f, 1.0f, 1.0f};
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
                .buffer = app->bounds_buffer,
                .offset = 0,
                .size = vertex_byte_count,
            },
            false
        );
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(app->device, transfer_buffer);
    }

    // Shaders.
    SDL_GPUShader* point_vertex_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/point.vert",
            .entry_point = "vs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* point_fragment_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/point.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* bounds_vertex_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/bounds.vert",
            .entry_point = "vs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* bounds_fragment_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/bounds.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .uniform_buffer_count = 1,
        }
    );

    // Pipeline.
    app->point_pipeline = SDL_CreateGPUGraphicsPipeline(
        app->device,
        &(SDL_GPUGraphicsPipelineCreateInfo) {
            .vertex_shader = point_vertex_shader,
            .fragment_shader = point_fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState) {
                    .vertex_buffer_descriptions =
                        (SDL_GPUVertexBufferDescription[]) {
                            {
                                .slot = 0,
                                .pitch = sizeof(ScOctreePoint),
                                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                                .instance_step_rate = 0,
                            },
                            {
                                .slot = 1,
                                .pitch = sizeof(ScOctreeNodeInstance),
                                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                                .instance_step_rate = 1,
                            },
                        },
                    .num_vertex_buffers = 2,
                    .vertex_attributes =
                        (SDL_GPUVertexAttribute[]) {
                            {
                                .location = 0,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
                                .offset = 0,
                            },
                            {
                                .location = 1,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                                .offset = sizeof(uint32_t),
                            },
                            {
                                .location = 2,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_INT3,
                                .offset = 0,
                            },
                            {
                                .location = 3,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_INT3,
                                .offset = 3 * sizeof(int32_t),
                            },
                        },
                    .num_vertex_attributes = 4,
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
    app->bounds_pipeline = SDL_CreateGPUGraphicsPipeline(
        app->device,
        &(SDL_GPUGraphicsPipelineCreateInfo) {
            .vertex_shader = bounds_vertex_shader,
            .fragment_shader = bounds_fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState) {
                    .vertex_buffer_descriptions =
                        (SDL_GPUVertexBufferDescription[]) {
                            {
                                .slot = 0,
                                .pitch = sizeof(ScVertex),
                                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                                .instance_step_rate = 0,
                            },
                            {
                                .slot = 1,
                                .pitch = sizeof(ScOctreeNodeInstance),
                                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                                .instance_step_rate = 1,
                            },
                        },
                    .num_vertex_buffers = 2,
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
                            {
                                .location = 2,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_INT3,
                                .offset = 0,
                            },
                            {
                                .location = 3,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_INT3,
                                .offset = 3 * sizeof(int32_t),
                            },
                        },
                    .num_vertex_attributes = 4,
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
    SC_SDL_ASSERT(app->bounds_pipeline != NULL);

    // Release.
    SDL_ReleaseGPUShader(app->device, point_vertex_shader);
    SDL_ReleaseGPUShader(app->device, point_fragment_shader);
    SDL_ReleaseGPUShader(app->device, bounds_vertex_shader);
    SDL_ReleaseGPUShader(app->device, bounds_fragment_shader);

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
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
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

    // Render pass - begin.
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
        cmd,
        &(SDL_GPUColorTargetInfo) {
            .texture = swapchain,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = (SDL_FColor) {0.025f, 0.025f, 0.025f, 1.0f},
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

    // Camera.
    enum {
        MAIN,
        AERIAL
    };
    SDL_GPUViewport viewports[2];
    ScMatrix4x4 transforms[2];
    {
        // Common.
        const float time = (float)(SDL_GetTicks() / 1000.0);
        const float fov = sc_rad_from_deg(60.0f);
        const float window_width = (float)(SC_WINDOW_WIDTH / 2);
        const float window_height = (float)SC_WINDOW_HEIGHT;
        const float aspect = window_width / window_height;
        const float z_near = 0.1f;
        const float z_far = 1000.0f;
        const ScVector3 point_origin = sc_bounds3_center(app->octree.point_bounds);

        // Main.
        {
            const float camera_turn_speed = 0.25f;
            const float camera_offset_radius = 500.0f;
            const ScVector3 camera_offset = (ScVector3) {
                camera_offset_radius * SDL_cosf(camera_turn_speed * time),
                camera_offset_radius * SDL_sinf(camera_turn_speed * time),
                camera_offset_radius * 0.5f,
            };
            const ScVector3 camera_origin = sc_vector3_add(point_origin, camera_offset);
            const ScVector3 camera_target = point_origin;
            const ScVector3 camera_up = (ScVector3) {0.0f, 0.0f, 1.0f};
            const ScMatrix4x4 perspective = sc_matrix4x4_perspective(fov, aspect, z_near, z_far);
            const ScMatrix4x4 view = sc_matrix4x4_look_at(camera_origin, camera_target, camera_up);
            transforms[MAIN] = sc_matrix4x4_multiply(view, perspective);
            viewports[MAIN] = (SDL_GPUViewport) {
                .x = 0.0f,
                .y = 0.0f,
                .w = window_width,
                .h = window_height,
                .min_depth = 0.0f,
                .max_depth = 1.0f,
            };

            sc_ddraw_line(&app->ddraw, camera_origin, camera_target, 0xff00ff00);
        }

        // Aerial.
        {
            const ScVector3 camera_offset = (ScVector3) {0.0f, -500.0f, 1000.0f};
            const ScVector3 camera_origin = sc_vector3_add(point_origin, camera_offset);
            const ScVector3 camera_target = point_origin;
            const ScVector3 camera_up = (ScVector3) {0.0f, 1.0f, 0.0f};
            const ScMatrix4x4 perspective = sc_matrix4x4_perspective(fov, aspect, z_near, z_far);
            const ScMatrix4x4 view = sc_matrix4x4_look_at(camera_origin, camera_target, camera_up);
            transforms[AERIAL] = sc_matrix4x4_multiply(view, perspective);
            viewports[AERIAL] = (SDL_GPUViewport) {
                .x = window_width,
                .y = 0.0f,
                .w = window_width,
                .h = window_height,
                .min_depth = 0.0f,
                .max_depth = 1.0f,
            };
        }
    }

    const uint32_t wanted_level = 10;

    // Draw - points.
    {
        SDL_BindGPUGraphicsPipeline(render_pass, app->point_pipeline);
        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            (SDL_GPUBufferBinding[]) {
                {
                    .buffer = app->point_buffer,
                    .offset = 0,
                },
                {
                    .buffer = app->node_buffer,
                    .offset = 0,
                },
            },
            2
        );

        for (uint32_t i = 0; i < 2; i++) {
            SDL_SetGPUViewport(render_pass, &viewports[i]);
            SDL_PushGPUVertexUniformData(cmd, 0, &transforms[i], sizeof(transforms[i]));
            for (uint32_t node_idx = 0; node_idx < (uint32_t)app->octree.node_count; node_idx++) {
                ScOctreeNode* node = &app->octree.nodes[node_idx];
                if (node->level == wanted_level) {
                    uint32_t vertex_count = (uint32_t)node->point_count;
                    uint32_t vertex_offset = (uint32_t)node->point_offset;
                    SDL_DrawGPUPrimitives(render_pass, vertex_count, 1, vertex_offset, node_idx);
                }
            }
        }
    }

    // Draw - lines.
    {
        SDL_BindGPUGraphicsPipeline(render_pass, app->bounds_pipeline);
        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            (SDL_GPUBufferBinding[]) {
                {
                    .buffer = app->bounds_buffer,
                    .offset = 0,
                },
                {
                    .buffer = app->node_buffer,
                    .offset = 0,
                },
            },
            2
        );

        for (uint32_t i = 0; i < 2; i++) {
            SDL_SetGPUViewport(render_pass, &viewports[i]);
            SDL_PushGPUVertexUniformData(cmd, 0, &transforms[i], sizeof(transforms[i]));
            for (uint32_t node_idx = 0; node_idx < (uint32_t)app->octree.node_count; node_idx++) {
                ScOctreeNode* node = &app->octree.nodes[node_idx];
                if (node->level == wanted_level) {
                    SDL_DrawGPUPrimitives(render_pass, app->bounds_vertex_count, 1, 0, node_idx);
                }
            }
        }
    }

    // Draw - debug.
    sc_ddraw_render(
        &app->ddraw,
        &(ScDebugRenderInfo) {
            .device = app->device,
            .command_buffer = cmd,
            .render_pass = render_pass,
            .viewport = viewports[AERIAL],
            .transform = transforms[AERIAL],
        }
    );

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
    SDL_ReleaseGPUGraphicsPipeline(app->device, app->bounds_pipeline);
    SDL_ReleaseGPUBuffer(app->device, app->point_buffer);
    SDL_ReleaseGPUBuffer(app->device, app->bounds_buffer);
    SDL_ReleaseGPUTexture(app->device, app->depth_stencil_texture);
    sc_ddraw_free(app->device, &app->ddraw);
    SDL_ReleaseWindowFromGPUDevice(app->device, app->window);
    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->device);

    // Free.
    sc_octree_free(&app->octree);
    free(app);

    // End.
    SC_ASSERT(result == SDL_APP_SUCCESS);
}
