#include "../../Core/IRenderable.h"
#pragma once

#include <vector>
#include <array>
#include <d3d12.h>
#include <string>
#include <memory>

#include "Engine/Core/shape/Plane.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Application/camera/Camera.h"
#include <wrl.h>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class PlaneClass : public IRenderable {
private: //メンバ変数

    Plane info_{};

    std::unique_ptr<Object3DResource> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

public: //メンバ関数
    //デストラクタ
    ~PlaneClass() = default;

    //初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");
    //更新
    void Update();

    // 描画
    void SyncBeforeDraw() override;
    void Draw() override;
    void Draw(bool isUI);

    // デバッグ
    void Debug(const char* planeName = "");

    //ゲッター
    Object3DResource* GetD3D12Resource() { return this->resource_.get(); }

    // Planeの情報を取得
    Plane GetInfo() const { return info_; }

    void SetTransform(const Transform& transform) { if (resource_) resource_->transform_ = transform; isDirty_ = true; }
    Transform GetTransform()const { return resource_ ? resource_->transform_ : Transform{}; }

    void SetTranslate(const Vector3& translate) { if (resource_) resource_->transform_.translate = translate; isDirty_ = true; }
    Vector3 GetTranslate()const { return resource_ ? resource_->transform_.translate : Vector3{}; }

    void SetScale(const Vector3& scale) { if (resource_) resource_->transform_.scale = scale; isDirty_ = true; }
    Vector3 GetScale()const { return resource_ ? resource_->transform_.scale : Vector3{}; }

    void SetRotate(const Vector3& rotate) { if (resource_) resource_->transform_.rotate = rotate; isDirty_ = true; }
    Vector3 GetRotate()const { return resource_ ? resource_->transform_.rotate : Vector3{}; }

    void SetColor(const Vector4& color) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->color = color; }
    Vector4 GetColor()const { return resource_ && resource_->GetMaterialData() ? resource_->GetMaterialData()->color : Vector4{}; }

    void SetAlphaReference(float alphaRef) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->alphaReference = alphaRef; }
    void SetUseClampSampler(int32_t useClamp) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->useClampSampler = useClamp; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }
    void SetCastShadows(bool cast) { castShadows_ = cast; }
    bool GetCastShadows() const { return castShadows_; }

private:
    // 行列更新の最適化用
    bool isDirty_ = true;
    bool isCullingEnabled_ = true;
    bool castShadows_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
};



