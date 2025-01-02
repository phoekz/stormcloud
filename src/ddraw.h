typedef struct ScDebugDrawVertex {
    vec3f position;
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
    mat4f clip_from_world;
    uint32_t frame_index;
} ScDebugRenderInfo;

typedef struct ScDebugDraw {
    uint32_t line_count;
    uint32_t line_capacity;
    uint32_t line_byte_count;
    ScDebugDrawVertex* lines;

    SDL_GPUTransferBuffer* line_transfer_buffers[SC_INFLIGHT_FRAME_COUNT];
    SDL_GPUBuffer* line_buffers[SC_INFLIGHT_FRAME_COUNT];

    SDL_GPUGraphicsPipeline* line_pipeline;
} ScDebugDraw;

static void
sc_ddraw_new(ScDebugDraw* ddraw, SDL_GPUDevice* device, const ScDebugDrawCreateInfo* create_info) {
    // Buffers.
    ddraw->line_count = 0;
    ddraw->line_capacity = 1024;
    ddraw->line_byte_count = ddraw->line_capacity * sizeof(ScDebugDrawVertex);
    ddraw->lines = malloc(ddraw->line_byte_count);
    for (uint32_t i = 0; i < SC_INFLIGHT_FRAME_COUNT; i++) {
        ddraw->line_transfer_buffers[i] = SDL_CreateGPUTransferBuffer(
            device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = ddraw->line_byte_count,
            }
        );
        ddraw->line_buffers[i] = SDL_CreateGPUBuffer(
            device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = ddraw->line_byte_count,
            }
        );
    }

    // Shaders.
    SDL_GPUShader* vertex_shader = sc_gpu_shader_new(
        device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/ddraw.vert",
            .entry_point = "vs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .sampler_count = 0,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* fragment_shader = sc_gpu_shader_new(
        device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/ddraw.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .sampler_count = 0,
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
                                .offset = sizeof(vec3f),
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

static void sc_ddraw_free(ScDebugDraw* ddraw, SDL_GPUDevice* device) {
    SDL_ReleaseGPUGraphicsPipeline(device, ddraw->line_pipeline);
    for (uint32_t i = 0; i < SC_INFLIGHT_FRAME_COUNT; i++) {
        SDL_ReleaseGPUBuffer(device, ddraw->line_buffers[i]);
        SDL_ReleaseGPUTransferBuffer(device, ddraw->line_transfer_buffers[i]);
    }
    free(ddraw->lines);
}

static void sc_ddraw_line(ScDebugDraw* ddraw, vec3f a, vec3f b, uint32_t color) {
    // Validation.
    SC_ASSERT(ddraw->line_count + 2 <= ddraw->line_capacity);

    // Write.
    ScDebugDrawVertex* vertices = &ddraw->lines[ddraw->line_count];
    vertices[0] = (ScDebugDrawVertex) {.position = a, .color = color};
    vertices[1] = (ScDebugDrawVertex) {.position = b, .color = color};
    ddraw->line_count += 2;
}

static void sc_ddraw_box(ScDebugDraw* ddraw, box3f box, uint32_t color) {
    // Validation.
    SC_ASSERT(ddraw->line_count + 24 <= ddraw->line_capacity);

    // Corners.
    const vec3f c[8] = {
        {box.mn.x, box.mn.y, box.mn.z},
        {box.mx.x, box.mn.y, box.mn.z},
        {box.mn.x, box.mx.y, box.mn.z},
        {box.mx.x, box.mx.y, box.mn.z},
        {box.mn.x, box.mn.y, box.mx.z},
        {box.mx.x, box.mn.y, box.mx.z},
        {box.mn.x, box.mx.y, box.mx.z},
        {box.mx.x, box.mx.y, box.mx.z},
    };

    // Write.
    ScDebugDrawVertex* vertices = &ddraw->lines[ddraw->line_count];
    vertices[0] = (ScDebugDrawVertex) {.position = c[0], .color = color};
    vertices[1] = (ScDebugDrawVertex) {.position = c[1], .color = color};
    vertices[2] = (ScDebugDrawVertex) {.position = c[1], .color = color};
    vertices[3] = (ScDebugDrawVertex) {.position = c[3], .color = color};
    vertices[4] = (ScDebugDrawVertex) {.position = c[3], .color = color};
    vertices[5] = (ScDebugDrawVertex) {.position = c[2], .color = color};
    vertices[6] = (ScDebugDrawVertex) {.position = c[2], .color = color};
    vertices[7] = (ScDebugDrawVertex) {.position = c[0], .color = color};

    vertices[8] = (ScDebugDrawVertex) {.position = c[4], .color = color};
    vertices[9] = (ScDebugDrawVertex) {.position = c[5], .color = color};
    vertices[10] = (ScDebugDrawVertex) {.position = c[5], .color = color};
    vertices[11] = (ScDebugDrawVertex) {.position = c[7], .color = color};
    vertices[12] = (ScDebugDrawVertex) {.position = c[7], .color = color};
    vertices[13] = (ScDebugDrawVertex) {.position = c[6], .color = color};
    vertices[14] = (ScDebugDrawVertex) {.position = c[6], .color = color};
    vertices[15] = (ScDebugDrawVertex) {.position = c[4], .color = color};

    vertices[16] = (ScDebugDrawVertex) {.position = c[0], .color = color};
    vertices[17] = (ScDebugDrawVertex) {.position = c[4], .color = color};
    vertices[18] = (ScDebugDrawVertex) {.position = c[1], .color = color};
    vertices[19] = (ScDebugDrawVertex) {.position = c[5], .color = color};
    vertices[20] = (ScDebugDrawVertex) {.position = c[2], .color = color};
    vertices[21] = (ScDebugDrawVertex) {.position = c[6], .color = color};
    vertices[22] = (ScDebugDrawVertex) {.position = c[3], .color = color};
    vertices[23] = (ScDebugDrawVertex) {.position = c[7], .color = color};
    ddraw->line_count += 24;
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
    const uint32_t frame_index = render_info->frame_index;
    SDL_GPUTransferBuffer* line_transfer_buffer = ddraw->line_transfer_buffers[frame_index];
    SDL_GPUBuffer* line_buffer = ddraw->line_buffers[frame_index];

    // Copy to device.
    const uint32_t line_byte_count = ddraw->line_count * sizeof(ScDebugDrawVertex);
    ScDebugDrawVertex* dst = SDL_MapGPUTransferBuffer(device, line_transfer_buffer, false);
    memcpy(dst, ddraw->lines, line_byte_count);
    SDL_UnmapGPUTransferBuffer(device, line_transfer_buffer);

    // Device commands.
    SDL_GPUCommandBuffer* upload_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_buffer);
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = line_transfer_buffer,
            .offset = 0,
        },
        &(SDL_GPUBufferRegion) {
            .buffer = line_buffer,
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
                .buffer = line_buffer,
                .offset = 0,
            },
        },
        1
    );
    SDL_SetGPUViewport(render_pass, &viewport);
    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &render_info->clip_from_world,
        sizeof(render_info->clip_from_world)
    );
    SDL_DrawGPUPrimitives(render_pass, ddraw->line_count, 1, 0, 0);

    // Reset.
    ddraw->line_count = 0;
}
