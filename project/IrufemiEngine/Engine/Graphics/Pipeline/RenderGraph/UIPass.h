#pragma once
#include "IRenderPass.h"

class UIPass : public IRenderPass {
public:
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
