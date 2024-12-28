cbuffer uniform_buffer: register(b0, space1) {
    float4x4 transform: packoffset(c0);
}

struct vs_input {
    uint position: TEXCOORD0;
    float4 color: TEXCOORD1;

    int3 instance_min: TEXCOORD2;
    int3 instance_max: TEXCOORD3;
};

struct vs_output {
    float4 position: SV_Position;
    float4 color: TEXCOORD0;
};

vs_output vs_main(vs_input input) {
    vs_output output;

    float unit = 1.0 / 64.0;
    float units_per_block = 1024.0;
    float block = unit * units_per_block;

    float min_x = block * (float)input.instance_min.x;
    float min_y = block * (float)input.instance_min.y;
    float min_z = block * (float)input.instance_min.z;
    float max_x = block * (float)input.instance_max.x;
    float max_y = block * (float)input.instance_max.y;
    float max_z = block * (float)input.instance_max.z;
    float extent_x = max_x - min_x;
    float extent_y = max_y - min_y;
    float extent_z = max_z - min_z;

    uint ix = input.position & 0x3ff;
    uint iy = (input.position >> 10) & 0x3ff;
    uint iz = (input.position >> 20) & 0x3ff;
    float x = min_x + ((float)ix / 1023.0f) * extent_x;
    float y = min_y + ((float)iy / 1023.0f) * extent_y;
    float z = min_z + ((float)iz / 1023.0f) * extent_z;
    float3 position = float3(x, y, z);

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
