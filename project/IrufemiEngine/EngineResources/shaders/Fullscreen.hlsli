struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};


struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};
