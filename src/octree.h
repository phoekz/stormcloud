typedef struct ScOctreeNode {
    int32_t min_x;
    int32_t min_y;
    int32_t min_z;
    int32_t max_x;
    int32_t max_y;
    int32_t max_z;
    uint16_t level;
    uint16_t octant_mask;
    uint32_t point_count;
    uint64_t point_offset;
    uint32_t octants[8];
} ScOctreeNode;

typedef struct ScOctreeNodeInstance {
    float min_x;
    float min_y;
    float min_z;
    float max_x;
    float max_y;
    float max_z;
} ScOctreeNodeInstance;

typedef struct ScOctreePoint {
    uint32_t position;
    uint32_t color;
} ScOctreePoint;

typedef struct ScOctreeUniforms {
    mat4f clip_from_world;
    float node_world_scale;
    uint32_t pad[3];
} ScOctreeUniforms;

typedef struct ScOctree {
    float unit_world_scale;
    float node_unit_count;
    float node_world_scale;

    ScOctreeNode* nodes;
    ScOctreeNodeInstance* node_instances;
    uint64_t node_count;

    ScOctreePoint* points;
    uint64_t point_count;
    box3f point_bounds;

    uint32_t* node_traverse;
    uint32_t node_traverse_count;
} ScOctree;

static void sc_octree_new(ScOctree* octree, const char* file_path) {
    // Timing.
    const uint64_t begin_time_ns = SDL_GetTicksNS();

    // Load header.
    FILE* file = fopen(file_path, "rb");
    char magic[8];
    fread(magic, 1, sizeof(magic), file);
    SC_ASSERT(strncmp(magic, "TOKYOOCT", sizeof(magic)) == 0);
    fread(&octree->node_count, 1, sizeof(uint64_t), file);
    fread(&octree->point_count, 1, sizeof(uint64_t), file);
    fread(&octree->point_bounds, 1, sizeof(box3f), file);
    fread(&octree->unit_world_scale, 1, sizeof(float), file);
    fread(&octree->node_unit_count, 1, sizeof(float), file);
    fread(&octree->node_world_scale, 1, sizeof(float), file);

    // Load nodes.
    octree->nodes = malloc(octree->node_count * sizeof(ScOctreeNode));
    fread(octree->nodes, 1, octree->node_count * sizeof(ScOctreeNode), file);

    // Load points.
    octree->points = malloc(octree->point_count * sizeof(ScOctreePoint));
    fread(octree->points, 1, octree->point_count * sizeof(ScOctreePoint), file);

    // Debug: morton order visualization.
    const bool debug_morton_order_coloring = false;
    if (debug_morton_order_coloring) {
        for (uint32_t node_idx = 0; node_idx < octree->node_count; ++node_idx) {
            const ScOctreeNode* node = &octree->nodes[node_idx];
            for (uint32_t point_idx = 0; point_idx < node->point_count; ++point_idx) {
                ScOctreePoint* point = &octree->points[node->point_offset + point_idx];
                const float linear_ratio = (float)point_idx / (float)node->point_count;
                point->color = sc_color_from_hsv(linear_ratio, 0.75f, 1.0f);
            }
        }
    }

    // Debug: write points as images.
    const bool debug_write_point_images = false;
    if (debug_write_point_images) {
        for (uint32_t node_idx = 0; node_idx < octree->node_count; ++node_idx) {
            const ScOctreeNode* node = &octree->nodes[node_idx];
            uint32_t image_size = 1;
            while (image_size * image_size < node->point_count) {
                image_size *= 2;
            }
            const uint32_t image_width = image_size;
            const uint32_t image_height = image_size;
            uint8_t* image_data = malloc(image_width * image_height * 4);
            for (uint32_t i = 0; i < image_width * image_height; i++) {
                image_data[4 * i + 0] = 0;
                image_data[4 * i + 1] = 0;
                image_data[4 * i + 2] = 0;
                image_data[4 * i + 3] = 255;
            }
            for (uint32_t point_idx = 0; point_idx < node->point_count; ++point_idx) {
                const ScOctreePoint* point = &octree->points[node->point_offset + point_idx];
                uint16_t x, y;
                morton2_decode32(&x, &y, point_idx);
                image_data[4 * (y * image_width + x) + 0] = (point->color >> 0) & 0xff;
                image_data[4 * (y * image_width + x) + 1] = (point->color >> 8) & 0xff;
                image_data[4 * (y * image_width + x) + 2] = (point->color >> 16) & 0xff;
                image_data[4 * (y * image_width + x) + 3] = 255;
            }
            char image_name[64];
            snprintf(image_name, sizeof(image_name), "temp/node_%d_%d.png", node->level, node_idx);
            stbi_write_png(image_name, image_width, image_height, 4, image_data, image_width * 4);
            free(image_data);
        }
    }

    // Node instances.
    octree->node_instances = malloc(octree->node_count * sizeof(ScOctreeNodeInstance));
    for (uint64_t i = 0; i < octree->node_count; ++i) {
        const ScOctreeNode* node = &octree->nodes[i];
        octree->node_instances[i] = (ScOctreeNodeInstance) {
            .min_x = (float)node->min_x,
            .min_y = (float)node->min_y,
            .min_z = (float)node->min_z,
            .max_x = (float)node->max_x,
            .max_y = (float)node->max_y,
            .max_z = (float)node->max_z,
        };
    }

    // Node traversal.
    octree->node_traverse = malloc(octree->node_count * sizeof(uint32_t));
    octree->node_traverse_count = 0;

    // Timing.
    const uint64_t end_time_ns = SDL_GetTicksNS();
    const uint64_t elapsed_time_ns = end_time_ns - begin_time_ns;
    SC_LOG_INFO(
        "Loaded %d points in %" PRIu64 " ms",
        octree->point_count,
        elapsed_time_ns / 1000000
    );

    // Logging.
    // clang-format off
    const vec3f point_bounds_extents = box3f_extents(octree->point_bounds);
    const vec3f point_bounds_center = box3f_center(octree->point_bounds);
    SC_LOG_INFO("Node count: %llu", octree->node_count);
    SC_LOG_INFO("Point count: %llu", octree->point_count);
    SC_LOG_INFO("Point bounds:");
    SC_LOG_INFO("  Min: %f, %f, %f", octree->point_bounds.mn.x, octree->point_bounds.mn.y, octree->point_bounds.mn.z);
    SC_LOG_INFO("  Max: %f, %f, %f", octree->point_bounds.mx.x, octree->point_bounds.mx.y, octree->point_bounds.mx.z);
    SC_LOG_INFO("  Extents: %f, %f, %f", point_bounds_extents.x, point_bounds_extents.y, point_bounds_extents.z);
    SC_LOG_INFO("  Center: %f, %f, %f", point_bounds_center.x, point_bounds_center.y, point_bounds_center.z);
    SC_LOG_INFO("Unit world scale: %f", octree->unit_world_scale);
    SC_LOG_INFO("Node unit count: %f", octree->node_unit_count);
    SC_LOG_INFO("Node world scale: %f", octree->node_world_scale);
    // clang-format on
}

static void sc_octree_free(ScOctree* octree) {
    free(octree->nodes);
    free(octree->node_instances);
    free(octree->points);
    free(octree->node_traverse);
}

typedef struct ScOctreeTraverseInfo {
    const ScPerspectiveCamera* camera;
    float lod_bias;
} ScOctreeTraverseInfo;

static void sc_octree_traverse(ScOctree* octree, const ScOctreeTraverseInfo* traverse_info) {
    // Unpack.
    const float node_unit_count = octree->node_unit_count;
    const float node_world_scale = octree->node_world_scale;
    const ScPerspectiveCamera* camera = traverse_info->camera;
    const float lod_bias = traverse_info->lod_bias;

    // Reset.
    octree->node_traverse_count = 0;

    // Traverse state.
    uint32_t todo[64] = {0};
    uint32_t todo_count = 0;
    todo[todo_count++] = 0;

    // Traversal.
    while (todo_count) {
        // Unpack.
        const uint32_t curr = todo[--todo_count];
        const ScOctreeNode* curr_node = &octree->nodes[curr];

        // Calculate current bounds.
        const vec3f curr_bounds_mn = (vec3f) {
            node_world_scale * (float)curr_node->min_x,
            node_world_scale * (float)curr_node->min_y,
            node_world_scale * (float)curr_node->min_z,
        };
        const vec3f curr_bounds_mx = (vec3f) {
            node_world_scale * (float)curr_node->max_x,
            node_world_scale * (float)curr_node->max_y,
            node_world_scale * (float)curr_node->max_z,
        };
        const box3f curr_bounds = (box3f) {
            .mn = curr_bounds_mn,
            .mx = curr_bounds_mx,
        };

        // Frustum culling.
        if (!sc_frustum_intersects_box(&camera->frustum, curr_bounds)) {
            continue;
        }

        // Special: leaf nodes are always rendered.
        if (curr_node->level == 0) {
            octree->node_traverse[octree->node_traverse_count++] = curr;
            continue;
        }

        // Calculate unit bounding sphere.
        const sphere3f curr_sphere = sphere3f_from_box3f(curr_bounds);
        const sphere3f unit_sphere = (sphere3f) {
            .o = curr_sphere.o,
            .r = curr_sphere.r / node_unit_count,
        };

        // Screen projected sphere area.
        // Todo: Can be negative, investigate why.
        const float sphere_area = sc_screen_projected_sphere_area(camera, unit_sphere);
        if (sphere_area > 0.0f && sphere_area < lod_bias) {
            octree->node_traverse[octree->node_traverse_count++] = curr;
            continue;
        }

        // Traverse children.
        for (uint32_t i = 0; i < 8; ++i) {
            const uint32_t child = curr_node->octants[i];
            if (child == ~0u) {
                continue;
            }
            SC_ASSERT(todo_count < SC_COUNTOF(todo));
            todo[todo_count++] = child;
        }
    }
}
