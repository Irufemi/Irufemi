#include "GetBackBufferIndex.h"

#include <cassert>

UINT GetBackBufferIndex(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swapChain) {
    assert(swapChain != nullptr);
    return swapChain->GetCurrentBackBufferIndex();
}