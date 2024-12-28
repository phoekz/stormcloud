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

typedef struct vec4f {
    float x, y, z, w;
} vec4f;

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

typedef struct sphere3f {
    vec3f o;
    float r;
} sphere3f;

typedef struct plane3f {
    vec3f n;
    float d;
} plane3f;

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

static vec3f vec3f_scale(vec3f lhs, float rhs) {
    return (vec3f) {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
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

static vec4f vec4f_add(vec4f lhs, vec4f rhs) {
    return (vec4f) {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
        lhs.w + rhs.w,
    };
}

static vec4f vec4f_sub(vec4f lhs, vec4f rhs) {
    return (vec4f) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
        lhs.w - rhs.w,
    };
}

static vec3f vec3f_from_vec4f(vec4f vec) {
    return (vec3f) {
        vec.x,
        vec.y,
        vec.z,
    };
}

static vec4f vec4f_from_vec3f(vec3f lhs, float rhs) {
    return (vec4f) {
        lhs.x,
        lhs.y,
        lhs.z,
        rhs,
    };
}

static vec4f vec4f_scale(vec4f lhs, float rhs) {
    return (vec4f) {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
        lhs.w * rhs,
    };
}

static float vec4f_dot(vec4f lhs, vec4f rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
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

static vec4f mat4f_mul_vec4f(mat4f m, vec4f v) {
    return (vec4f) {
        m.m00 * v.x + m.m10 * v.y + m.m20 * v.z + m.m30 * v.w,
        m.m01 * v.x + m.m11 * v.y + m.m21 * v.z + m.m31 * v.w,
        m.m02 * v.x + m.m12 * v.y + m.m22 * v.z + m.m32 * v.w,
        m.m03 * v.x + m.m13 * v.y + m.m23 * v.z + m.m33 * v.w,
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

static mat4f mat4f_scale(mat4f m, float s) {
    return (mat4f) {
        m.m00 * s,
        m.m01 * s,
        m.m02 * s,
        m.m03 * s,
        m.m10 * s,
        m.m11 * s,
        m.m12 * s,
        m.m13 * s,
        m.m20 * s,
        m.m21 * s,
        m.m22 * s,
        m.m23 * s,
        m.m30 * s,
        m.m31 * s,
        m.m32 * s,
        m.m33 * s,
    };
}

static mat4f mat4f_transpose(mat4f m) {
    return (mat4f) {
        m.m00,
        m.m10,
        m.m20,
        m.m30,
        m.m01,
        m.m11,
        m.m21,
        m.m31,
        m.m02,
        m.m12,
        m.m22,
        m.m32,
        m.m03,
        m.m13,
        m.m23,
        m.m33,
    };
}

static mat4f mat4f_inverse(mat4f m) {
    const float t0 = m.m22 * m.m33 - m.m32 * m.m23;
    const float t1 = m.m21 * m.m33 - m.m31 * m.m23;
    const float t2 = m.m21 * m.m32 - m.m31 * m.m22;
    const float t3 = m.m20 * m.m33 - m.m30 * m.m23;
    const float t4 = m.m20 * m.m32 - m.m30 * m.m22;
    const float t5 = m.m20 * m.m31 - m.m30 * m.m21;
    const float t6 = m.m12 * m.m33 - m.m32 * m.m13;
    const float t7 = m.m11 * m.m33 - m.m31 * m.m13;
    const float t8 = m.m11 * m.m32 - m.m31 * m.m12;
    const float t9 = m.m12 * m.m23 - m.m22 * m.m13;
    const float t10 = m.m11 * m.m23 - m.m21 * m.m13;
    const float t11 = m.m11 * m.m22 - m.m21 * m.m12;
    const float t12 = m.m10 * m.m33 - m.m30 * m.m13;
    const float t13 = m.m10 * m.m32 - m.m30 * m.m12;
    const float t14 = m.m10 * m.m23 - m.m20 * m.m13;
    const float t15 = m.m10 * m.m22 - m.m20 * m.m12;
    const float t16 = m.m10 * m.m31 - m.m30 * m.m11;
    const float t17 = m.m10 * m.m21 - m.m20 * m.m11;

    mat4f r;
    r.m00 = m.m11 * t0 - m.m12 * t1 + m.m13 * t2;
    r.m01 = -(m.m01 * t0 - m.m02 * t1 + m.m03 * t2);
    r.m02 = m.m01 * t6 - m.m02 * t7 + m.m03 * t8;
    r.m03 = -(m.m01 * t9 - m.m02 * t10 + m.m03 * t11);
    r.m10 = -(m.m10 * t0 - m.m12 * t3 + m.m13 * t4);
    r.m11 = m.m00 * t0 - m.m02 * t3 + m.m03 * t4;
    r.m12 = -(m.m00 * t6 - m.m02 * t12 + m.m03 * t13);
    r.m13 = m.m00 * t9 - m.m02 * t14 + m.m03 * t15;
    r.m20 = m.m10 * t1 - m.m11 * t3 + m.m13 * t5;
    r.m21 = -(m.m00 * t1 - m.m01 * t3 + m.m03 * t5);
    r.m22 = m.m00 * t7 - m.m01 * t12 + m.m03 * t16;
    r.m23 = -(m.m00 * t10 - m.m01 * t14 + m.m03 * t17);
    r.m30 = -(m.m10 * t2 - m.m11 * t4 + m.m12 * t5);
    r.m31 = m.m00 * t2 - m.m01 * t4 + m.m02 * t5;
    r.m32 = -(m.m00 * t8 - m.m01 * t13 + m.m02 * t16);
    r.m33 = m.m00 * t11 - m.m01 * t15 + m.m02 * t17;
    const float det = 1.0f / (m.m00 * r.m00 + m.m01 * r.m10 + m.m02 * r.m20 + m.m03 * r.m30);
    return mat4f_scale(r, det);
}

static mat4f mat4f_perspective(float fov, float aspect, float znear, float zfar) {
    const float num = 1.0f / SDL_tanf(fov * 0.5f);
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
// Geometry
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

static bool bounds3f_contains(bounds3f bounds, vec3f point) {
    return point.x >= bounds.mn.x && point.y >= bounds.mn.y && point.z >= bounds.mn.z
        && point.x <= bounds.mx.x && point.y <= bounds.mx.y && point.z <= bounds.mx.z;
}

static sphere3f sphere3f_from_bounds3f(bounds3f bounds) {
    const vec3f origin = bounds3f_center(bounds);
    const vec3f extents = bounds3f_extents(bounds);
    const float radius = vec3f_len(extents) * 0.5f;
    return (sphere3f) {
        .o = origin,
        .r = radius,
    };
}

static plane3f plane3f_new(vec3f n, float d) {
    return (plane3f) {
        .n = n,
        .d = d,
    };
}

static plane3f plane3f_normalize(plane3f plane) {
    const float length = vec3f_len(plane.n);
    return (plane3f) {
        .n = vec3f_scale(plane.n, 1.0f / length),
        .d = plane.d / length,
    };
}

static plane3f plane3f_from_vec4f(vec4f vec) {
    return (plane3f) {
        .n = (vec3f) {vec.x, vec.y, vec.z},
        .d = vec.w,
    };
}

static vec4f vec4f_from_plane3f(plane3f plane) {
    return (vec4f) {
        plane.n.x,
        plane.n.y,
        plane.n.z,
        plane.d,
    };
}
