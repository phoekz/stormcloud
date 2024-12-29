cbuffer uniform_buffer: register(b0, space1) {
    float4x4 transform;
    float node_world_scale;
    uint32_t pad_0;
    uint32_t pad_1;
    uint32_t pad_2;
}

struct vs_input {
    float3 position: TEXCOORD0;
    float4 color: TEXCOORD1;
    float3 instance_min: TEXCOORD2;
    float3 instance_max: TEXCOORD3;
};

struct vs_output {
    float4 position: SV_Position;
    float4 color: TEXCOORD0;
};

vs_output vs_main(vs_input input) {
    const float min_x = node_world_scale * input.instance_min.x;
    const float min_y = node_world_scale * input.instance_min.y;
    const float min_z = node_world_scale * input.instance_min.z;
    const float max_x = node_world_scale * input.instance_max.x;
    const float max_y = node_world_scale * input.instance_max.y;
    const float max_z = node_world_scale * input.instance_max.z;
    const float extent_x = max_x - min_x;
    const float extent_y = max_y - min_y;
    const float extent_z = max_z - min_z;

    const float x = min_x + input.position.x * extent_x;
    const float y = min_y + input.position.y * extent_y;
    const float z = min_z + input.position.z * extent_z;
    const float3 position = float3(x, y, z);

    vs_output output;
    output.position = mul(transform, float4(position, 1.0f));
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
