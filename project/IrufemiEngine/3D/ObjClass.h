#pragma once

#include <d3d12.h>
#include <string>
#include "../camera/Camera.h"
#include "../source/D3D12ResourceUtil.h"
#include <wrl.h>
#include <cstdint>
#include <memory>

// 前方宣言

class TextureManager;
class DrawManager;
class DebugUI;

//==========================
// objが配布されているサイト
// https://quaternius.com/
// 使用する場合はライセンスがCCOのものを利用する
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

class ObjClass {
protected: //メンバ変数

    ObjModel objModel_;

    std::vector<std::unique_ptr<Texture>> textures_;

    std::vector<std::unique_ptr<D3D12ResourceUtil>> resources_;

#pragma region 外部参照

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

#pragma endregion


public: //メンバ関数

    //デストラクタ
    ~ObjClass() = default;

    //初期化
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");

    void Update(const char* objName = " ");

    void Draw();
    // 位置
    const Vector3& GetPosition(uint32_t index = 0)const { return resources_[index]->transform_.translate; }
    void SetPosition(const Vector3& position, uint32_t index = 0) { resources_[index]->transform_.translate = position; }

    // 回転
    const Vector3& GetRotate(uint32_t index = 0)const { return resources_[index]->transform_.rotate; }
    void SetRotate(const Vector3& rotate, uint32_t index = 0) { for (auto& res : resources_) { res->transform_.rotate = rotate; } }
    void SetRotateX(const float& rotate, uint32_t index = 0) { for (auto& res : resources_) { res->transform_.rotate.x = rotate; } }
    void SetRotateY(const float& rotate, uint32_t index = 0) { for (auto& res : resources_) { res->transform_.rotate.y = rotate; } }
    void SetRotateZ(const float& rotate, uint32_t index = 0) { for (auto& res : resources_) { res->transform_.rotate.z = rotate; } }

    // 拡縮
    const Vector3& GetScale(uint32_t index = 0)const { return resources_[index]->transform_.scale; }
    void SetScale(const Vector3& scale) { for (auto& res : resources_) { res->transform_.scale = scale; } }

    // Transform
    const Transform& GetTransform(uint32_t index = 0)const { return resources_[index]->transform_; }
    void SetTransform(Transform transform) { for (auto& res : resources_) { res->transform_ = transform; } }

    // Transform
    const TransformationMatrix& GetTransformationMatrix(uint32_t index = 0)const { return resources_[index]->transformationMatrix_; }
    void SetTransformationMatrix(TransformationMatrix transformationMatrix, uint32_t index = 0) { resources_[index]->transformationMatrix_ = transformationMatrix; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }

};

