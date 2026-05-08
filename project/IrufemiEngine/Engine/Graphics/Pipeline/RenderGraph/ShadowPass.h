#pragma once
#include "IRenderPass.h"

class ShadowPass : public IRenderPass {
public:
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
