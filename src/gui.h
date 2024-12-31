typedef struct ScGuiVertex {
    vec2f position;
    vec2f texcoord;
    uint32_t color;
} ScGuiVertex;

static_assert(sizeof(ScGuiVertex) == sizeof(ImDrawVert), "ScGuiVertex size mismatch");

typedef struct ScGuiIndex {
    uint16_t index;
} ScGuiIndex;

static_assert(sizeof(ScGuiIndex) == sizeof(ImDrawIdx), "ScGuiIndex size mismatch");

typedef struct ScGuiUniforms {
    vec2f scale;
    vec2f offset;
} ScGuiUniforms;

typedef struct ScGuiCreateInfo {
    SDL_Window* window;
    SDL_GPUDevice* device;
    SDL_GPUTextureFormat color_format;
    SDL_GPUTextureFormat depth_stencil_format;
} ScGuiCreateInfo;

typedef struct ScGui {
    ImGuiContext* context;
    SDL_Window* window;
    char* clipboard_text;
    uint64_t performance_frequency;
    uint64_t performance_counter;
    uint32_t frame_index;
    SDL_GPUTransferBuffer* transfer_buffers[SC_INFLIGHT_FRAME_COUNT];
    SDL_GPUBuffer* vertex_buffers[SC_INFLIGHT_FRAME_COUNT];
    SDL_GPUBuffer* index_buffers[SC_INFLIGHT_FRAME_COUNT];
    uint32_t vertex_buffer_capacity;
    uint32_t index_buffer_capacity;
    ScGuiVertex* vertex_data;
    ScGuiIndex* index_data;
    SDL_GPUTexture* font_texture;
    SDL_GPUSampler* font_sampler;
    SDL_GPUGraphicsPipeline* pipeline;
} ScGui;

static void sc_gui_imgui_set_ime_data_callback(
    ImGuiContext* context,
    ImGuiViewport* viewport,
    ImGuiPlatformImeData* data
) {
    // Unpack.
    SC_UNUSED(context);
    SC_UNUSED(viewport);
    ImGuiIO* io = ImGui_GetIO();
    ScGui* gui = io->BackendPlatformUserData;
    SDL_Window* window = gui->window;

    // Text input.
    if (data->WantVisible) {
        SDL_SetTextInputArea(
            window,
            &(SDL_Rect) {
                .x = (int32_t)data->InputPos.x,
                .y = (int32_t)data->InputPos.y,
                .w = 1,
                .h = (int32_t)data->InputLineHeight,
            },
            0
        );
        SDL_StartTextInput(window);
    } else {
        SDL_StopTextInput(window);
    }
}

static const char* sc_gui_imgui_get_clipboard_text_callback(ImGuiContext* context) {
    // Unpack.
    SC_UNUSED(context);
    ImGuiIO* io = ImGui_GetIO();
    ScGui* gui = io->BackendPlatformUserData;

    // Get clipboard text.
    if (gui->clipboard_text != NULL) {
        free(gui->clipboard_text);
    }
    const char* text = SDL_GetClipboardText();
    const uint32_t text_length = (uint32_t)strlen(text);
    gui->clipboard_text = malloc(text_length + 1);
    memcpy(gui->clipboard_text, text, text_length);
    gui->clipboard_text[text_length] = '\0';
    return gui->clipboard_text;
}

static void sc_gui_imgui_set_clipboard_text_callback(ImGuiContext* context, const char* text) {
    // Unpack.
    SC_UNUSED(context);

    // Set clipboard text.
    SDL_SetClipboardText(text);
}

static void sc_gui_new(ScGui* gui, const ScGuiCreateInfo* create_info) {
    // Unpack.
    SDL_Window* window = create_info->window;
    SDL_GPUDevice* device = create_info->device;
    SDL_GPUTextureFormat color_format = create_info->color_format;
    SDL_GPUTextureFormat depth_stencil_format = create_info->depth_stencil_format;

    // ImGui setup.
    gui->context = ImGui_CreateContext(NULL);
    gui->window = window;
    ImGui_StyleColorsDark(NULL);
    ImGuiIO* io = ImGui_GetIO();
    ImGuiPlatformIO* platform_io = ImGui_GetPlatformIO();
    io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io->BackendPlatformUserData = gui;
    platform_io->Platform_GetClipboardTextFn = sc_gui_imgui_get_clipboard_text_callback;
    platform_io->Platform_SetClipboardTextFn = sc_gui_imgui_set_clipboard_text_callback;
    gui->clipboard_text = NULL;
    platform_io->Platform_SetImeDataFn = sc_gui_imgui_set_ime_data_callback;

    // Timing.
    gui->performance_frequency = SDL_GetPerformanceFrequency();
    gui->performance_counter = SDL_GetPerformanceCounter();
    gui->frame_index = 0;

    // Buffers.
    gui->vertex_buffer_capacity = 1 << 12;
    gui->index_buffer_capacity = 1 << 12;
    const uint32_t vertex_byte_count = gui->vertex_buffer_capacity * sizeof(ScGuiVertex);
    const uint32_t index_byte_count = gui->index_buffer_capacity * sizeof(ScGuiIndex);
    const uint32_t total_byte_count = vertex_byte_count + index_byte_count;
    gui->vertex_data = malloc(vertex_byte_count);
    gui->index_data = malloc(index_byte_count);
    for (uint32_t i = 0; i < 2; i++) {
        gui->vertex_buffers[i] = SDL_CreateGPUBuffer(
            device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = vertex_byte_count,
            }
        );
        gui->index_buffers[i] = SDL_CreateGPUBuffer(
            device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                .size = index_byte_count,
            }
        );
        gui->transfer_buffers[i] = SDL_CreateGPUTransferBuffer(
            device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = total_byte_count,
            }
        );
    }

    // Font texture.
    {
        // Get data.
        uint8_t* pixels;
        int32_t width;
        int32_t height;
        int32_t bytes_per_pixel;
        ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, &bytes_per_pixel);
        ImFontAtlas_SetTexID(io->Fonts, ~0u);

        // Texture.
        gui->font_texture = SDL_CreateGPUTexture(
            device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                .width = (uint32_t)width,
                .height = (uint32_t)height,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            }
        );

        // Transfer.
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = width * height * bytes_per_pixel,
            }
        );
        void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(data, pixels, width * height * bytes_per_pixel);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
        SDL_GPUCommandBuffer* upload_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_buffer);
        SDL_UploadToGPUTexture(
            copy_pass,
            &(SDL_GPUTextureTransferInfo) {
                .transfer_buffer = transfer_buffer,
                .offset = 0,
            },
            &(SDL_GPUTextureRegion) {
                .texture = gui->font_texture,
                .w = (uint32_t)width,
                .h = (uint32_t)height,
                .d = 1,
            },
            false
        );
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(upload_buffer);

        // Release.
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

        // Sampler.
        gui->font_sampler = SDL_CreateGPUSampler(
            device,
            &(SDL_GPUSamplerCreateInfo) {
                .min_filter = SDL_GPU_FILTER_LINEAR,
                .mag_filter = SDL_GPU_FILTER_LINEAR,
                .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
                .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                .mip_lod_bias = 0.0f,
                .max_anisotropy = 0.0f,
                .compare_op = SDL_GPU_COMPAREOP_INVALID,
                .min_lod = 0.0f,
                .max_lod = 0.0f,
                .enable_anisotropy = false,
                .enable_compare = false,
            }
        );
    }

    // Shaders.
    SDL_GPUShader* vertex_shader = sc_gpu_shader_new(
        device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/gui.vert",
            .entry_point = "vs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .sampler_count = 0,
            .uniform_buffer_count = 1,
        }
    );
    SDL_GPUShader* fragment_shader = sc_gpu_shader_new(
        device,
        &(ScGpuShaderCreateInfo) {
            .file_path = "src/shaders/dxil/gui.frag",
            .entry_point = "fs_main",
            .shader_stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .sampler_count = 1,
            .uniform_buffer_count = 0,
        }
    );

    // Pipeline.
    gui->pipeline = SDL_CreateGPUGraphicsPipeline(
        device,
        &(SDL_GPUGraphicsPipelineCreateInfo) {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState) {
                    .vertex_buffer_descriptions =
                        &(SDL_GPUVertexBufferDescription) {
                            .slot = 0,
                            .pitch = sizeof(ScGuiVertex),
                            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                            .instance_step_rate = 0,

                        },
                    .num_vertex_buffers = 1,
                    .vertex_attributes =
                        (SDL_GPUVertexAttribute[]) {
                            {
                                .location = 0,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                .offset = 0,
                            },
                            {
                                .location = 1,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                .offset = sizeof(vec2f),
                            },
                            {
                                .location = 2,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                                .offset = sizeof(vec2f) + sizeof(vec2f),
                            },
                        },
                    .num_vertex_attributes = 3,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .rasterizer_state =
                (SDL_GPURasterizerState) {
                    .fill_mode = SDL_GPU_FILLMODE_FILL,
                    .cull_mode = SDL_GPU_CULLMODE_NONE,
                    .front_face = SDL_GPU_FRONTFACE_CLOCKWISE,
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
                    .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
                    .back_stencil_state = (SDL_GPUStencilOpState) {0},
                    .front_stencil_state = (SDL_GPUStencilOpState) {0},
                    .compare_mask = 0,
                    .write_mask = 0,
                    .enable_depth_test = false,
                    .enable_depth_write = false,
                    .enable_stencil_test = false,
                },
            .target_info =
                (SDL_GPUGraphicsPipelineTargetInfo) {
                    .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
                        .format = color_format,
                        .blend_state =
                            (SDL_GPUColorTargetBlendState) {
                                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                                .color_write_mask = 0,
                                .enable_blend = true,
                                .enable_color_write_mask = 0,
                            },
                    }},
                    .num_color_targets = 1,
                    .depth_stencil_format = depth_stencil_format,
                    .has_depth_stencil_target = true,
                },
        }
    );

    // Release.
    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);
}

static void sc_gui_free(ScGui* gui, SDL_GPUDevice* device) {
    // GPU resources.
    SDL_ReleaseGPUGraphicsPipeline(device, gui->pipeline);
    SDL_ReleaseGPUTexture(device, gui->font_texture);
    SDL_ReleaseGPUSampler(device, gui->font_sampler);
    for (uint32_t i = 0; i < 2; i++) {
        SDL_ReleaseGPUBuffer(device, gui->vertex_buffers[i]);
        SDL_ReleaseGPUBuffer(device, gui->index_buffers[i]);
        SDL_ReleaseGPUTransferBuffer(device, gui->transfer_buffers[i]);
    }
    free(gui->vertex_data);
    free(gui->index_data);

    // ImGui.
    if (gui->clipboard_text != NULL) {
        free(gui->clipboard_text);
    }
    ImGuiIO* io = ImGui_GetIO();
    io->BackendPlatformUserData = NULL;
    ImGui_DestroyContext(gui->context);
}

static void sc_gui_event(ScGui* gui, const SDL_Event* event) {
    // Unpack.
    ImGuiIO* io = ImGui_GetIO();
    SC_UNUSED(gui);

    // Handle.
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION: {
            ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
            ImGuiIO_AddMousePosEvent(io, (float)event->motion.x, (float)event->motion.y);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            uint32_t mouse_button = ~0u;
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: mouse_button = ImGuiMouseButton_Left; break;
                case SDL_BUTTON_RIGHT: mouse_button = ImGuiMouseButton_Right; break;
                case SDL_BUTTON_MIDDLE: mouse_button = ImGuiMouseButton_Middle; break;
                default: break;
            }
            if (mouse_button == ~0u) {
                break;
            }
            ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
            ImGuiIO_AddMouseButtonEvent(
                io,
                mouse_button,
                event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
            );
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            ImGuiIO_AddMouseSourceEvent(io, ImGuiMouseSource_Mouse);
            ImGuiIO_AddMouseWheelEvent(io, -event->wheel.x, event->wheel.y);
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            ImGuiIO_AddInputCharactersUTF8(io, event->text.text);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            ImGuiIO_AddKeyEvent(io, ImGuiMod_Ctrl, (event->key.mod & SDL_KMOD_CTRL) != 0);
            ImGuiIO_AddKeyEvent(io, ImGuiMod_Shift, (event->key.mod & SDL_KMOD_SHIFT) != 0);
            ImGuiIO_AddKeyEvent(io, ImGuiMod_Alt, (event->key.mod & SDL_KMOD_ALT) != 0);
            ImGuiIO_AddKeyEvent(io, ImGuiMod_Super, (event->key.mod & SDL_KMOD_GUI) != 0);

            ImGuiKey key = ImGuiKey_None;
            switch (event->key.key) {
                case SDLK_TAB: key = ImGuiKey_Tab; break;
                case SDLK_LEFT: key = ImGuiKey_LeftArrow; break;
                case SDLK_RIGHT: key = ImGuiKey_RightArrow; break;
                case SDLK_UP: key = ImGuiKey_UpArrow; break;
                case SDLK_DOWN: key = ImGuiKey_DownArrow; break;
                case SDLK_PAGEUP: key = ImGuiKey_PageUp; break;
                case SDLK_PAGEDOWN: key = ImGuiKey_PageDown; break;
                case SDLK_HOME: key = ImGuiKey_Home; break;
                case SDLK_END: key = ImGuiKey_End; break;
                case SDLK_INSERT: key = ImGuiKey_Insert; break;
                case SDLK_DELETE: key = ImGuiKey_Delete; break;
                case SDLK_BACKSPACE: key = ImGuiKey_Backspace; break;
                case SDLK_SPACE: key = ImGuiKey_Space; break;
                case SDLK_RETURN: key = ImGuiKey_Enter; break;
                case SDLK_ESCAPE: key = ImGuiKey_Escape; break;
                case SDLK_APOSTROPHE: key = ImGuiKey_Apostrophe; break;
                case SDLK_COMMA: key = ImGuiKey_Comma; break;
                case SDLK_MINUS: key = ImGuiKey_Minus; break;
                case SDLK_PERIOD: key = ImGuiKey_Period; break;
                case SDLK_SLASH: key = ImGuiKey_Slash; break;
                case SDLK_SEMICOLON: key = ImGuiKey_Semicolon; break;
                case SDLK_EQUALS: key = ImGuiKey_Equal; break;
                case SDLK_LEFTBRACKET: key = ImGuiKey_LeftBracket; break;
                case SDLK_BACKSLASH: key = ImGuiKey_Backslash; break;
                case SDLK_RIGHTBRACKET: key = ImGuiKey_RightBracket; break;
                case SDLK_GRAVE: key = ImGuiKey_GraveAccent; break;
                case SDLK_CAPSLOCK: key = ImGuiKey_CapsLock; break;
                case SDLK_SCROLLLOCK: key = ImGuiKey_ScrollLock; break;
                case SDLK_NUMLOCKCLEAR: key = ImGuiKey_NumLock; break;
                case SDLK_PRINTSCREEN: key = ImGuiKey_PrintScreen; break;
                case SDLK_PAUSE: key = ImGuiKey_Pause; break;
                case SDLK_LCTRL: key = ImGuiKey_LeftCtrl; break;
                case SDLK_LSHIFT: key = ImGuiKey_LeftShift; break;
                case SDLK_LALT: key = ImGuiKey_LeftAlt; break;
                case SDLK_LGUI: key = ImGuiKey_LeftSuper; break;
                case SDLK_RCTRL: key = ImGuiKey_RightCtrl; break;
                case SDLK_RSHIFT: key = ImGuiKey_RightShift; break;
                case SDLK_RALT: key = ImGuiKey_RightAlt; break;
                case SDLK_RGUI: key = ImGuiKey_RightSuper; break;
                case SDLK_APPLICATION: key = ImGuiKey_Menu; break;
                case SDLK_0: key = ImGuiKey_0; break;
                case SDLK_1: key = ImGuiKey_1; break;
                case SDLK_2: key = ImGuiKey_2; break;
                case SDLK_3: key = ImGuiKey_3; break;
                case SDLK_4: key = ImGuiKey_4; break;
                case SDLK_5: key = ImGuiKey_5; break;
                case SDLK_6: key = ImGuiKey_6; break;
                case SDLK_7: key = ImGuiKey_7; break;
                case SDLK_8: key = ImGuiKey_8; break;
                case SDLK_9: key = ImGuiKey_9; break;
                case SDLK_A: key = ImGuiKey_A; break;
                case SDLK_B: key = ImGuiKey_B; break;
                case SDLK_C: key = ImGuiKey_C; break;
                case SDLK_D: key = ImGuiKey_D; break;
                case SDLK_E: key = ImGuiKey_E; break;
                case SDLK_F: key = ImGuiKey_F; break;
                case SDLK_G: key = ImGuiKey_G; break;
                case SDLK_H: key = ImGuiKey_H; break;
                case SDLK_I: key = ImGuiKey_I; break;
                case SDLK_J: key = ImGuiKey_J; break;
                case SDLK_K: key = ImGuiKey_K; break;
                case SDLK_L: key = ImGuiKey_L; break;
                case SDLK_M: key = ImGuiKey_M; break;
                case SDLK_N: key = ImGuiKey_N; break;
                case SDLK_O: key = ImGuiKey_O; break;
                case SDLK_P: key = ImGuiKey_P; break;
                case SDLK_Q: key = ImGuiKey_Q; break;
                case SDLK_R: key = ImGuiKey_R; break;
                case SDLK_S: key = ImGuiKey_S; break;
                case SDLK_T: key = ImGuiKey_T; break;
                case SDLK_U: key = ImGuiKey_U; break;
                case SDLK_V: key = ImGuiKey_V; break;
                case SDLK_W: key = ImGuiKey_W; break;
                case SDLK_X: key = ImGuiKey_X; break;
                case SDLK_Y: key = ImGuiKey_Y; break;
                case SDLK_Z: key = ImGuiKey_Z; break;
                case SDLK_F1: key = ImGuiKey_F1; break;
                case SDLK_F2: key = ImGuiKey_F2; break;
                case SDLK_F3: key = ImGuiKey_F3; break;
                case SDLK_F4: key = ImGuiKey_F4; break;
                case SDLK_F5: key = ImGuiKey_F5; break;
                case SDLK_F6: key = ImGuiKey_F6; break;
                case SDLK_F7: key = ImGuiKey_F7; break;
                case SDLK_F8: key = ImGuiKey_F8; break;
                case SDLK_F9: key = ImGuiKey_F9; break;
                case SDLK_F10: key = ImGuiKey_F10; break;
                case SDLK_F11: key = ImGuiKey_F11; break;
                case SDLK_F12: key = ImGuiKey_F12; break;
                case SDLK_F13: key = ImGuiKey_F13; break;
                case SDLK_F14: key = ImGuiKey_F14; break;
                case SDLK_F15: key = ImGuiKey_F15; break;
                case SDLK_F16: key = ImGuiKey_F16; break;
                case SDLK_F17: key = ImGuiKey_F17; break;
                case SDLK_F18: key = ImGuiKey_F18; break;
                case SDLK_F19: key = ImGuiKey_F19; break;
                case SDLK_F20: key = ImGuiKey_F20; break;
                case SDLK_F21: key = ImGuiKey_F21; break;
                case SDLK_F22: key = ImGuiKey_F22; break;
                case SDLK_F23: key = ImGuiKey_F23; break;
                case SDLK_F24: key = ImGuiKey_F24; break;
                case SDLK_AC_BACK: key = ImGuiKey_AppBack; break;
                case SDLK_AC_FORWARD: key = ImGuiKey_AppForward; break;
                case SDLK_KP_0: key = ImGuiKey_Keypad0; break;
                case SDLK_KP_1: key = ImGuiKey_Keypad1; break;
                case SDLK_KP_2: key = ImGuiKey_Keypad2; break;
                case SDLK_KP_3: key = ImGuiKey_Keypad3; break;
                case SDLK_KP_4: key = ImGuiKey_Keypad4; break;
                case SDLK_KP_5: key = ImGuiKey_Keypad5; break;
                case SDLK_KP_6: key = ImGuiKey_Keypad6; break;
                case SDLK_KP_7: key = ImGuiKey_Keypad7; break;
                case SDLK_KP_8: key = ImGuiKey_Keypad8; break;
                case SDLK_KP_9: key = ImGuiKey_Keypad9; break;
                case SDLK_KP_PERIOD: key = ImGuiKey_KeypadDecimal; break;
                case SDLK_KP_DIVIDE: key = ImGuiKey_KeypadDivide; break;
                case SDLK_KP_MULTIPLY: key = ImGuiKey_KeypadMultiply; break;
                case SDLK_KP_MINUS: key = ImGuiKey_KeypadSubtract; break;
                case SDLK_KP_PLUS: key = ImGuiKey_KeypadAdd; break;
                case SDLK_KP_ENTER: key = ImGuiKey_KeypadEnter; break;
                case SDLK_KP_EQUALS: key = ImGuiKey_KeypadEqual; break;
                default: break;
            }

            ImGuiIO_AddKeyEvent(io, key, event->type == SDL_EVENT_KEY_DOWN);
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST: {
            ImGuiIO_AddFocusEvent(io, event->type == SDL_EVENT_WINDOW_FOCUS_GAINED);
            break;
        }

        default: break;
    }
}

static void sc_gui_frame_begin(ScGui* gui) {
    // Unpack.
    ImGuiIO* io = ImGui_GetIO();

    // Window.
    int32_t w, h;
    int32_t display_w, display_h;
    SDL_GetWindowSize(gui->window, &w, &h);
    if (SDL_GetWindowFlags(gui->window) & SDL_WINDOW_MINIMIZED) {
        w = h = 0;
    }
    SDL_GetWindowSizeInPixels(gui->window, &display_w, &display_h);
    io->DisplaySize.x = (float)w;
    io->DisplaySize.y = (float)h;
    if (w > 0 && h > 0) {
        io->DisplayFramebufferScale.x = (float)display_w / w;
        io->DisplayFramebufferScale.y = (float)display_h / h;
    }

    // Time.
    const uint64_t counter = SDL_GetPerformanceCounter();
    io->DeltaTime =
        (float)((double)(counter - gui->performance_counter) / (double)gui->performance_frequency);
    gui->performance_counter = counter;

    // New frame.
    ImGui_NewFrame();
}

static void sc_gui_frame_end(
    ScGui* gui,
    SDL_GPUDevice* device,
    SDL_GPUCommandBuffer* command_buffer,
    SDL_GPURenderPass* render_pass
) {
    // Generate draw data.
    ImGui_Render();

    // Get draw data.
    ImDrawData* draw_data = ImGui_GetDrawData();

    // Validation.
    const uint32_t vertex_count = (uint32_t)draw_data->TotalVtxCount;
    const uint32_t index_count = (uint32_t)draw_data->TotalIdxCount;
    SC_ASSERT(vertex_count <= gui->vertex_buffer_capacity);
    SC_ASSERT(index_count <= gui->index_buffer_capacity);

    // Early out.
    if (vertex_count == 0 || index_count == 0) {
        return;
    }

    // Select buffers.
    SDL_GPUBuffer* vertex_buffer = gui->vertex_buffers[gui->frame_index];
    SDL_GPUBuffer* index_buffer = gui->index_buffers[gui->frame_index];
    SDL_GPUTransferBuffer* transfer_buffer = gui->transfer_buffers[gui->frame_index];

    // Copy to device.
    const uint32_t vertex_byte_count = vertex_count * sizeof(ScGuiVertex);
    const uint32_t index_byte_count = index_count * sizeof(ScGuiIndex);
    ScGuiVertex* vertex_data_dst = gui->vertex_data;
    ScGuiIndex* index_data_dst = gui->index_data;
    for (int32_t cmd_list_idx = 0; cmd_list_idx < draw_data->CmdListsCount; cmd_list_idx++) {
        ImDrawList* cmd_list = draw_data->CmdLists.Data[cmd_list_idx];
        memcpy(
            vertex_data_dst,
            cmd_list->VtxBuffer.Data,
            cmd_list->VtxBuffer.Size * sizeof(ImDrawVert)
        );
        memcpy(
            index_data_dst,
            cmd_list->IdxBuffer.Data,
            cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx)
        );
        vertex_data_dst += cmd_list->VtxBuffer.Size;
        index_data_dst += cmd_list->IdxBuffer.Size;
    }
    SC_ASSERT(vertex_data_dst == gui->vertex_data + vertex_count);
    SC_ASSERT(index_data_dst == gui->index_data + index_count);
    uint8_t* dst = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy(dst, gui->vertex_data, vertex_byte_count);
    memcpy(dst + vertex_byte_count, gui->index_data, index_byte_count);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_GPUCommandBuffer* upload_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_buffer);
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        },
        &(SDL_GPUBufferRegion) {
            .buffer = vertex_buffer,
            .offset = 0,
            .size = vertex_byte_count,
        },
        false
    );
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = vertex_byte_count,
        },
        &(SDL_GPUBufferRegion) {
            .buffer = index_buffer,
            .offset = 0,
            .size = index_byte_count,
        },
        false
    );
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_buffer);

    // Render.
    SDL_BindGPUGraphicsPipeline(render_pass, gui->pipeline);
    SDL_SetGPUViewport(
        render_pass,
        &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = draw_data->DisplaySize.x,
            .h = draw_data->DisplaySize.y,
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        }
    );
    SDL_BindGPUVertexBuffers(
        render_pass,
        0,
        (SDL_GPUBufferBinding[]) {
            {
                .buffer = vertex_buffer,
                .offset = 0,
            },
        },
        1
    );
    SDL_BindGPUIndexBuffer(
        render_pass,
        &(SDL_GPUBufferBinding) {
            .buffer = index_buffer,
            .offset = 0,
        },
        SDL_GPU_INDEXELEMENTSIZE_16BIT
    );
    SDL_BindGPUFragmentSamplers(
        render_pass,
        0,
        (SDL_GPUTextureSamplerBinding[]) {{
            .texture = gui->font_texture,
            .sampler = gui->font_sampler,
        }},
        1
    );
    ScGuiUniforms uniforms = {
        .scale.x = 2.0f / draw_data->DisplaySize.x,
        .scale.y = -2.0f / draw_data->DisplaySize.y,
        .offset.x = -1.0f - draw_data->DisplayPos.x * (2.0f / draw_data->DisplaySize.x),
        .offset.y = 1.0f - draw_data->DisplayPos.y * (2.0f / draw_data->DisplaySize.y),
    };
    SDL_PushGPUVertexUniformData(command_buffer, 0, &uniforms, sizeof(ScGuiUniforms));
    uint32_t global_vertex_offset = 0;
    uint32_t global_index_offset = 0;
    for (int32_t cmd_list_idx = 0; cmd_list_idx < draw_data->CmdListsCount; cmd_list_idx++) {
        ImDrawList* cmd_list = draw_data->CmdLists.Data[cmd_list_idx];
        for (int32_t cmd_idx = 0; cmd_idx < cmd_list->CmdBuffer.Size; cmd_idx++) {
            ImDrawCmd* cmd = &cmd_list->CmdBuffer.Data[cmd_idx];
            SDL_SetGPUScissor(
                render_pass,
                &(SDL_Rect) {
                    .x = (int32_t)cmd->ClipRect.x,
                    .y = (int32_t)cmd->ClipRect.y,
                    .w = (int32_t)(cmd->ClipRect.z - cmd->ClipRect.x),
                    .h = (int32_t)(cmd->ClipRect.w - cmd->ClipRect.y),
                }
            );
            SDL_DrawGPUIndexedPrimitives(
                render_pass,
                cmd->ElemCount,
                1,
                cmd->IdxOffset + global_index_offset,
                cmd->VtxOffset + global_vertex_offset,
                0
            );
        }
        global_vertex_offset += cmd_list->VtxBuffer.Size;
        global_index_offset += cmd_list->IdxBuffer.Size;
    }

    // Update.
    gui->frame_index = (gui->frame_index + 1) % 2;
}
