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
    uint32_t node_leaf_level;

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
    fread(&octree->node_leaf_level, 1, sizeof(uint32_t), file);

    // Load nodes.
    octree->nodes = malloc(octree->node_count * sizeof(ScOctreeNode));
    fread(octree->nodes, 1, octree->node_count * sizeof(ScOctreeNode), file);

    // Load points.
    octree->points = malloc(octree->point_count * sizeof(ScOctreePoint));
    fread(octree->points, 1, octree->point_count * sizeof(ScOctreePoint), file);

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
    SC_LOG_INFO("Node leaf level: %u", octree->node_leaf_level);
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
    const uint32_t node_leaf_level = octree->node_leaf_level;
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
        if (curr_node->level == node_leaf_level) {
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
