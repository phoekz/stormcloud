cbuffer uniform_buffer: register(b0, space1) {
    float4x4 clip_from_world: packoffset(c0);
}

struct vs_input {
    float3 position: TEXCOORD0;
    float4 color: TEXCOORD1;
};

struct vs_output {
    float4 position: SV_Position;
    float4 color: TEXCOORD0;
};

vs_output vs_main(vs_input input) {
    vs_output output;
    output.position = mul(clip_from_world, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}

struct fs_output {
    float4 color: SV_Target0;
};

fs_output fs_main(vs_output input) {
    fs_output output;
    output.color = input.color;
    return output;
}
