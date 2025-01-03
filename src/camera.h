//
// Frustum
//

// Notes:
// - https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
// - https://donw.io/post/frustum-point-extraction/
// - https://iquilezles.org/articles/frustumcorrect/
// - https://iquilezles.org/articles/sphereproj/

typedef enum ScFrustumPlane {
    SC_FRUSTUM_PLANE_L, // -x
    SC_FRUSTUM_PLANE_R, // +x
    SC_FRUSTUM_PLANE_B, // -y
    SC_FRUSTUM_PLANE_T, // +y
    SC_FRUSTUM_PLANE_N, // -z
    SC_FRUSTUM_PLANE_F, // +z
    SC_FRUSTUM_PLANE_COUNT,
} ScFrustumPlane;

typedef enum ScFrustumCorner {
    SC_FRUSTUM_CORNER_LBN,
    SC_FRUSTUM_CORNER_RBN,
    SC_FRUSTUM_CORNER_LTN,
    SC_FRUSTUM_CORNER_RTN,
    SC_FRUSTUM_CORNER_LBF,
    SC_FRUSTUM_CORNER_RBF,
    SC_FRUSTUM_CORNER_LTF,
    SC_FRUSTUM_CORNER_RTF,
    SC_FRUSTUM_CORNER_COUNT,
} ScFrustumCorner;

typedef struct ScFrustum {
    plane3f planes[SC_FRUSTUM_PLANE_COUNT];
    vec3f corners[SC_FRUSTUM_CORNER_COUNT];
} ScFrustum;

static bool sc_frustum_intersects_box(const ScFrustum* frustum, box3f box) {
    // Unpack.
    const plane3f* frustum_planes = frustum->planes;
    const vec3f* frustum_corners = frustum->corners;

    // Box corners.
    const vec4f box_corners[8] = {
        (vec4f) {box.mn.x, box.mn.y, box.mn.z, 1.0f},
        (vec4f) {box.mx.x, box.mn.y, box.mn.z, 1.0f},
        (vec4f) {box.mn.x, box.mx.y, box.mn.z, 1.0f},
        (vec4f) {box.mx.x, box.mx.y, box.mn.z, 1.0f},
        (vec4f) {box.mn.x, box.mn.y, box.mx.z, 1.0f},
        (vec4f) {box.mx.x, box.mn.y, box.mx.z, 1.0f},
        (vec4f) {box.mn.x, box.mx.y, box.mx.z, 1.0f},
        (vec4f) {box.mx.x, box.mx.y, box.mx.z, 1.0f},
    };

    // Init.
    uint32_t count;

    // Box outside/inside frustum - 0
    count = 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[0]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[1]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[2]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[3]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[4]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[5]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[6]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[0]), box_corners[7]) < 0.0f) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Box outside/inside frustum - 1
    count = 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[0]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[1]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[2]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[3]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[4]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[5]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[6]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[1]), box_corners[7]) < 0.0f) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Box outside/inside frustum - 2
    count = 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[0]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[1]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[2]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[3]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[4]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[5]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[6]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[2]), box_corners[7]) < 0.0f) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Box outside/inside frustum - 3
    count = 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[0]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[1]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[2]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[3]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[4]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[5]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[6]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[3]), box_corners[7]) < 0.0f) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Box outside/inside frustum - 4
    count = 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[0]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[1]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[2]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[3]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[4]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[5]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[6]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[4]), box_corners[7]) < 0.0f) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Box outside/inside frustum - 5
    count = 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[0]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[1]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[2]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[3]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[4]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[5]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[6]) < 0.0f) ? 1 : 0;
    count += (vec4f_dot(vec4f_from_plane3f(frustum_planes[5]), box_corners[7]) < 0.0f) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Frustum outside/inside box - 0
    count = 0;
    count += (frustum_corners[0].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[1].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[2].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[3].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[4].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[5].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[6].x > box.mx.x) ? 1 : 0;
    count += (frustum_corners[7].x > box.mx.x) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Frustum outside/inside box - 1
    count = 0;
    count += (frustum_corners[0].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[1].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[2].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[3].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[4].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[5].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[6].x < box.mn.x) ? 1 : 0;
    count += (frustum_corners[7].x < box.mn.x) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Frustum outside/inside box - 2
    count = 0;
    count += (frustum_corners[0].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[1].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[2].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[3].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[4].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[5].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[6].y > box.mx.y) ? 1 : 0;
    count += (frustum_corners[7].y > box.mx.y) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Frustum outside/inside box - 3
    count = 0;
    count += (frustum_corners[0].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[1].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[2].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[3].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[4].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[5].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[6].y < box.mn.y) ? 1 : 0;
    count += (frustum_corners[7].y < box.mn.y) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Frustum outside/inside box - 4
    count = 0;
    count += (frustum_corners[0].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[1].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[2].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[3].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[4].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[5].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[6].z > box.mx.z) ? 1 : 0;
    count += (frustum_corners[7].z > box.mx.z) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Frustum outside/inside box - 5
    count = 0;
    count += (frustum_corners[0].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[1].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[2].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[3].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[4].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[5].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[6].z < box.mn.z) ? 1 : 0;
    count += (frustum_corners[7].z < box.mn.z) ? 1 : 0;
    if (count == 8) {
        return false;
    }

    // Inside or intersects.
    return true;
}

//
// Perspective camera
//

typedef struct ScPerspectiveCameraCreateInfo {
    float screen_width;
    float screen_height;
    float field_of_view;
    float clip_distance_near;
    float clip_distance_far;
    vec3f world_position;
    vec3f world_target;
    vec3f world_up;
} ScPerspectiveCameraCreateInfo;

typedef struct ScPerspectiveCamera {
    // Screen parameters.
    float screen_width;
    float screen_height;
    float screen_aspect_ratio;
    float screen_area;

    // Camera parameters.
    float field_of_view;
    float focal_length;
    float clip_distance_near;
    float clip_distance_far;

    // Camera space.
    vec3f world_position;
    vec3f world_right;
    vec3f world_up;
    vec3f world_forward;

    // Transforms.
    mat4f view_from_world;
    mat4f clip_from_view;
    mat4f clip_from_world;

    // Inverse transforms.
    mat4f view_from_clip;
    mat4f world_from_view;
    mat4f world_from_clip;

    // Frustum.
    ScFrustum frustum;
} ScPerspectiveCamera;

static ScPerspectiveCamera
sc_perspective_camera_new(const ScPerspectiveCameraCreateInfo* create_info) {
    // Validation.
    SC_ASSERT(create_info->screen_width > 0.0f);
    SC_ASSERT(create_info->screen_height > 0.0f);
    SC_ASSERT(create_info->field_of_view > 0.0f);
    SC_ASSERT(create_info->field_of_view < SC_PI);
    SC_ASSERT(create_info->clip_distance_near > 0.0f);
    SC_ASSERT(create_info->clip_distance_far > create_info->clip_distance_near);

    // Screen parameters.
    const float screen_width = create_info->screen_width;
    const float screen_height = create_info->screen_height;
    const float screen_aspect_ratio = screen_width / screen_height;
    const float screen_area = screen_width * screen_height;
    SC_ASSERT(isfinite(screen_aspect_ratio));
    SC_ASSERT(isfinite(screen_area));

    // Camera parameters.
    const float field_of_view = create_info->field_of_view;
    const float focal_length = 1.0f / tanf(field_of_view * 0.5f);
    const float clip_distance_near = create_info->clip_distance_near;
    const float clip_distance_far = create_info->clip_distance_far;
    SC_ASSERT(isfinite(focal_length));

    // Camera space.
    const vec3f world_position = create_info->world_position;
    const vec3f world_target = create_info->world_target;
    const vec3f world_forward = vec3f_normalize(vec3f_sub(world_target, world_position));
    const vec3f world_right = vec3f_normalize(vec3f_cross(world_forward, create_info->world_up));
    const vec3f world_up = vec3f_normalize(vec3f_cross(world_right, world_forward));
    SC_ASSERT(vec3f_isfinite(world_position));
    SC_ASSERT(vec3f_isfinite(world_target));
    SC_ASSERT(vec3f_isfinite(world_forward));
    SC_ASSERT(vec3f_isfinite(world_right));
    SC_ASSERT(vec3f_isfinite(world_up));

    // Transforms.
    const mat4f view_from_world = mat4f_lookat(world_position, world_target, world_up);
    const mat4f clip_from_view = mat4f_perspective(
        field_of_view,
        screen_aspect_ratio,
        clip_distance_near,
        clip_distance_far
    );
    const mat4f clip_from_world = mat4f_mul(clip_from_view, view_from_world);

    // Inverse transforms.
    const mat4f view_from_clip = mat4f_inverse(clip_from_view);
    const mat4f world_from_view = mat4f_inverse(view_from_world);
    const mat4f world_from_clip = mat4f_mul(world_from_view, view_from_clip);

    // Frustum.
    ScFrustum frustum;
    {
        const vec4f r0 = mat4f_row(clip_from_world, 0);
        const vec4f r1 = mat4f_row(clip_from_world, 1);
        const vec4f r2 = mat4f_row(clip_from_world, 2);
        const vec4f r3 = mat4f_row(clip_from_world, 3);
        frustum.planes[SC_FRUSTUM_PLANE_L] = plane3f_from_vec4f(vec4f_add(r3, r0));
        frustum.planes[SC_FRUSTUM_PLANE_R] = plane3f_from_vec4f(vec4f_sub(r3, r0));
        frustum.planes[SC_FRUSTUM_PLANE_B] = plane3f_from_vec4f(vec4f_sub(r3, r1));
        frustum.planes[SC_FRUSTUM_PLANE_T] = plane3f_from_vec4f(vec4f_add(r3, r1));
        frustum.planes[SC_FRUSTUM_PLANE_N] = plane3f_from_vec4f(vec4f_sub(r3, r2));
        frustum.planes[SC_FRUSTUM_PLANE_F] = plane3f_from_vec4f(vec4f_add(r3, r2));
    }
    {
        const vec4f lbn = mat4f_mul_vec4f(world_from_clip, vec4f_new(-1.0f, -1.0f, 0.0f, 1.0f));
        const vec4f rbn = mat4f_mul_vec4f(world_from_clip, vec4f_new(+1.0f, -1.0f, 0.0f, 1.0f));
        const vec4f ltn = mat4f_mul_vec4f(world_from_clip, vec4f_new(-1.0f, +1.0f, 0.0f, 1.0f));
        const vec4f rtn = mat4f_mul_vec4f(world_from_clip, vec4f_new(+1.0f, +1.0f, 0.0f, 1.0f));
        const vec4f lbf = mat4f_mul_vec4f(world_from_clip, vec4f_new(-1.0f, -1.0f, 1.0f, 1.0f));
        const vec4f rbf = mat4f_mul_vec4f(world_from_clip, vec4f_new(+1.0f, -1.0f, 1.0f, 1.0f));
        const vec4f ltf = mat4f_mul_vec4f(world_from_clip, vec4f_new(-1.0f, +1.0f, 1.0f, 1.0f));
        const vec4f rtf = mat4f_mul_vec4f(world_from_clip, vec4f_new(+1.0f, +1.0f, 1.0f, 1.0f));
        frustum.corners[SC_FRUSTUM_CORNER_LBN] = vec3f_scale(vec3f_from_vec4f(lbn), 1.0f / lbn.w);
        frustum.corners[SC_FRUSTUM_CORNER_RBN] = vec3f_scale(vec3f_from_vec4f(rbn), 1.0f / rbn.w);
        frustum.corners[SC_FRUSTUM_CORNER_LTN] = vec3f_scale(vec3f_from_vec4f(ltn), 1.0f / ltn.w);
        frustum.corners[SC_FRUSTUM_CORNER_RTN] = vec3f_scale(vec3f_from_vec4f(rtn), 1.0f / rtn.w);
        frustum.corners[SC_FRUSTUM_CORNER_LBF] = vec3f_scale(vec3f_from_vec4f(lbf), 1.0f / lbf.w);
        frustum.corners[SC_FRUSTUM_CORNER_RBF] = vec3f_scale(vec3f_from_vec4f(rbf), 1.0f / rbf.w);
        frustum.corners[SC_FRUSTUM_CORNER_LTF] = vec3f_scale(vec3f_from_vec4f(ltf), 1.0f / ltf.w);
        frustum.corners[SC_FRUSTUM_CORNER_RTF] = vec3f_scale(vec3f_from_vec4f(rtf), 1.0f / rtf.w);
    }

    return (ScPerspectiveCamera) {
        .screen_width = screen_width,
        .screen_height = screen_height,
        .screen_aspect_ratio = screen_aspect_ratio,
        .screen_area = screen_area,
        .field_of_view = field_of_view,
        .focal_length = focal_length,
        .clip_distance_near = clip_distance_near,
        .clip_distance_far = clip_distance_far,
        .world_position = world_position,
        .world_right = world_right,
        .world_up = world_up,
        .world_forward = world_forward,
        .view_from_world = view_from_world,
        .clip_from_view = clip_from_view,
        .clip_from_world = clip_from_world,
        .view_from_clip = view_from_clip,
        .world_from_view = world_from_view,
        .world_from_clip = world_from_clip,
        .frustum = frustum,
    };
}

static float sc_screen_projected_sphere_area(const ScPerspectiveCamera* camera, sphere3f sphere) {
    const float screen_area = camera->screen_area;
    const float fl = camera->focal_length;
    const mat4f v = camera->view_from_world;
    const vec3f o = vec3f_from_vec4f(mat4f_mul_vec4f(v, vec4f_from_vec3f(sphere.o, 1.0f)));
    const float r2 = sphere.r * sphere.r;
    const float z2 = o.z * o.z;
    const float l2 = vec3f_dot(o, o);
    const float area = -SC_PI * fl * fl * r2 * sqrtf(fabsf((l2 - r2) / (r2 - z2))) / (r2 - z2);
    const float result = area * screen_area * 0.25f;
    SC_ASSERT(isfinite(result));
    return result;
}

//
// Camera control - common
//

typedef struct ScCameraControlCommonCreateInfo {
    box3f scene_bounds;
} ScCameraControlCommonCreateInfo;

typedef struct ScCameraControlCommonUpdateInfo {
    float screen_width;
    float screen_height;
    float field_of_view;
    float clip_distance_near;
    float clip_distance_far;
    float delta_time;
    bool input_captured;
} ScCameraControlCommonUpdateInfo;

//
// Camera control - orbit
//

typedef struct ScCameraControlOrbitCreateInfo {
    ScCameraControlCommonCreateInfo common;
} ScCameraControlOrbitCreateInfo;

typedef struct ScCameraControlOrbitUpdateInfo {
    ScCameraControlCommonUpdateInfo common;
} ScCameraControlOrbitUpdateInfo;

typedef struct ScCameraControlOrbit {
    float orbit_turn_horizontal;
    float orbit_turn_horizontal_end;
    float orbit_turn_horizontal_rate;
    float orbit_turn_horizontal_speed;
    float orbit_turn_vertical;
    float orbit_turn_vertical_end;
    float orbit_turn_vertical_rate;
    float orbit_turn_vertical_min;
    float orbit_turn_vertical_max;

    float world_target_min_distance;
    float world_target_max_distance;
    float world_target_distance_rate;
    float world_target_distance_end;
    float world_target_distance;
    float world_target_pan_speed;
    vec3f world_target_end;
    float world_target_rate;
    vec3f world_target;
    vec3f world_up;

    bool mouse_left;
    bool mouse_right;
    vec2f mouse_motion;
    float mouse_wheel;
} ScCameraControlOrbit;

static ScCameraControlOrbit
sc_camera_control_orbit_new(const ScCameraControlOrbitCreateInfo* create_info) {
    const vec3f scene_extents = box3f_extents(create_info->common.scene_bounds);
    const vec3f scene_center = box3f_center(create_info->common.scene_bounds);
    const float scene_max_extent = vec3f_component_max(scene_extents);
    return (ScCameraControlOrbit) {
        .orbit_turn_horizontal = 0.0f,
        .orbit_turn_horizontal_end = 0.0f,
        .orbit_turn_horizontal_rate = 8.0f,
        .orbit_turn_horizontal_speed = 0.75f,
        .orbit_turn_vertical = 0.0f,
        .orbit_turn_vertical_end = 0.0f,
        .orbit_turn_vertical_rate = 8.0f,
        .orbit_turn_vertical_min = -0.5f + 1.0f / 64.0f,
        .orbit_turn_vertical_max = 0.5f - 1.0f / 64.0f,

        .world_target_min_distance = 16.0f,
        .world_target_max_distance = scene_max_extent,
        .world_target_distance_rate = 8.0f,
        .world_target_distance_end = scene_max_extent,
        .world_target_distance = scene_max_extent,
        .world_target_pan_speed = scene_max_extent * 0.5f,
        .world_target_end = scene_center,
        .world_target_rate = 8.0f,
        .world_target = scene_center,
        .world_up = (vec3f) {0.0f, 0.0f, 1.0f},

        .mouse_left = false,
        .mouse_right = false,
        .mouse_motion = (vec2f) {0.0f, 0.0f},
        .mouse_wheel = 0.0f,
    };
}

static void sc_camera_control_orbit_event(ScCameraControlOrbit* ctrl, SDL_Event* event) {
    switch (event->type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                ctrl->mouse_left = true;
            } else if (event->button.button == SDL_BUTTON_RIGHT) {
                ctrl->mouse_right = true;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                ctrl->mouse_left = false;
            } else if (event->button.button == SDL_BUTTON_RIGHT) {
                ctrl->mouse_right = false;
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            ctrl->mouse_motion = vec2f_new(event->motion.xrel, event->motion.yrel);
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            ctrl->mouse_wheel = event->wheel.y;
            break;
        }
        default: break;
    }
}

static void sc_camera_control_orbit_update(
    ScCameraControlOrbit* ctrl,
    const ScCameraControlOrbitUpdateInfo* update_info,
    ScPerspectiveCamera* dst_camera
) {
    // Inputs.
    if (!update_info->common.input_captured) {
        // Zoom.
        if (ctrl->mouse_wheel < 0.0f) {
            ctrl->world_target_distance_end *= 1.25f;
            ctrl->world_target_distance_end =
                fminf(ctrl->world_target_distance_end, ctrl->world_target_max_distance);
        } else if (ctrl->mouse_wheel > 0.0f) {
            ctrl->world_target_distance_end *= 0.75f;
            ctrl->world_target_distance_end =
                fmaxf(ctrl->world_target_distance_end, ctrl->world_target_min_distance);
        }
        ctrl->mouse_wheel = 0.0f;

        // Orientation.
        if (ctrl->mouse_left) {
            const float screen_max_extent =
                fmaxf(update_info->common.screen_width, update_info->common.screen_height);
            const float mouse_x = ctrl->mouse_motion.x / screen_max_extent;
            const float mouse_y = ctrl->mouse_motion.y / screen_max_extent;
            ctrl->orbit_turn_horizontal_end -= mouse_x * ctrl->orbit_turn_horizontal_speed;
            ctrl->orbit_turn_vertical_end += mouse_y * ctrl->orbit_turn_horizontal_speed;
            ctrl->orbit_turn_vertical_end =
                fminf(ctrl->orbit_turn_vertical_end, ctrl->orbit_turn_vertical_max);
            ctrl->orbit_turn_vertical_end =
                fmaxf(ctrl->orbit_turn_vertical_end, ctrl->orbit_turn_vertical_min);
            SC_LOG_INFO("%f", ctrl->orbit_turn_vertical_end);
        }

        // Pan.
        if (ctrl->mouse_right) {
            const float screen_max_extent =
                fmaxf(update_info->common.screen_width, update_info->common.screen_height);
            const float mouse_x = ctrl->mouse_motion.x / screen_max_extent;
            const float mouse_y = ctrl->mouse_motion.y / screen_max_extent;
            const vec3f forward = (vec3f) {
                cosf(ctrl->orbit_turn_vertical * SC_PI) * cosf(ctrl->orbit_turn_horizontal * SC_PI),
                cosf(ctrl->orbit_turn_vertical * SC_PI) * sinf(ctrl->orbit_turn_horizontal * SC_PI),
                sinf(ctrl->orbit_turn_vertical * SC_PI),
            };
            const vec3f right = vec3f_cross(forward, ctrl->world_up);
            const vec3f up = vec3f_cross(right, forward);
            const float r_scale = mouse_x * update_info->common.delta_time
                * ctrl->world_target_distance * ctrl->world_target_pan_speed;
            const float u_scale = mouse_y * update_info->common.delta_time
                * ctrl->world_target_distance * ctrl->world_target_pan_speed;
            ctrl->world_target_end = vec3f_add(
                ctrl->world_target_end,
                vec3f_add(vec3f_scale(right, r_scale), vec3f_scale(up, u_scale))
            );
        }

        ctrl->mouse_motion = vec2f_new(0.0f, 0.0f);
    }

    // Interpolate.
    ctrl->orbit_turn_horizontal = explerpf(
        ctrl->orbit_turn_horizontal,
        ctrl->orbit_turn_horizontal_end,
        ctrl->orbit_turn_horizontal_rate,
        update_info->common.delta_time
    );
    ctrl->orbit_turn_vertical = explerpf(
        ctrl->orbit_turn_vertical,
        ctrl->orbit_turn_vertical_end,
        ctrl->orbit_turn_vertical_rate,
        update_info->common.delta_time
    );
    ctrl->world_target_distance = explerpf(
        ctrl->world_target_distance,
        ctrl->world_target_distance_end,
        ctrl->world_target_distance_rate,
        update_info->common.delta_time
    );
    ctrl->world_target = vec3f_explerp(
        ctrl->world_target,
        ctrl->world_target_end,
        ctrl->world_target_rate,
        update_info->common.delta_time
    );

    // Unpack.
    const float screen_width = update_info->common.screen_width;
    const float screen_height = update_info->common.screen_height;
    const float field_of_view = update_info->common.field_of_view;
    const float clip_distance_near = update_info->common.clip_distance_near;
    const float clip_distance_far = update_info->common.clip_distance_far;
    const float world_target_distance = ctrl->world_target_distance;
    const vec3f world_target = ctrl->world_target;
    const vec3f world_up = ctrl->world_up;

    // World position.
    const vec3f world_target_direction = (vec3f) {
        cosf(ctrl->orbit_turn_vertical * SC_PI) * cosf(ctrl->orbit_turn_horizontal * SC_PI),
        cosf(ctrl->orbit_turn_vertical * SC_PI) * sinf(ctrl->orbit_turn_horizontal * SC_PI),
        sinf(ctrl->orbit_turn_vertical * SC_PI),
    };
    const vec3f world_target_offset = vec3f_scale(world_target_direction, world_target_distance);
    const vec3f world_position = vec3f_add(world_target, world_target_offset);

    // Update.
    *dst_camera = sc_perspective_camera_new(&(ScPerspectiveCameraCreateInfo) {
        .screen_width = screen_width,
        .screen_height = screen_height,
        .field_of_view = field_of_view,
        .clip_distance_near = clip_distance_near,
        .clip_distance_far = clip_distance_far,
        .world_position = world_position,
        .world_target = world_target,
        .world_up = world_up,
    });
}

//
// Camera control - autoplay
//

typedef struct ScCameraControlAutoplayCreateInfo {
    ScCameraControlCommonCreateInfo common;
} ScCameraControlAutoplayCreateInfo;

typedef struct ScCameraControlAutoplayUpdateInfo {
    ScCameraControlCommonUpdateInfo common;
} ScCameraControlAutoplayUpdateInfo;

typedef struct ScCameraControlAutoplay {
    float turn_min_radius;
    float turn_radius;
    float turn_speed;
    vec3f world_target;
    vec3f world_up;
    float time;
} ScCameraControlAutoplay;

static ScCameraControlAutoplay
sc_camera_control_autoplay_new(const ScCameraControlAutoplayCreateInfo* create_info) {
    const vec3f scene_extents = box3f_extents(create_info->common.scene_bounds);
    const vec3f scene_center = box3f_center(create_info->common.scene_bounds);
    const float scene_max_extent = vec3f_component_max(scene_extents);
    return (ScCameraControlAutoplay) {
        .turn_min_radius = 64.0f,
        .turn_radius = scene_max_extent,
        .turn_speed = 0.25f,
        .world_target = scene_center,
        .world_up = (vec3f) {0.0f, 0.0f, 1.0f},
    };
}

static void sc_camera_control_autoplay_event(ScCameraControlAutoplay* ctrl, SDL_Event* event) {
    // Note: This camera cannot be controlled by the user.
    SC_UNUSED(ctrl);
    SC_UNUSED(event);
}

static void sc_camera_control_autoplay_update(
    ScCameraControlAutoplay* ctrl,
    const ScCameraControlAutoplayUpdateInfo* update_info,
    ScPerspectiveCamera* dst_camera
) {
    // Update.
    ctrl->time += update_info->common.delta_time;

    // Unpack.
    const float turn_min_radius = ctrl->turn_min_radius;
    const float turn_radius = ctrl->turn_radius;
    const float turn_speed = ctrl->turn_speed;
    const float screen_width = update_info->common.screen_width;
    const float screen_height = update_info->common.screen_height;
    const float field_of_view = update_info->common.field_of_view;
    const float clip_distance_near = update_info->common.clip_distance_near;
    const float clip_distance_far = update_info->common.clip_distance_far;
    const vec3f world_target = ctrl->world_target;
    const vec3f world_up = ctrl->world_up;
    const float time = ctrl->time;

    // World position.
    const float world_target_offset_radius =
        turn_min_radius + turn_radius * (0.5f + 0.5f * cosf(33.333f + 0.5f * time));
    const vec3f world_target_offset = (vec3f) {
        world_target_offset_radius * cosf(turn_speed * time),
        world_target_offset_radius * sinf(turn_speed * time),
        world_target_offset_radius * 0.5f,
    };
    const vec3f world_position = vec3f_add(world_target, world_target_offset);

    // Update.
    *dst_camera = sc_perspective_camera_new(&(ScPerspectiveCameraCreateInfo) {
        .screen_width = screen_width,
        .screen_height = screen_height,
        .field_of_view = field_of_view,
        .clip_distance_near = clip_distance_near,
        .clip_distance_far = clip_distance_far,
        .world_position = world_position,
        .world_target = world_target,
        .world_up = world_up,
    });
}

//
// Camera control - aerial
//

typedef struct ScCameraControlAerialCreateInfo {
    ScCameraControlCommonCreateInfo common;
} ScCameraControlAerialCreateInfo;

typedef struct ScCameraControlAerialUpdateInfo {
    ScCameraControlCommonUpdateInfo common;
    vec3f world_target;
} ScCameraControlAerialUpdateInfo;

typedef struct ScCameraControlAerial {
    vec3f world_target_offset;
    vec3f world_up;
} ScCameraControlAerial;

static ScCameraControlAerial
sc_camera_control_aerial_new(const ScCameraControlAerialCreateInfo* create_info) {
    const vec3f scene_extents = box3f_extents(create_info->common.scene_bounds);
    const float scene_max_extent = vec3f_component_max(scene_extents);
    return (ScCameraControlAerial) {
        .world_target_offset = (vec3f) {0.0f, 0.0f, 1.5f * scene_max_extent},
        .world_up = (vec3f) {0.0f, 1.0f, 0.0f},
    };
}

static void sc_camera_control_aerial_event(ScCameraControlAerial* ctrl, SDL_Event* event) {
    // Note: This camera cannot be controlled by the user.
    SC_UNUSED(ctrl);
    SC_UNUSED(event);
}

static void sc_camera_control_aerial_update(
    ScCameraControlAerial* ctrl,
    const ScCameraControlAerialUpdateInfo* update_info,
    ScPerspectiveCamera* dst_camera
) {
    // Unpack.
    const float screen_width = update_info->common.screen_width;
    const float screen_height = update_info->common.screen_height;
    const float field_of_view = update_info->common.field_of_view;
    const float clip_distance_near = update_info->common.clip_distance_near;
    const float clip_distance_far = update_info->common.clip_distance_far;
    const vec3f world_target = update_info->world_target;
    const vec3f world_target_offset = ctrl->world_target_offset;
    const vec3f world_up = ctrl->world_up;

    // World position.
    const vec3f world_position = vec3f_add(world_target, world_target_offset);

    // Update.
    *dst_camera = sc_perspective_camera_new(&(ScPerspectiveCameraCreateInfo) {
        .screen_width = screen_width,
        .screen_height = screen_height,
        .field_of_view = field_of_view,
        .clip_distance_near = clip_distance_near,
        .clip_distance_far = clip_distance_far,
        .world_position = world_position,
        .world_target = world_target,
        .world_up = world_up,
    });
}
