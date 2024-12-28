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
