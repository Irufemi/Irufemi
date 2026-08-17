#include "Renderer/Pipeline/RenderGraph/ComputePass.h"
#include "Renderer/DrawManager.h"

void ComputePass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    // 登録されているコンピュートタスクを一斉にディスパッチし、必要に応じてUAVバリアを発行する
    drawManager->ExecuteComputePasses();
}
