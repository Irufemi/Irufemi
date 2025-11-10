#pragma once
#include <d3d12.h>
#include <string>
#include "Application/camera/Camera.h"
#include "source/D3D12ResourceUtil.h"
#include "math/ObjModel.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;

//==========================
// objが配布されているサイト
// https://quaternius.com/
// 使用する場合はライセンスがCCOのものを利用する
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

class ObjClass {
protected: //メンバ変数
    // 共有モデル
    std::shared_ptr<ObjModel> objModel_;

    std::vector<std::unique_ptr<Texture>> textures_;
    std::vector<std::unique_ptr<D3D12ResourceUtil>> resources_;

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_; // 新規

public: //メンバ関数

    //デストラクタ
    ~ObjClass() = default;
    
    //初期化
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");

    void Update(const char* objName = " ");

    void Draw();

    // Transform 系は従来通り
    const Vector3& GetPosition(uint32_t index = 0)const { return resources_[index]->transform_.translate; }
    void SetPosition(const Vector3& position, uint32_t index = 0) { resources_[index]->transform_.translate = position; }

    const Vector3& GetRotate(uint32_t index = 0)const { return resources_[index]->transform_.rotate; }
    void SetRotate(const Vector3& rotate, uint32_t index = 0) { for (auto& r : resources_) r->transform_.rotate = rotate; }
    void SetRotateX(const float& rotate) { for (auto& r : resources_) r->transform_.rotate.x = rotate; }
    void SetRotateY(const float& rotate) { for (auto& r : resources_) r->transform_.rotate.y = rotate; }
    void SetRotateZ(const float& rotate) { for (auto& r : resources_) r->transform_.rotate.z = rotate; }
    
    // 拡縮
    const Vector3& GetScale(uint32_t index = 0) const { return resources_[index]->transform_.scale; }
    void SetScale(const Vector3& scale) { for (auto& r : resources_) r->transform_.scale = scale; }

    const Transform& GetTransform(uint32_t index = 0)const { return resources_[index]->transform_; }
    void SetTransform(Transform t) { for (auto& r : resources_) r->transform_ = t; }

    const TransformationMatrix& GetTransformationMatrix(uint32_t index = 0)const { return resources_[index]->transformationMatrix_; }
    void SetTransformationMatrix(TransformationMatrix m, uint32_t index = 0) { resources_[index]->transformationMatrix_ = m; }

    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; } // 追加
};

