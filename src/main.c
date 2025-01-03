//
// Stormcloud - Includes.
//

#include "common.h"
#include "math.h"
#include "color.h"
#include "camera.h"
#include "octree.h"
#include "gpu.h"
#include "ddraw.h"
#include "gui.h"

//
// Stormcloud - App Camera.
//

typedef enum ScAppViewMode {
    VIEW_MODE_FULLSCREEN,
    VIEW_MODE_SPLIT,
    VIEW_MODE_COUNT,
} ScAppViewMode;

static const char* SC_APP_VIEW_MODE_NAME[] = {
    "Fullscreen",
    "Split",
};

typedef enum ScAppMainCameraControlType {
    MAIN_CAMERA_CONTROL_TYPE_ORBIT,
    MAIN_CAMERA_CONTROL_TYPE_AUTOPLAY,
    MAIN_CAMERA_CONTROL_TYPE_COUNT,
} ScAppMainCameraControlType;

static const char* SC_APP_MAIN_CAMERA_CONTROL_TYPE_NAME[] = {
    "Orbit",
    "Autoplay",
};

typedef enum ScAppCameraType {
    CAMERA_TYPE_MAIN,
    CAMERA_TYPE_AERIAL,
    CAMERA_TYPE_COUNT,
} ScAppCameraType;

typedef struct ScAppCameraCreateInfo {
    SDL_GPUDevice* device;
    SDL_GPUTextureFormat color_format;
    SDL_GPUTextureFormat depth_stencil_format;
} ScAppCameraCreateInfo;

typedef struct ScAppCamera {
    ScPerspectiveCamera camera;
    SDL_GPUViewport viewport;
    ScOctreeUniforms uniforms;
    ScDebugDraw ddraw;
} ScAppCamera;

static void sc_app_camera_new(ScAppCamera* camera, const ScAppCameraCreateInfo* create_info) {
    // Unpack.
    SDL_GPUDevice* device = create_info->device;
    const SDL_GPUTextureFormat color_format = create_info->color_format;
    const SDL_GPUTextureFormat depth_stencil_format = create_info->depth_stencil_format;

    // Debug draw.
    sc_ddraw_new(
        &camera->ddraw,
        device,
        &(ScDebugDrawCreateInfo) {
            .color_format = color_format,
            .depth_stencil_format = depth_stencil_format,
        }
    );
}

static void sc_app_camera_free(ScAppCamera* camera, SDL_GPUDevice* device) {
    sc_ddraw_free(&camera->ddraw, device);
}

//
// Stormcloud - App.
//

#define SC_WINDOW_WIDTH 1920
#define SC_WINDOW_HEIGHT 1200
#define SC_SWAPCHAIN_PRESENT_MODE SDL_GPU_PRESENTMODE_VSYNC
#define SC_SWAPCHAIN_COMPOSITION SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR
#define SC_SWAPCHAIN_COLOR_FORMAT SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB
#define SC_SWAPCHAIN_DEPTH_STENCIL_FORMAT SDL_GPU_TEXTUREFORMAT_D32_FLOAT

typedef struct ScAppParameters {
    float lod_bias;
    ScAppViewMode view_mode;
    ScAppMainCameraControlType main_camera_control_type;
} ScAppParameters;

typedef struct ScApp {
    // App.
    ScAppParameters parameters;
    ScOctree octree;
    ScAppCamera cameras[CAMERA_TYPE_COUNT];
    ScCameraControlOrbit orbit_control;
    ScCameraControlAutoplay autoplay_control;
    ScCameraControlAerial aerial_control;

    // Rendering state.
    SDL_Window* window;
    SDL_GPUDevice* device;
    SDL_GPUTexture* depth_stencil_texture;
    SDL_GPUBuffer* point_buffer;
    SDL_GPUBuffer* node_buffer;
    SDL_GPUBuffer* bounds_buffer;
    uint32_t bounds_vertex_count;
    SDL_GPUGraphicsPipeline* point_pipeline;
    SDL_GPUGraphicsPipeline* bounds_pipeline;

    // User interface.
    ScGui gui;

    // Frame statistics.
    uint32_t frame_index;
    uint64_t frame_time_ns;
    uint64_t frame_time_frequency;
} ScApp;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    // Arguments.
    SC_ASSERT(argc == 2);

    // Create app.
    ScApp* app = calloc(1, sizeof(ScApp));
    *appstate = app;

    // Parameters.
    app->parameters.lod_bias = 1.0f / 8.0f;
    app->parameters.view_mode = VIEW_MODE_SPLIT;
    app->parameters.main_camera_control_type = MAIN_CAMERA_CONTROL_TYPE_ORBIT;

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
    if (!SDL_WindowSupportsGPUPresentMode(app->device, app->window, SC_SWAPCHAIN_PRESENT_MODE)) {
        SC_LOG_ERROR("SDL_WindowSupportsGPUPresentMode failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_WindowSupportsGPUSwapchainComposition(
            app->device,
            app->window,
            SC_SWAPCHAIN_COMPOSITION
        )) {
        SC_LOG_ERROR("SDL_WindowSupportsGPUSwapchainComposition failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetGPUSwapchainParameters(
            app->device,
            app->window,
            SC_SWAPCHAIN_COMPOSITION,
            SC_SWAPCHAIN_PRESENT_MODE
        )) {
        SC_LOG_ERROR("SDL_SetGPUSwapchainParameters failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (SDL_GetGPUSwapchainTextureFormat(app->device, app->window) != SC_SWAPCHAIN_COLOR_FORMAT) {
        SC_LOG_ERROR("SDL_GetGPUSwapchainTextureFormat failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Depth stencil texture.
    {
        app->depth_stencil_texture = SDL_CreateGPUTexture(
            app->device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = SC_SWAPCHAIN_DEPTH_STENCIL_FORMAT,
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
        memcpy(data, app->octree.node_instances, node_byte_count);
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
        vec3f position;
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
        const vec3f mn = {0.0f, 0.0f, 0.0f};
        const vec3f mx = {1.0f, 1.0f, 1.0f};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mn.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mn.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mn.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mx.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mx.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mx.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mx.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mn.y, mn.z}, .color = 0xffffffff};

        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mn.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mn.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mn.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mx.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mx.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mx.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mx.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mn.y, mx.z}, .color = 0xffffffff};

        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mn.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mn.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mn.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mn.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mx.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mx.x, mx.y, mx.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mx.y, mn.z}, .color = 0xffffffff};
        *data++ = (ScVertex) {.position = (vec3f) {mn.x, mx.y, mx.z}, .color = 0xffffffff};
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
            .sampler_count = 0,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* point_fragment_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/point.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .sampler_count = 0,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* bounds_vertex_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/bounds.vert",
            .entry_point = "vs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .sampler_count = 0,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* bounds_fragment_shader = sc_gpu_shader_new(
        app->device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/bounds.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .sampler_count = 0,
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
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                .offset = 0,
                            },
                            {
                                .location = 3,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                .offset = 3 * sizeof(float),
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
                        .format = SC_SWAPCHAIN_COLOR_FORMAT,
                        .blend_state = (SDL_GPUColorTargetBlendState) {0},
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = SC_SWAPCHAIN_DEPTH_STENCIL_FORMAT,
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
                                .offset = sizeof(vec3f),
                            },
                            {
                                .location = 2,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                .offset = 0,
                            },
                            {
                                .location = 3,
                                .buffer_slot = 1,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                .offset = 3 * sizeof(float),
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
                        .format = SC_SWAPCHAIN_COLOR_FORMAT,
                        .blend_state = (SDL_GPUColorTargetBlendState) {0},
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = SC_SWAPCHAIN_DEPTH_STENCIL_FORMAT,
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

    // Cameras.
    for (uint32_t i = 0; i < CAMERA_TYPE_COUNT; i++) {
        sc_app_camera_new(
            &app->cameras[i],
            &(ScAppCameraCreateInfo) {
                .device = app->device,
                .color_format = SC_SWAPCHAIN_COLOR_FORMAT,
                .depth_stencil_format = SC_SWAPCHAIN_DEPTH_STENCIL_FORMAT,
            }
        );
    }

    // Camera controllers.
    {
        const ScCameraControlCommonCreateInfo common_create_info = {
            .scene_bounds = app->octree.point_bounds,
        };
        app->orbit_control = sc_camera_control_orbit_new(&(ScCameraControlOrbitCreateInfo) {
            .common = common_create_info,
        });
        app->autoplay_control =
            sc_camera_control_autoplay_new(&(ScCameraControlAutoplayCreateInfo) {
                .common = common_create_info,
            });
        app->aerial_control = sc_camera_control_aerial_new(&(ScCameraControlAerialCreateInfo) {
            .common = common_create_info,
        });
    }

    // Gui.
    sc_gui_new(
        &app->gui,
        &(ScGuiCreateInfo) {
            .window = app->window,
            .device = app->device,
            .color_format = SC_SWAPCHAIN_COLOR_FORMAT,
            .depth_stencil_format = SC_SWAPCHAIN_DEPTH_STENCIL_FORMAT,
        }
    );

    // Frame index.
    app->frame_index = 0;
    app->frame_time_ns = SDL_GetPerformanceCounter();
    app->frame_time_frequency = SDL_GetPerformanceFrequency();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    // Unpack.
    ScApp* app = (ScApp*)appstate;

    // Exit-events.
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
    }

    // Camera controllers.
    switch (app->parameters.main_camera_control_type) {
        case MAIN_CAMERA_CONTROL_TYPE_ORBIT:
            sc_camera_control_orbit_event(&app->orbit_control, event);
            break;
        case MAIN_CAMERA_CONTROL_TYPE_AUTOPLAY:
            sc_camera_control_autoplay_event(&app->autoplay_control, event);
            break;
        default: break;
    }

    // Gui.
    sc_gui_event(&app->gui, event);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    // Unpack.
    ScApp* app = (ScApp*)appstate;
    ScAppCamera* main_camera = &app->cameras[CAMERA_TYPE_MAIN];
    ScAppCamera* aerial_camera = &app->cameras[CAMERA_TYPE_AERIAL];

    // Timing.
    const uint64_t frame_time_ns = SDL_GetPerformanceCounter();
    const uint64_t frame_time_elapsed_ns = frame_time_ns - app->frame_time_ns;
    app->frame_time_ns = frame_time_ns;
    const float delta_time =
        (float)((double)frame_time_elapsed_ns / (double)app->frame_time_frequency);

    // Camera - pre-traversal update.
    {
        // Common.
        float screen_width = 0.0f;
        float screen_height = 0.0f;
        switch (app->parameters.view_mode) {
            case VIEW_MODE_FULLSCREEN:
                screen_width = (float)SC_WINDOW_WIDTH;
                screen_height = (float)SC_WINDOW_HEIGHT;
                break;
            case VIEW_MODE_SPLIT:
                screen_width = 0.5f * (float)SC_WINDOW_WIDTH;
                screen_height = (float)SC_WINDOW_HEIGHT;
                break;
        }

        // Camera controls.
        const ScCameraControlCommonUpdateInfo common_update_info = {
            .screen_width = screen_width,
            .screen_height = screen_height,
            .field_of_view = rad_from_deg(60.0f),
            .clip_distance_near = 16.0f,
            .clip_distance_far = 2048.0f,
            .delta_time = delta_time,
            .input_captured = ImGui_GetIO()->WantCaptureMouse,
        };
        switch (app->parameters.main_camera_control_type) {
            case MAIN_CAMERA_CONTROL_TYPE_ORBIT:
                sc_camera_control_orbit_update(
                    &app->orbit_control,
                    &(ScCameraControlOrbitUpdateInfo) {
                        .common = common_update_info,
                    },
                    &main_camera->camera
                );
                break;
            case MAIN_CAMERA_CONTROL_TYPE_AUTOPLAY:
                sc_camera_control_autoplay_update(
                    &app->autoplay_control,
                    &(ScCameraControlAutoplayUpdateInfo) {
                        .common = common_update_info,
                    },
                    &main_camera->camera
                );
                break;
            default: break;
        }
        sc_camera_control_aerial_update(
            &app->aerial_control,
            &(ScCameraControlAerialUpdateInfo) {
                .common = common_update_info,
                .world_target = box3f_center(app->octree.point_bounds),
            },
            &aerial_camera->camera
        );

        // Viewports.
        main_camera->viewport = (SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = screen_width,
            .h = screen_height,
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        };
        aerial_camera->viewport = (SDL_GPUViewport) {
            .x = app->parameters.view_mode == VIEW_MODE_SPLIT ? screen_width : 0.0f,
            .y = 0.0f,
            .w = screen_width,
            .h = screen_height,
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        };

        // Uniforms.
        main_camera->uniforms = (ScOctreeUniforms) {
            .clip_from_world = main_camera->camera.clip_from_world,
            .node_world_scale = app->octree.node_world_scale,
        };
        aerial_camera->uniforms = (ScOctreeUniforms) {
            .clip_from_world = aerial_camera->camera.clip_from_world,
            .node_world_scale = app->octree.node_world_scale,
        };

        // Debug.
        ScDebugDraw* main_ddraw = &main_camera->ddraw;
        const ScFrustum* main_frustum = &main_camera->camera.frustum;
        const vec3f main_camera_position = main_camera->camera.world_position;
        sc_ddraw_box(main_ddraw, app->octree.point_bounds, 0xffffffff);

        if (app->parameters.view_mode == VIEW_MODE_SPLIT) {
            // clang-format off
            ScDebugDraw* aerial_ddraw = &aerial_camera->ddraw;
            sc_ddraw_line(aerial_ddraw, main_camera_position, vec3f_add(main_camera_position, vec3f_scale(main_camera->camera.world_right, 50.0f)), 0xff0000ff);
            sc_ddraw_line(aerial_ddraw, main_camera_position, vec3f_add(main_camera_position, vec3f_scale(main_camera->camera.world_up, 50.0f)), 0xff00ff00);
            sc_ddraw_line(aerial_ddraw, main_camera_position, vec3f_add(main_camera_position, vec3f_scale(main_camera->camera.world_forward, 50.0f)), 0xffff0000);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LBN], main_frustum->corners[SC_FRUSTUM_CORNER_LBF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_RBN], main_frustum->corners[SC_FRUSTUM_CORNER_RBF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LTN], main_frustum->corners[SC_FRUSTUM_CORNER_LTF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_RTN], main_frustum->corners[SC_FRUSTUM_CORNER_RTF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LBN], main_frustum->corners[SC_FRUSTUM_CORNER_RBN], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LTN], main_frustum->corners[SC_FRUSTUM_CORNER_RTN], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LBN], main_frustum->corners[SC_FRUSTUM_CORNER_LTN], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_RBN], main_frustum->corners[SC_FRUSTUM_CORNER_RTN], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LBF], main_frustum->corners[SC_FRUSTUM_CORNER_RBF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LTF], main_frustum->corners[SC_FRUSTUM_CORNER_RTF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_LBF], main_frustum->corners[SC_FRUSTUM_CORNER_LTF], 0xff808080);
            sc_ddraw_line(aerial_ddraw, main_frustum->corners[SC_FRUSTUM_CORNER_RBF], main_frustum->corners[SC_FRUSTUM_CORNER_RTF], 0xff808080);
            // clang-format on
        }
    }

    // Octree - traversal.
    sc_octree_traverse(
        &app->octree,
        &(ScOctreeTraverseInfo) {
            .camera = &main_camera->camera,
            .lod_bias = app->parameters.lod_bias,
        }
    );

    // Gui - begin.
    sc_gui_frame_begin(&app->gui);

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

    // Draw view.
    switch (app->parameters.view_mode) {
        case VIEW_MODE_FULLSCREEN: {
            // Points.
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
            SDL_SetGPUViewport(render_pass, &main_camera->viewport);
            SDL_PushGPUVertexUniformData(cmd, 0, &main_camera->uniforms, sizeof(ScOctreeUniforms));
            for (uint32_t i = 0; i < app->octree.node_traverse_count; i++) {
                const uint32_t node_idx = app->octree.node_traverse[i];
                const ScOctreeNode* node = &app->octree.nodes[node_idx];
                const uint32_t vertex_count = (uint32_t)node->point_count;
                const uint32_t vertex_offset = (uint32_t)node->point_offset;
                SDL_DrawGPUPrimitives(render_pass, vertex_count, 1, vertex_offset, node_idx);
            }

            // Debug.
            sc_ddraw_render(
                &main_camera->ddraw,
                &(ScDebugRenderInfo) {
                    .device = app->device,
                    .command_buffer = cmd,
                    .render_pass = render_pass,
                    .viewport = main_camera->viewport,
                    .clip_from_world = main_camera->camera.clip_from_world,
                    .frame_index = app->frame_index,
                }
            );
            break;
        }

        case VIEW_MODE_SPLIT: {
            // Points.
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
            for (uint32_t i = 0; i < CAMERA_TYPE_COUNT; i++) {
                ScAppCamera* camera = &app->cameras[i];
                SDL_SetGPUViewport(render_pass, &camera->viewport);
                SDL_PushGPUVertexUniformData(cmd, 0, &camera->uniforms, sizeof(ScOctreeUniforms));
                for (uint32_t j = 0; j < app->octree.node_traverse_count; j++) {
                    const uint32_t node_idx = app->octree.node_traverse[j];
                    const ScOctreeNode* node = &app->octree.nodes[node_idx];
                    const uint32_t vertex_count = (uint32_t)node->point_count;
                    const uint32_t vertex_offset = (uint32_t)node->point_offset;
                    SDL_DrawGPUPrimitives(render_pass, vertex_count, 1, vertex_offset, node_idx);
                }
            }

            // Nodes.
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
            SDL_SetGPUViewport(render_pass, &aerial_camera->viewport);
            SDL_PushGPUVertexUniformData(
                cmd,
                0,
                &aerial_camera->uniforms,
                sizeof(ScOctreeUniforms)
            );
            for (uint32_t i = 0; i < app->octree.node_traverse_count; i++) {
                const uint32_t node_idx = app->octree.node_traverse[i];
                SDL_DrawGPUPrimitives(render_pass, app->bounds_vertex_count, 1, 0, node_idx);
            }

            // Debug.
            for (uint32_t i = 0; i < CAMERA_TYPE_COUNT; i++) {
                ScAppCamera* camera = &app->cameras[i];
                sc_ddraw_render(
                    &camera->ddraw,
                    &(ScDebugRenderInfo) {
                        .device = app->device,
                        .command_buffer = cmd,
                        .render_pass = render_pass,
                        .viewport = camera->viewport,
                        .clip_from_world = camera->camera.clip_from_world,
                        .frame_index = app->frame_index,
                    }
                );
            }

            break;
        }

        default: break;
    }

    // Gui.
    {
        uint64_t visible_point_count = 0;
        for (uint32_t i = 0; i < app->octree.node_traverse_count; i++) {
            const uint32_t node_idx = app->octree.node_traverse[i];
            const ScOctreeNode* node = &app->octree.nodes[node_idx];
            visible_point_count += node->point_count;
        }
        const float visible_mpoint_count = (float)visible_point_count / 1e6f;

        ImGui_SetNextWindowSize((ImVec2) {240.0f, 300.0f}, ImGuiCond_Once);
        ImGui_Begin("stormcloud", NULL, 0);
        ImGui_Text("octree_points: %u", app->octree.point_count);
        ImGui_Text("octree_nodes: %u", app->octree.node_count);
        ImGui_Text("traversed_nodes: %u", app->octree.node_traverse_count);
        ImGui_Text("visible_points: %u (%.2fM)", visible_point_count, visible_mpoint_count);
        ImGui_SliderFloat("lod_bias", &app->parameters.lod_bias, 0.0f, 1.0f);
        ImGui_ComboChar(
            "view_mode",
            (int32_t*)&app->parameters.view_mode,
            SC_APP_VIEW_MODE_NAME,
            SC_COUNTOF(SC_APP_VIEW_MODE_NAME)
        );
        ImGui_ComboChar(
            "camera_control",
            (int32_t*)&app->parameters.main_camera_control_type,
            SC_APP_MAIN_CAMERA_CONTROL_TYPE_NAME,
            SC_COUNTOF(SC_APP_MAIN_CAMERA_CONTROL_TYPE_NAME)
        );
        ImGui_End();
    }

    // Gui - end.
    sc_gui_frame_end(
        &app->gui,
        &(ScGuiRenderInfo) {
            .device = app->device,
            .command_buffer = cmd,
            .render_pass = render_pass,
            .frame_index = app->frame_index,
        }
    );

    // Render pass - end.
    SDL_EndGPURenderPass(render_pass);

    // Submit.
    SDL_SubmitGPUCommandBuffer(cmd);

    // Frame index.
    app->frame_index = (app->frame_index + 1) % SC_INFLIGHT_FRAME_COUNT;

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
    for (uint32_t i = 0; i < CAMERA_TYPE_COUNT; i++) {
        sc_app_camera_free(&app->cameras[i], app->device);
    }
    sc_gui_free(&app->gui, app->device);
    SDL_ReleaseWindowFromGPUDevice(app->device, app->window);
    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->device);

    // Free.
    sc_octree_free(&app->octree);
    free(app);

    // End.
    SC_ASSERT(result == SDL_APP_SUCCESS);
}
