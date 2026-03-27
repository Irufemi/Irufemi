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
#include "Engine/Core/Shape/Sphere.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class SphereClass {
protected: //メンバ変数

    Sphere info_{};

    const float pi_ = 3.141592654f;

    const uint32_t kSubdivision_ = 16;

    //経度分割1つ分の角度 Φd

    const float kLonEvery_ = pi_ * 2.0f / static_cast<float>(kSubdivision_);

    //緯度分割分の角度 θd
    const float kLatEvery_ = pi_ / static_cast<float>(kSubdivision_);

    bool isRotateY_ = true;

    // D3D12リソース
    std::unique_ptr<Object3DResource> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

    Camera* camera_ = nullptr;

public: //メンバ関数
    // コンストラクタ
    SphereClass() {};

    // デストラクタ
    ~SphereClass() = default;

    // 初期化
    void Initialize(Camera* camera,  const std::string& textureName = "resources/uvChecker.png");

    // 更新
    void Update();

    // 描画
    void Draw();

    // デバッグ
    void Debug(const char* sphereName = " ");

    Object3DResource* GetD3D12Resource() { return this->resource_.get(); }
    void AddRotateY(float value) { this->resource_->transform_.rotate.y += value; }

    // Sphereの情報を取得
    Sphere GetInfo() const { return info_; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }

    void SetInfo(const Sphere& info) { info_ = info; isDirty_ = true; }
    void SetCenter(const Vector3& center) { info_.center = center; isDirty_ = true; }
    void SetRadius(const float& radius) { info_.radius = radius; isDirty_ = true; }
    void SetRotate(const Vector3& rotate) { if (resource_) resource_->transform_.rotate = rotate; isDirty_ = true; }
    void SetColor(const Vector4&color){ if (resource_ && resource_->materialData_) resource_->materialData_->color = color; }

private:
    // 行列更新の最適化用
    bool isDirty_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
};

