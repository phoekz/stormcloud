cbuffer uniform_buffer: register(b0, space1) {
    float2 scale;
    float2 offset;
}

Texture2D<float4> font_texture: register(t0, space2);
SamplerState font_sampler: register(s0, space2);

struct vs_input {
    float2 position: TEXCOORD0;
    float2 texcoord: TEXCOORD1;
    float4 color: TEXCOORD2;
};

struct vs_output {
    float4 position: SV_Position;
    float2 texcoord: TEXCOORD0;
    float4 color: TEXCOORD1;
};

float3 linear_from_srgb(float3 color) {
    return pow(color, 2.2);
}

vs_output vs_main(vs_input input) {
    vs_output output;
    output.position = float4(input.position * scale + offset, 0.0f, 1.0f);
    output.texcoord = input.texcoord;
    output.color.rgb = linear_from_srgb(input.color.rgb);
    output.color.a = input.color.a;
    return output;
}

struct fs_output {
    float4 color: SV_Target0;
};

fs_output fs_main(vs_output input) {
    fs_output output;
    output.color = input.color * font_texture.Sample(font_sampler, input.texcoord);
    return output;
}
