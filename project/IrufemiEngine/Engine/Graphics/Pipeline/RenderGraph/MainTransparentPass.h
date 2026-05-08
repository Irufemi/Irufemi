#pragma once
#include "IRenderPass.h"

class MainTransparentPass : public IRenderPass {
public:
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
