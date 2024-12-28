/*

# Conventions

- Scalar prefixes: c, uc, s, us, i, ui, l, ul, f, d
- Vectors: vec2f, vec3i
- Matrices: mat4f, mat2x3f
- Geometry: point3f, line3f, bounds2f, bounds3f, plane3f, ray3f

*/

//
// Common
//

#define SC_PI SDL_PI_F

static float rad_from_deg(float deg) {
    return deg * (SC_PI / 180.0f);
}

static float deg_from_rad(float rad) {
    return rad * (180.0f / SC_PI);
}

//
// Types
//

typedef struct vec3f {
    float x, y, z;
} vec3f;

typedef struct mat4f {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
} mat4f;

typedef struct bounds3f {
    vec3f mn;
    vec3f mx;
} bounds3f;

//
// Vector
//

static vec3f vec3f_add(vec3f lhs, vec3f rhs) {
    return (vec3f) {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
}

static vec3f vec3f_sub(vec3f lhs, vec3f rhs) {
    return (vec3f) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

static float vec3f_len(vec3f vec) {
    return SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

static vec3f vec3f_normalize(vec3f vec) {
    const float length = vec3f_len(vec);
    return (vec3f) {
        vec.x / length,
        vec.y / length,
        vec.z / length,
    };
}

static float vec3f_dot(vec3f lhs, vec3f rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

static vec3f vec3f_cross(vec3f lhs, vec3f rhs) {
    return (vec3f) {
        lhs.y * rhs.z - rhs.y * lhs.z,
        -(lhs.x * rhs.z - rhs.x * lhs.z),
        lhs.x * rhs.y - rhs.x * lhs.y,
    };
}

//
// Matrix
//

static mat4f mat4f_identity() {
    return (mat4f) {
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };
}

static mat4f mat4f_mul(mat4f lhs, mat4f rhs) {
    mat4f r;
    r.m00 = lhs.m00 * rhs.m00 + lhs.m10 * rhs.m01 + lhs.m20 * rhs.m02 + lhs.m30 * rhs.m03;
    r.m01 = lhs.m01 * rhs.m00 + lhs.m11 * rhs.m01 + lhs.m21 * rhs.m02 + lhs.m31 * rhs.m03;
    r.m02 = lhs.m02 * rhs.m00 + lhs.m12 * rhs.m01 + lhs.m22 * rhs.m02 + lhs.m32 * rhs.m03;
    r.m03 = lhs.m03 * rhs.m00 + lhs.m13 * rhs.m01 + lhs.m23 * rhs.m02 + lhs.m33 * rhs.m03;
    r.m10 = lhs.m00 * rhs.m10 + lhs.m10 * rhs.m11 + lhs.m20 * rhs.m12 + lhs.m30 * rhs.m13;
    r.m11 = lhs.m01 * rhs.m10 + lhs.m11 * rhs.m11 + lhs.m21 * rhs.m12 + lhs.m31 * rhs.m13;
    r.m12 = lhs.m02 * rhs.m10 + lhs.m12 * rhs.m11 + lhs.m22 * rhs.m12 + lhs.m32 * rhs.m13;
    r.m13 = lhs.m03 * rhs.m10 + lhs.m13 * rhs.m11 + lhs.m23 * rhs.m12 + lhs.m33 * rhs.m13;
    r.m20 = lhs.m00 * rhs.m20 + lhs.m10 * rhs.m21 + lhs.m20 * rhs.m22 + lhs.m30 * rhs.m23;
    r.m21 = lhs.m01 * rhs.m20 + lhs.m11 * rhs.m21 + lhs.m21 * rhs.m22 + lhs.m31 * rhs.m23;
    r.m22 = lhs.m02 * rhs.m20 + lhs.m12 * rhs.m21 + lhs.m22 * rhs.m22 + lhs.m32 * rhs.m23;
    r.m23 = lhs.m03 * rhs.m20 + lhs.m13 * rhs.m21 + lhs.m23 * rhs.m22 + lhs.m33 * rhs.m23;
    r.m30 = lhs.m00 * rhs.m30 + lhs.m10 * rhs.m31 + lhs.m20 * rhs.m32 + lhs.m30 * rhs.m33;
    r.m31 = lhs.m01 * rhs.m30 + lhs.m11 * rhs.m31 + lhs.m21 * rhs.m32 + lhs.m31 * rhs.m33;
    r.m32 = lhs.m02 * rhs.m30 + lhs.m12 * rhs.m31 + lhs.m22 * rhs.m32 + lhs.m32 * rhs.m33;
    r.m33 = lhs.m03 * rhs.m30 + lhs.m13 * rhs.m31 + lhs.m23 * rhs.m32 + lhs.m33 * rhs.m33;
    return r;
}

static mat4f mat4f_perspective(float fov, float aspect, float znear, float zfar) {
    const float num = 1.0f / ((float)SDL_tanf(fov * 0.5f));
    const float a = num / aspect;
    const float b = num;
    const float c = zfar / (znear - zfar);
    const float d = (znear * zfar) / (znear - zfar);
    return (mat4f) {
        a,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        b,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        c,
        -1.0f,
        0.0f,
        0.0f,
        d,
        0.0f,
    };
}

static mat4f mat4f_lookat(vec3f origin, vec3f target, vec3f up) {
    const vec3f fwd = vec3f_normalize((vec3f) {
        origin.x - target.x,
        origin.y - target.y,
        origin.z - target.z,
    });
    const vec3f right = vec3f_normalize(vec3f_cross(up, fwd));
    const vec3f up_corrected = vec3f_cross(fwd, right);
    const float tx = -vec3f_dot(right, origin);
    const float ty = -vec3f_dot(up_corrected, origin);
    const float tz = -vec3f_dot(fwd, origin);
    return (mat4f) {
        right.x,
        up_corrected.x,
        fwd.x,
        0.0f,
        right.y,
        up_corrected.y,
        fwd.y,
        0.0f,
        right.z,
        up_corrected.z,
        fwd.z,
        0.0f,
        tx,
        ty,
        tz,
        1.0f,
    };
}

//
// Bounds
//

static vec3f bounds3f_extents(bounds3f bounds) {
    return (vec3f) {
        bounds.mx.x - bounds.mn.x,
        bounds.mx.y - bounds.mn.y,
        bounds.mx.z - bounds.mn.z,
    };
}

static vec3f bounds3f_center(bounds3f bounds) {
    return (vec3f) {
        (bounds.mn.x + bounds.mx.x) * 0.5f,
        (bounds.mn.y + bounds.mx.y) * 0.5f,
        (bounds.mn.z + bounds.mx.z) * 0.5f,
    };
}
