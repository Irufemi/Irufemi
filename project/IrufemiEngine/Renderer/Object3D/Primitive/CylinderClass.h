#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Application/camera/Camera.h"
#include "Renderer/Object3D/Object3DResource.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

struct Cylinder {
    Vector3 center{ 0.0f, 0.0f, 0.0f };
    float   radius = 1.0f; // XZ 方向の半径
    float   height = 1.0f; // Y 方向の高さ
};

class CylinderClass {
protected: // メンバ変数

    Cylinder info_{};

    const float pi_ = 3.141592654f;

    // 周方向分割数
    const uint32_t kSubdivision_ = 16;
    // 高さ方向分割数(側面)
    const uint32_t kHeightSubdivision_ = 1;

    // 周方向 1 セグメントあたりの角度
    const float kThetaEvery_ = pi_ * 2.0f / static_cast<float>(kSubdivision_);

    // D3D12 リソース
    std::unique_ptr<Object3DResource> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照(非所有)
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    Camera* camera_ = nullptr;

public: // メンバ関数
    // コンストラクタ
    CylinderClass() {}
    // デストラクタ
    ~CylinderClass() = default;

    // 初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");

    // 更新
    void Update();

    // 描画
    void Draw();

    // デバッグ
    void Debug(const char* cylinderName = " ");

    // 補助
    Object3DResource* GetD3D12Resource() { return this->resource_.get(); }
    void AddRotateY(float value) { this->resource_->transform_.rotate.y += value; }

    // 情報アクセス
    Cylinder GetInfo() const { return info_; }
    Vector3 GetCenter() const { return info_.center; }
    Vector3 GetRight() const;
    Vector3 GetUp() const;
    Vector3 GetDirection() const;

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

    void SetInfo(const Cylinder& info) { info_ = info; isDirty_ = true; }
    void SetCenter(const Vector3& center) { info_.center = center; isDirty_ = true; }
    void SetRadius(float radius) { info_.radius = radius; isDirty_ = true; }
    void SetHeight(float height) { info_.height = height; isDirty_ = true; }
    void SetRotate(const Vector3& rotate) { if (resource_) resource_->transform_.rotate = rotate; isDirty_ = true; }
    void SetColor(const Vector4& color) { if (resource_ && resource_->materialData_) resource_->materialData_->color = color; }

private:
    // 行列更新の最適化用
    bool isDirty_ = true;
    bool isCullingEnabled_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
};

