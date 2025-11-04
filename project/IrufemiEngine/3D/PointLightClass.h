#pragma once

#include "math/PointLight.h"
#include "engine/directX/DirectXCommon.h"
#include "wrl.h"
#include <d3d12.h>

class PointLightClass
{
    
private: // メンバ変数

    Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;
    PointLight* data_ = nullptr;

    static DirectXCommon* dxCommon_;

public: // メンバ関数

    // 初期化
    void Initialize();

    // 更新
    void Debug();

    // PointLightの情報を渡す
    PointLight* GetData() { return data_; }
    // PointLightの情報をセットする
    void SetData(PointLight* info) { data_ = info; }

    ID3D12Resource* GetResource() { return resource_.Get(); }

    static void SetDxCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }

    void SetIntensity(const float& intensity) { data_->intensity = intensity; }
    void SetPos(const Vector3& pos) { data_->position = pos; }
    void SetColor(const Vector4& color) { data_->color = color; }

};

