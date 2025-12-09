#pragma once
#include <d3d12.h>
#include <string>
#include "Application/camera/Camera.h"
#include "source/D3D12ResourceUtil.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "math/Vector4.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
struct ManagedModel;

//==========================
// objが配布されているサイト
// https://quaternius.com/
// 使用する場合はライセンスがCCOのものを利用する
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

class ObjClass {
private:
    // 共有モデルデータ（CPU/GPU）
    std::shared_ptr<ManagedModel> managedModel_;

    std::vector<std::unique_ptr<Texture>> textures_;
    // インスタンス固有リソース（Transform, Material等）
    std::vector<std::unique_ptr<D3D12ResourceUtil>> instanceResources_;

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_;

public: //メンバ関数

    //デストラクタ
~ObjClass() = default;
    //初期化
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");
    void Update();
    void Draw();
    void Debug(const char* objName = " ");

    // Transform 系ゲッター/セッター（instanceResources_ を参照するように変更）
    const Vector3& GetPosition(uint32_t index = 0)const { return instanceResources_[index]->transform_.translate; }
    void SetPosition(const Vector3& position, uint32_t index = 0) { instanceResources_[index]->transform_.translate = position; }

    const Vector3& GetRotate(uint32_t index = 0)const { return instanceResources_[index]->transform_.rotate; }
    void SetRotate(const Vector3& rotate, uint32_t index = 0) { for (auto& res : instanceResources_) { res->transform_.rotate = rotate; } }
    void SetRotateX(const float& rotate) { for (auto& res : instanceResources_) { res->transform_.rotate.x = rotate; } }
    void SetRotateY(const float& rotate) { for (auto& res : instanceResources_) { res->transform_.rotate.y = rotate; } }
    void SetRotateZ(const float& rotate) { for (auto& res : instanceResources_) { res->transform_.rotate.z = rotate; } }
    
    // 拡縮
    const Vector3& GetScale(uint32_t index = 0)const { return instanceResources_[index]->transform_.scale; }
    void SetScale(const Vector3& scale) { for (auto& res : instanceResources_) { res->transform_.scale = scale; } }
    const Transform& GetTransform(uint32_t index = 0)const { return instanceResources_[index]->transform_; }
    void SetTransform(Transform transform) { for (auto& res : instanceResources_) { res->transform_ = transform; } }
    const TransformationMatrix& GetTransformationMatrix(uint32_t index = 0)const { return instanceResources_[index]->transformationMatrix_; }
    void SetTransformationMatrix(TransformationMatrix transformationMatrix, uint32_t index = 0) { instanceResources_[index]->transformationMatrix_ = transformationMatrix; }

    // lighitingModeの切り替え
    void SetLightingMode(int32_t index) { for (auto& res : instanceResources_) { res->materialData_->lightingMode = index; } }

    // 色の一括設定
    void SetColor(const Vector4 &color) {
      for (auto &res : instanceResources_) {
        if (res->materialData_) {
          res->materialData_->color = color;
        }
      }
    }

    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
};

