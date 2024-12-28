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
