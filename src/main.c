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
// Utilities.
//

static uint64_t u64_min(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

#define SC_UNUSED(x) (void)(x)
#define SC_ASSERT(expr) \
    if (!(expr)) {      \
        abort();        \
    }

//
// Stormcloud data.
//

typedef struct ScPosition {
    float x;
    float y;
    float z;
} ScPosition;

typedef struct ScColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ScColor;

typedef struct ScData {
    uint64_t point_count;
    const ScPosition* positions;
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
    ScPosition* positions = malloc(hdr.point_count * sizeof(ScPosition));
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
    sc_data->positions = (const ScPosition*)positions;
    sc_data->colors = (const ScColor*)colors;

    // Timing.
    const uint64_t end_time_ns = SDL_GetTicksNS();
    const uint64_t elapsed_time_ns = end_time_ns - begin_time_ns;
    SDL_Log("Loaded %d points in %" PRIu64 " ms", hdr.point_count, elapsed_time_ns / 1000000);
}

static void sc_data_free(ScData* sc_data) {
    free((void*)sc_data->positions);
    free((void*)sc_data->colors);
}

//
// Main.
//

typedef struct ScApp {
    ScData data;
    SDL_Window* window;
    SDL_GPUDevice* device;
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
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Window & device.
    app->window = SDL_CreateWindow("stormcloud", 1280, 800, 0);
    if (app->window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    app->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, false, NULL);
    if (app->device == NULL) {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_ClaimWindowForGPUDevice(app->device, app->window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    const SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
    const SDL_GPUSwapchainComposition composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR;
    if (!SDL_WindowSupportsGPUPresentMode(app->device, app->window, present_mode)) {
        SDL_Log("SDL_WindowSupportsGPUPresentMode failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetGPUSwapchainParameters(app->device, app->window, composition, present_mode)) {
        SDL_Log("SDL_SetGPUSwapchainParameters failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

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
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return -1;
    }

    // Swapchain.
    SDL_GPUTexture* swapchain = NULL;
    if (!SDL_AcquireGPUSwapchainTexture(cmd, app->window, &swapchain, NULL, NULL)) {
        SDL_Log("SDL_AcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return -1;
    }

    // Render pass.
    if (swapchain != NULL) {
        SDL_GPUColorTargetInfo color_target_info = {0};
        color_target_info.texture = swapchain;
        color_target_info.clear_color = (SDL_FColor) {1.0f, 0.5f, 0.0f, 1.0f};
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &color_target_info, 1, NULL);
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
    SDL_ReleaseWindowFromGPUDevice(app->device, app->window);
    SDL_DestroyWindow(app->window);
    SDL_DestroyGPUDevice(app->device);

    // Free.
    sc_data_free(&app->data);
    free(app);

    // End.
    SC_ASSERT(result == SDL_APP_SUCCESS);
}
