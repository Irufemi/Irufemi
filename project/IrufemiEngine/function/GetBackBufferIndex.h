#pragma once

#include <dxgi1_6.h>
#include <wrl.h>

UINT GetBackBufferIndex(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swapChain);
