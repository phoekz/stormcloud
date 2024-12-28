#define SC_PI SDL_PI_F

static float sc_rad_from_deg(float deg) {
    return deg * (SC_PI / 180.0f);
}

typedef struct ScVector3 {
    float x, y, z;
} ScVector3;

typedef struct ScMatrix4x4 {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
} ScMatrix4x4;

typedef struct ScBounds3 {
    ScVector3 min;
    ScVector3 max;
} ScBounds3;

static ScVector3 sc_vector3_add(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
}

static ScVector3 sc_vector3_sub(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
}

static float sc_vector3_length(ScVector3 vec) {
    return SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

static ScVector3 sc_vector3_normalized(ScVector3 vec) {
    const float length = sc_vector3_length(vec);
    return (ScVector3) {
        vec.x / length,
        vec.y / length,
        vec.z / length,
    };
}

static float sc_vector3_dot(ScVector3 lhs, ScVector3 rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

static ScVector3 sc_vector3_cross(ScVector3 lhs, ScVector3 rhs) {
    return (ScVector3) {
        lhs.y * rhs.z - rhs.y * lhs.z,
        -(lhs.x * rhs.z - rhs.x * lhs.z),
        lhs.x * rhs.y - rhs.x * lhs.y,
    };
}

static ScMatrix4x4 sc_matrix4x4_identity() {
    // clang-format off
    return (ScMatrix4x4) {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    // clang-format on
}

static ScMatrix4x4 sc_matrix4x4_multiply(ScMatrix4x4 lhs, ScMatrix4x4 rhs) {
    ScMatrix4x4 m;
    m.m00 = (lhs.m00 * rhs.m00) + (lhs.m01 * rhs.m10) + (lhs.m02 * rhs.m20) + (lhs.m03 * rhs.m30);
    m.m01 = (lhs.m00 * rhs.m01) + (lhs.m01 * rhs.m11) + (lhs.m02 * rhs.m21) + (lhs.m03 * rhs.m31);
    m.m02 = (lhs.m00 * rhs.m02) + (lhs.m01 * rhs.m12) + (lhs.m02 * rhs.m22) + (lhs.m03 * rhs.m32);
    m.m03 = (lhs.m00 * rhs.m03) + (lhs.m01 * rhs.m13) + (lhs.m02 * rhs.m23) + (lhs.m03 * rhs.m33);
    m.m10 = (lhs.m10 * rhs.m00) + (lhs.m11 * rhs.m10) + (lhs.m12 * rhs.m20) + (lhs.m13 * rhs.m30);
    m.m11 = (lhs.m10 * rhs.m01) + (lhs.m11 * rhs.m11) + (lhs.m12 * rhs.m21) + (lhs.m13 * rhs.m31);
    m.m12 = (lhs.m10 * rhs.m02) + (lhs.m11 * rhs.m12) + (lhs.m12 * rhs.m22) + (lhs.m13 * rhs.m32);
    m.m13 = (lhs.m10 * rhs.m03) + (lhs.m11 * rhs.m13) + (lhs.m12 * rhs.m23) + (lhs.m13 * rhs.m33);
    m.m20 = (lhs.m20 * rhs.m00) + (lhs.m21 * rhs.m10) + (lhs.m22 * rhs.m20) + (lhs.m23 * rhs.m30);
    m.m21 = (lhs.m20 * rhs.m01) + (lhs.m21 * rhs.m11) + (lhs.m22 * rhs.m21) + (lhs.m23 * rhs.m31);
    m.m22 = (lhs.m20 * rhs.m02) + (lhs.m21 * rhs.m12) + (lhs.m22 * rhs.m22) + (lhs.m23 * rhs.m32);
    m.m23 = (lhs.m20 * rhs.m03) + (lhs.m21 * rhs.m13) + (lhs.m22 * rhs.m23) + (lhs.m23 * rhs.m33);
    m.m30 = (lhs.m30 * rhs.m00) + (lhs.m31 * rhs.m10) + (lhs.m32 * rhs.m20) + (lhs.m33 * rhs.m30);
    m.m31 = (lhs.m30 * rhs.m01) + (lhs.m31 * rhs.m11) + (lhs.m32 * rhs.m21) + (lhs.m33 * rhs.m31);
    m.m32 = (lhs.m30 * rhs.m02) + (lhs.m31 * rhs.m12) + (lhs.m32 * rhs.m22) + (lhs.m33 * rhs.m32);
    m.m33 = (lhs.m30 * rhs.m03) + (lhs.m31 * rhs.m13) + (lhs.m32 * rhs.m23) + (lhs.m33 * rhs.m33);
    return m;
}

static ScMatrix4x4 sc_matrix4x4_perspective(float fov, float aspect, float znear, float zfar) {
    const float num = 1.0f / ((float)SDL_tanf(fov * 0.5f));
    // clang-format off
    return (ScMatrix4x4) {
        num / aspect, 0, 0, 0,
        0, num, 0, 0,
        0, 0, zfar / (znear - zfar), -1,
        0, 0, (znear * zfar) / (znear - zfar), 0,
    };
    // clang-format on
}

static ScMatrix4x4 sc_matrix4x4_look_at(ScVector3 origin, ScVector3 target, ScVector3 up) {
    const ScVector3 target_to_origin = {
        origin.x - target.x,
        origin.y - target.y,
        origin.z - target.z,
    };
    const ScVector3 vec_a = sc_vector3_normalized(target_to_origin);
    const ScVector3 vec_b = sc_vector3_normalized(sc_vector3_cross(up, vec_a));
    const ScVector3 vec_c = sc_vector3_cross(vec_a, vec_b);
    // clang-format off
    return (ScMatrix4x4) {
        vec_b.x, vec_c.x, vec_a.x, 0,
        vec_b.y, vec_c.y, vec_a.y, 0,
        vec_b.z, vec_c.z, vec_a.z, 0,
        -sc_vector3_dot(vec_b, origin), -sc_vector3_dot(vec_c, origin), -sc_vector3_dot(vec_a, origin), 1,
    };
    // clang-format on
}

static ScVector3 sc_bounds3_extents(ScBounds3 bounds) {
    return (ScVector3) {
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z,
    };
}

static ScVector3 sc_bounds3_center(ScBounds3 bounds) {
    return (ScVector3) {
        (bounds.min.x + bounds.max.x) * 0.5f,
        (bounds.min.y + bounds.max.y) * 0.5f,
        (bounds.min.z + bounds.max.z) * 0.5f,
    };
}
