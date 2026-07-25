#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    float32_t4 maskColor = gMaskTexture.Sample(gSampler, input.texcoord);
    
    if (maskColor.r > 0.5f) {
        return output;
    }
    
    // Grayscale conversion based on BT.709
    float32_t value = dot(output.color.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
    output.color.rgb = float32_t3(value, value, value);
    
    return output;
}
