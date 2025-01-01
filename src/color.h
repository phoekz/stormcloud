static SC_INLINE uint32_t sc_color_from_hsv(float hue, float saturation, float value) {
    const float h = hue;
    const float s = saturation;
    const float v = value;
    const uint32_t i = (uint32_t)floorf(h * 6);
    const float f = h * 6 - i;
    const float p = v * (1 - s);
    const float q = v * (1 - f * s);
    const float t = v * (1 - (1 - f) * s);

    float r, g, b;
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
        default: r = 0, g = 0, b = 0; break;
    }

    uint32_t color = 0;
    color |= (uint32_t)(r * 255.0f);
    color |= (uint32_t)(g * 255.0f) << 8;
    color |= (uint32_t)(b * 255.0f) << 16;
    color |= 0xff000000;
    return color;
}
