#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>

#include "Renderer/System/Core/MultiBufferSyncState.h"

class DirectXCommon;

class BaseResource : public MultiBufferSyncState {
public:
    virtual ~BaseResource() = default;

    /**
     * @brief DirectXCommon を設定する。
     * @param[in] dxCommon 設定する DirectXCommon の値
     */
    static void SetDirectXCommon(DirectXCommon* dxCommon) {
        s_dxCommon_ = dxCommon;
    }
    /**
     * @brief DirectXCommon を取得する。
     * @return 取得された DirectXCommon
     */
    static DirectXCommon* GetDirectXCommon() {
        return s_dxCommon_;
    }

    /**
     * @brief CreateResource を実行する。
     */
    virtual void CreateResource() = 0;
    /**
     * @brief Map を実行する。
     */
    virtual void Map() = 0;
    /**
     * @brief Unmap を実行する。
     */
    virtual void Unmap() = 0;

protected:
    static DirectXCommon* s_dxCommon_;
};
