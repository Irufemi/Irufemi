#pragma once

#include <vector>
#include <array>
#include <d3d12.h>

#include "Engine/Core/Shape/Triangle.h"
#include "Renderer/D3D12ResourceUtil.h"
#include "Application/camera/Camera.h"
#include <wrl.h>
#include <memory>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class TriangleClass {
protected: //メンバ変数

    Triangle info_{};
    
    std::unique_ptr<D3D12ResourceUtil> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

public: //メンバ関数
    //デストラクタ
    ~TriangleClass() = default;

    //初期化
    void Initialize(Camera* camera,const std::string& textureName = "resources/uvChecker.png");
    //更新
    void Update();

    // 描画
    void Draw();

    // デバッグ
    void Debug(const char* triangleName = "");

    //ゲッター
    D3D12ResourceUtil* GetD3D12Resource() { return this->resource_.get(); }

    // Triangleの情報を取得
    Triangle GetInfo() const { return info_; }

    void SetTransform(const Transform& transform) { if (resource_) resource_->transform_ = transform; isDirty_ = true; }
    Transform GetTransform()const { return resource_ ? resource_->transform_ : Transform{}; }

    void SetTranslate(const Vector3& translate) { if (resource_) resource_->transform_.translate = translate; isDirty_ = true; }
    Vector3 GetTranslate()const { return resource_ ? resource_->transform_.translate : Vector3{}; }

    void SetScale(const Vector3& scale) { if (resource_) resource_->transform_.scale = scale; isDirty_ = true; }
    Vector3 GetScale()const { return resource_ ? resource_->transform_.scale : Vector3{}; }

    void SetRotate(const Vector3& rotate) { if (resource_) resource_->transform_.rotate = rotate; isDirty_ = true; }
    Vector3 GetRotate()const { return resource_ ? resource_->transform_.rotate : Vector3{}; }

    void SetColor(const Vector4& color) { if (resource_ && resource_->materialData_) resource_->materialData_->color = color; }
    Vector4 GetColor()const { return resource_ && resource_->materialData_ ? resource_->materialData_->color : Vector4{}; }

private:
public:
    // 行列更新の最適化用
    bool isDirty_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    
};
