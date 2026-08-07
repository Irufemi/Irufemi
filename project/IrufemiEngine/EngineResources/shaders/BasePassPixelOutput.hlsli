#pragma once

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
    float32_t4 mask : SV_TARGET1;
    float32_t4 normal : SV_TARGET2;
    float32_t4 material : SV_TARGET3;
    float32_t2 velocity : SV_TARGET4;
};
