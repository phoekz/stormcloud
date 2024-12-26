struct vs_input {
    float3 position: TEXCOORD0;
};

struct vs_output {
    float4 position: SV_Position;
};

vs_output vs_main(vs_input input) {
    vs_output output;
    output.position = float4(input.position, 1.0f);
    return output;
}

struct fs_output {
    float4 color: SV_Target0;
};

fs_output fs_main() {
    fs_output output;
    output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    return output;
}
