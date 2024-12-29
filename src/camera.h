static bool
sc_frustum_box_test(const plane3f frustum_planes[6], const vec3f frustum_corners[8], box3f box) {
    // From: https://iquilezles.org/articles/frustumcorrect/

    // Init.
    uint32_t count;

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
