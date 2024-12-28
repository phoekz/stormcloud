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
