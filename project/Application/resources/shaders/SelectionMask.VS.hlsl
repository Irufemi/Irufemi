struct VertexInput {
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PixelInput {
    float4 pos : SV_POSITION;
};

// 変換行列 (b1)
cbuffer TransformMatrix : register(b1) {
    matrix WVP;
    matrix World;
    matrix WorldInverseTranspose;
};

PixelInput VSMain(VertexInput input) {
    PixelInput output;
    output.pos = mul(input.pos, WVP);
    return output;
}
