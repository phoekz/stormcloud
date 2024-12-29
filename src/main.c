//
// Stormcloud - Includes.
//

#include "common.h"
#include "math.h"
#include "camera.h"
#include "octree.h"
#include "gpu.h"
#include "ddraw.h"

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
    if (!SDL_WindowSupportsGPUSwapchainComposition(app->device, app->window, composition)) {
        SC_LOG_ERROR("SDL_WindowSupportsGPUSwapchainComposition failed: %s", SDL_GetError());
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
                                .offset = sizeof(vec3f),
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
        AERIAL,
        COUNT,
    };
    SDL_GPUViewport viewports[COUNT];
    mat4f transforms[COUNT];
    mat4f inverse_transforms[COUNT];
    mat4f projection_transforms[COUNT];
    mat4f view_transforms[COUNT];
    vec3f camera_positions[COUNT];
    float camera_fov = rad_from_deg(60.0f);
    const float window_width = (float)(SC_WINDOW_WIDTH / 2);
    const float window_height = (float)SC_WINDOW_HEIGHT;
    {
        // Common.
        const float time = (float)(SDL_GetTicks() / 1000.0);
        const float aspect = window_width / window_height;
        const float z_near = 0.1f;
        const float z_far = 1000.0f;
        const vec3f point_origin = bounds3f_center(app->octree.point_bounds);

        // Main.
        {
            const float camera_turn_speed = 0.25f;
            const float camera_offset_radius =
                1.0f + 500.0f * (0.5f + 0.5f * SDL_cosf(0.5f * time));
            const vec3f camera_offset = (vec3f) {
                camera_offset_radius * SDL_cosf(camera_turn_speed * time),
                camera_offset_radius * SDL_sinf(camera_turn_speed * time),
                camera_offset_radius * 0.5f,
            };
            const vec3f camera_origin = vec3f_add(point_origin, camera_offset);
            const vec3f camera_target = point_origin;
            const vec3f camera_up = (vec3f) {0.0f, 0.0f, 1.0f};
            const mat4f projection = mat4f_perspective(camera_fov, aspect, z_near, z_far);
            const mat4f view = mat4f_lookat(camera_origin, camera_target, camera_up);
            transforms[MAIN] = mat4f_mul(projection, view);
            inverse_transforms[MAIN] = mat4f_inverse(transforms[MAIN]);
            projection_transforms[MAIN] = projection;
            view_transforms[MAIN] = view;
            camera_positions[MAIN] = camera_origin;
            viewports[MAIN] = (SDL_GPUViewport) {
                .x = 0.0f,
                .y = 0.0f,
                .w = window_width,
                .h = window_height,
                .min_depth = 0.0f,
                .max_depth = 1.0f,
            };

            sc_ddraw_line(&app->ddraw, camera_origin, camera_target, 0xff0000ff);

            vec3f frustum_points[5] = {
                {0.0f, 0.0f, 0.0f},
                {-1.0f, -1.0f, 1.0f},
                {1.0f, -1.0f, 1.0f},
                {1.0f, 1.0f, 1.0f},
                {-1.0f, 1.0f, 1.0f},
            };
            for (uint32_t i = 0; i < SC_COUNTOF(frustum_points); i++) {
                const vec4f v = vec4f_from_vec3f(frustum_points[i], 1.0f);
                const vec4f r = mat4f_mul_vec4f(inverse_transforms[MAIN], v);
                const vec4f h = vec4f_scale(r, 1.0f / r.w);
                frustum_points[i] = vec3f_from_vec4f(h);
            }
            sc_ddraw_line(&app->ddraw, frustum_points[0], frustum_points[1], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[0], frustum_points[2], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[0], frustum_points[3], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[0], frustum_points[4], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[1], frustum_points[2], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[2], frustum_points[3], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[3], frustum_points[4], 0xff00ff00);
            sc_ddraw_line(&app->ddraw, frustum_points[4], frustum_points[1], 0xff00ff00);
        }

        // Aerial.
        {
            const vec3f camera_offset = (vec3f) {0.0f, 0.0f, 1000.0f};
            const vec3f camera_origin = vec3f_add(point_origin, camera_offset);
            const vec3f camera_target = point_origin;
            const vec3f camera_up = (vec3f) {0.0f, 1.0f, 0.0f};
            const mat4f projection = mat4f_perspective(camera_fov, aspect, z_near, z_far);
            const mat4f view = mat4f_lookat(camera_origin, camera_target, camera_up);
            transforms[AERIAL] = mat4f_mul(projection, view);
            inverse_transforms[AERIAL] = mat4f_inverse(transforms[AERIAL]);
            projection_transforms[AERIAL] = projection;
            view_transforms[AERIAL] = view;
            camera_positions[AERIAL] = camera_origin;
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

    // Traverse.
    sc_octree_traverse(
        &app->octree,
        &(ScOctreeTraverseInfo) {
            .projection = projection_transforms[MAIN],
            .view = view_transforms[MAIN],
            .projection_view = transforms[MAIN],
            .inverse_projection_view = inverse_transforms[MAIN],
            .camera_position = camera_positions[MAIN],
            .focal_length = 1.0f / SDL_tanf(camera_fov * 0.5f),
            .window_width = window_width,
            .window_height = window_height,
        }
    );

    // Uniforms.
    const ScOctreeUniforms uniforms[COUNT] = {
        {
            .projection_view = transforms[MAIN],
            .node_world_scale = app->octree.node_world_scale,
        },
        {
            .projection_view = transforms[AERIAL],
            .node_world_scale = app->octree.node_world_scale,
        },
    };

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

        for (uint32_t camera_idx = 0; camera_idx < COUNT; camera_idx++) {
            SDL_SetGPUViewport(render_pass, &viewports[camera_idx]);
            SDL_PushGPUVertexUniformData(cmd, 0, &uniforms[camera_idx], sizeof(ScOctreeUniforms));
            for (uint32_t i = 0; i < app->octree.node_traverse_count; i++) {
                const uint32_t node_idx = app->octree.node_traverse[i];
                const ScOctreeNode* node = &app->octree.nodes[node_idx];
                const uint32_t vertex_count = (uint32_t)node->point_count;
                const uint32_t vertex_offset = (uint32_t)node->point_offset;
                SDL_DrawGPUPrimitives(render_pass, vertex_count, 1, vertex_offset, node_idx);
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

        for (uint32_t camera_idx = 0; camera_idx < COUNT; camera_idx++) {
            if (camera_idx != AERIAL) {
                continue;
            }
            SDL_SetGPUViewport(render_pass, &viewports[camera_idx]);
            SDL_PushGPUVertexUniformData(cmd, 0, &uniforms[camera_idx], sizeof(ScOctreeUniforms));
            for (uint32_t i = 0; i < app->octree.node_traverse_count; i++) {
                const uint32_t node_idx = app->octree.node_traverse[i];
                SDL_DrawGPUPrimitives(render_pass, app->bounds_vertex_count, 1, 0, node_idx);
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
