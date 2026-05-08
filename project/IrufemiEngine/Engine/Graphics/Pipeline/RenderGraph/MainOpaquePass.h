#pragma once
#include "IRenderPass.h"

class MainOpaquePass : public IRenderPass {
public:
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
