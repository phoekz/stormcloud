/*

# Conventions

- Scalar prefixes: c, uc, s, us, i, ui, l, ul, f, d
- Vectors: vec2f, vec3i
- Matrices: mat4f, mat2x3f
- Geometry: point3f, line3f, rect2f, box3f, plane3f, ray3f

*/

//
// Common
//

#define SC_PI SDL_PI_F

static SC_INLINE float rad_from_deg(float deg) {
    return deg * (SC_PI / 180.0f);
}

static SC_INLINE float deg_from_rad(float rad) {
    return rad * (180.0f / SC_PI);
}

//
// Types
//

typedef struct vec2f {
    float x, y;
} vec2f;

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

typedef struct box3f {
    vec3f mn;
    vec3f mx;
} box3f;

typedef struct sphere3f {
    vec3f o;
    float r;
} sphere3f;

typedef struct plane3f {
    vec3f n;
    float d;
} plane3f;

//
// Scalar
//

static SC_INLINE float lerpf(float a, float b, float t) {
    return (1.0f - t) * a + t * b;
}

static SC_INLINE float explerpf(float a, float b, float rate, float dt) {
    return lerpf(b, a, exp2f(-rate * dt));
}

//
// Vector
//

static SC_INLINE vec2f vec2f_new(float x, float y) {
    return (vec2f) {
        x,
        y,
    };
}

static SC_INLINE vec3f vec3f_new(float x, float y, float z) {
    return (vec3f) {
        x,
        y,
        z,
    };
}

static SC_INLINE vec3f vec3f_new_x(float x) {
    return (vec3f) {
        x,
        0.0f,
        0.0f,
    };
}

static SC_INLINE vec3f vec3f_new_y(float y) {
    return (vec3f) {
        0.0f,
        y,
        0.0f,
    };
}

static SC_INLINE vec3f vec3f_new_z(float z) {
    return (vec3f) {
        0.0f,
        0.0f,
        z,
    };
}

static SC_INLINE vec3f vec3f_neg(vec3f vec) {
    return (vec3f) {
        -vec.x,
        -vec.y,
        -vec.z,
    };
}

static SC_INLINE vec3f vec3f_add(vec3f lhs, vec3f rhs) {
    return (vec3f) {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
}

static SC_INLINE vec3f vec3f_sub(vec3f lhs, vec3f rhs) {
    return (vec3f) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

static SC_INLINE vec3f vec3f_scale(vec3f lhs, float rhs) {
    return (vec3f) {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
    };
}

static SC_INLINE float vec3f_len(vec3f vec) {
    return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

static SC_INLINE vec3f vec3f_normalize(vec3f vec) {
    const float length = vec3f_len(vec);
    return (vec3f) {
        vec.x / length,
        vec.y / length,
        vec.z / length,
    };
}

static SC_INLINE float vec3f_dot(vec3f lhs, vec3f rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

static SC_INLINE vec3f vec3f_cross(vec3f lhs, vec3f rhs) {
    return (vec3f) {
        lhs.y * rhs.z - rhs.y * lhs.z,
        -(lhs.x * rhs.z - rhs.x * lhs.z),
        lhs.x * rhs.y - rhs.x * lhs.y,
    };
}

static SC_INLINE float vec3f_component_min(vec3f vec) {
    return SDL_min(SDL_min(vec.x, vec.y), vec.z);
}

static SC_INLINE float vec3f_component_max(vec3f vec) {
    return SDL_max(SDL_max(vec.x, vec.y), vec.z);
}

static SC_INLINE bool vec3f_isfinite(vec3f vec) {
    return isfinite(vec.x) && isfinite(vec.y) && isfinite(vec.z);
}

static SC_INLINE vec3f vec3f_lerp(vec3f a, vec3f b, float t) {
    return (vec3f) {
        lerpf(a.x, b.x, t),
        lerpf(a.y, b.y, t),
        lerpf(a.z, b.z, t),
    };
}

static SC_INLINE vec3f vec3f_explerp(vec3f a, vec3f b, float rate, float dt) {
    return (vec3f) {
        explerpf(a.x, b.x, rate, dt),
        explerpf(a.y, b.y, rate, dt),
        explerpf(a.z, b.z, rate, dt),
    };
}

static SC_INLINE vec4f vec4f_new(float x, float y, float z, float w) {
    return (vec4f) {
        x,
        y,
        z,
        w,
    };
}

static SC_INLINE vec4f vec4f_add(vec4f lhs, vec4f rhs) {
    return (vec4f) {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
        lhs.w + rhs.w,
    };
}

static SC_INLINE vec4f vec4f_sub(vec4f lhs, vec4f rhs) {
    return (vec4f) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
        lhs.w - rhs.w,
    };
}

static SC_INLINE vec3f vec3f_from_vec4f(vec4f vec) {
    return (vec3f) {
        vec.x,
        vec.y,
        vec.z,
    };
}

static SC_INLINE vec4f vec4f_from_vec3f(vec3f lhs, float rhs) {
    return (vec4f) {
        lhs.x,
        lhs.y,
        lhs.z,
        rhs,
    };
}

static SC_INLINE vec4f vec4f_scale(vec4f lhs, float rhs) {
    return (vec4f) {
        lhs.x * rhs,
        lhs.y * rhs,
        lhs.z * rhs,
        lhs.w * rhs,
    };
}

static SC_INLINE float vec4f_dot(vec4f lhs, vec4f rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
}

//
// Matrix
//

static SC_INLINE mat4f mat4f_identity() {
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

static SC_INLINE vec4f mat4f_col(mat4f m, uint8_t col) {
    SC_ASSERT(col < 4);
    switch (col) {
        case 0: return (vec4f) {m.m00, m.m01, m.m02, m.m03};
        case 1: return (vec4f) {m.m10, m.m11, m.m12, m.m13};
        case 2: return (vec4f) {m.m20, m.m21, m.m22, m.m23};
        case 3: return (vec4f) {m.m30, m.m31, m.m32, m.m33};
        default: return (vec4f) {0.0f, 0.0f, 0.0f, 0.0f};
    }
}

static SC_INLINE vec4f mat4f_row(mat4f m, uint8_t row) {
    SC_ASSERT(row < 4);
    switch (row) {
        case 0: return (vec4f) {m.m00, m.m10, m.m20, m.m30};
        case 1: return (vec4f) {m.m01, m.m11, m.m21, m.m31};
        case 2: return (vec4f) {m.m02, m.m12, m.m22, m.m32};
        case 3: return (vec4f) {m.m03, m.m13, m.m23, m.m33};
        default: return (vec4f) {0.0f, 0.0f, 0.0f, 0.0f};
    }
}

static SC_INLINE vec4f mat4f_mul_vec4f(mat4f m, vec4f v) {
    return (vec4f) {
        m.m00 * v.x + m.m10 * v.y + m.m20 * v.z + m.m30 * v.w,
        m.m01 * v.x + m.m11 * v.y + m.m21 * v.z + m.m31 * v.w,
        m.m02 * v.x + m.m12 * v.y + m.m22 * v.z + m.m32 * v.w,
        m.m03 * v.x + m.m13 * v.y + m.m23 * v.z + m.m33 * v.w,
    };
}

static SC_INLINE mat4f mat4f_mul(mat4f lhs, mat4f rhs) {
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

static SC_INLINE mat4f mat4f_scale(mat4f m, float s) {
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

static SC_INLINE mat4f mat4f_transpose(mat4f m) {
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

static SC_INLINE mat4f mat4f_inverse(mat4f m) {
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

static SC_INLINE mat4f mat4f_perspective(float fov, float aspect, float znear, float zfar) {
    const float focal_length = 1.0f / tanf(fov * 0.5f);
    const float a = focal_length / aspect;
    const float b = focal_length;
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

static SC_INLINE mat4f mat4f_lookat(vec3f eye, vec3f center, vec3f global_up) {
    const vec3f forward = vec3f_normalize((vec3f) {
        center.x - eye.x,
        center.y - eye.y,
        center.z - eye.z,
    });
    const vec3f right = vec3f_normalize(vec3f_cross(forward, global_up));
    const vec3f up = vec3f_normalize(vec3f_cross(right, forward));
    const float tx = -vec3f_dot(right, eye);
    const float ty = -vec3f_dot(up, eye);
    const float tz = vec3f_dot(forward, eye);
    return (mat4f) {
        right.x,
        up.x,
        -forward.x,
        0.0f,
        right.y,
        up.y,
        -forward.y,
        0.0f,
        right.z,
        up.z,
        -forward.z,
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

static SC_INLINE vec3f box3f_extents(box3f box) {
    return (vec3f) {
        box.mx.x - box.mn.x,
        box.mx.y - box.mn.y,
        box.mx.z - box.mn.z,
    };
}

static SC_INLINE vec3f box3f_center(box3f box) {
    return (vec3f) {
        (box.mn.x + box.mx.x) * 0.5f,
        (box.mn.y + box.mx.y) * 0.5f,
        (box.mn.z + box.mx.z) * 0.5f,
    };
}

static SC_INLINE bool box3f_contains(box3f box, vec3f point) {
    return point.x >= box.mn.x && point.y >= box.mn.y && point.z >= box.mn.z && point.x <= box.mx.x
        && point.y <= box.mx.y && point.z <= box.mx.z;
}

static SC_INLINE sphere3f sphere3f_from_box3f(box3f box) {
    const vec3f origin = box3f_center(box);
    const vec3f extents = box3f_extents(box);
    const float radius = vec3f_len(extents) * 0.5f;
    return (sphere3f) {
        .o = origin,
        .r = radius,
    };
}

static SC_INLINE plane3f plane3f_new(vec3f n, float d) {
    return (plane3f) {
        .n = n,
        .d = d,
    };
}

static SC_INLINE plane3f plane3f_normalize(plane3f plane) {
    const float length = vec3f_len(plane.n);
    return (plane3f) {
        .n = vec3f_scale(plane.n, 1.0f / length),
        .d = plane.d / length,
    };
}

static SC_INLINE plane3f plane3f_from_vec4f(vec4f vec) {
    return plane3f_normalize((plane3f) {
        .n = (vec3f) {vec.x, vec.y, vec.z},
        .d = vec.w,
    });
}

static SC_INLINE vec4f vec4f_from_plane3f(plane3f plane) {
    return (vec4f) {
        plane.n.x,
        plane.n.y,
        plane.n.z,
        plane.d,
    };
}

//
// Morton codes
//

static SC_INLINE uint16_t morton2_encode16(uint8_t x, uint8_t y) {
    uint32_t res = x | ((uint32_t)y << 16);
    res = (res | (res << 4)) & 0x0f0f0f0f;
    res = (res | (res << 2)) & 0x33333333;
    res = (res | (res << 1)) & 0x55555555;
    return (uint16_t)(res | (res >> 15));
}

static SC_INLINE uint32_t morton2_encode32(uint16_t x, uint16_t y) {
    uint64_t res = x | ((uint64_t)y << 32);
    res = (res | (res << 8)) & 0x00ff00ff00ff00ff;
    res = (res | (res << 4)) & 0x0f0f0f0f0f0f0f0f;
    res = (res | (res << 2)) & 0x3333333333333333;
    res = (res | (res << 1)) & 0x5555555555555555;
    return (uint32_t)(res | (res >> 31));
}

static SC_INLINE void morton2_decode16(uint8_t* x, uint8_t* y, uint16_t mc) {
    uint32_t res = (mc | ((uint32_t)(mc) << 15)) & 0x55555555;
    res = (res | (res >> 1)) & 0x33333333;
    res = (res | (res >> 2)) & 0x0f0f0f0f;
    res = res | (res >> 4);
    *x = (uint8_t)(res);
    *y = (uint8_t)(res >> 16);
}

static SC_INLINE void morton2_decode32(uint16_t* x, uint16_t* y, uint32_t mc) {
    uint64_t res = (mc | ((uint64_t)mc << 31)) & 0x5555555555555555;
    res = (res | (res >> 1)) & 0x3333333333333333;
    res = (res | (res >> 2)) & 0x0f0f0f0f0f0f0f0f;
    res = (res | (res >> 4)) & 0x00ff00ff00ff00ff;
    res = res | (res >> 8);
    *x = (uint16_t)res;
    *y = (uint16_t)(res >> 32);
}
