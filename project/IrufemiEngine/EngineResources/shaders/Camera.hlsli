#pragma once

struct Camera {
    float32_t4x4 view;
    float32_t4x4 projection;
    float32_t3 worldPosition;
};
