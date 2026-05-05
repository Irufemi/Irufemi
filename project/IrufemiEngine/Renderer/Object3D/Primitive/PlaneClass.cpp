#include "Renderer/Object3D/Primitive/PlaneClass.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Texture/TextureManager.h"

#include "Engine/Graphics/Camera/Camera.h"
#include <wrl.h>
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"

TextureManager* PlaneClass::textureManager_ = nullptr;
DrawManager* PlaneClass::drawManager_ = nullptr;
DebugUI* PlaneClass::ui_ = nullptr;
IrufemiEngine* PlaneClass::engine_ = nullptr;

#include "Engine/IrufemiEngine.h"
void PlaneClass::Initialize(const std::string& textureName) {

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Plane);

    // Object3DResourceを生成
    resource_ = std::make_unique<Object3DResource>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // リソースのメモリを確保 (定数バッファのみ)
    resource_->CreateResource();
    resource_->Map();

    // マテリアル
    if (resource_->GetMaterialData()) {
        resource_->GetMaterialData()->color = { 1.0f,1.0f,1.0f,1.0f };
        resource_->GetMaterialData()->enableLighting = true;
        resource_->GetMaterialData()->hasTexture = true;
        resource_->GetMaterialData()->lightingMode = 3;
        resource_->GetMaterialData()->uvTransform = Math::MakeIdentity4x4();
        resource_->GetMaterialData()->metallic = 0.0f;
        resource_->GetMaterialData()->roughness = 0.5f;
        resource_->GetMaterialData()->environmentCoefficient = 0.0f;
        resource_->GetMaterialData()->alphaReference = 0.5f;
        resource_->GetMaterialData()->useClampSampler = 0;
    }

    Update();

    // テクスチャ設定
    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void PlaneClass::Update() {
    if (!resource_ || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // Transform更新
    resource_->UpdateTransform(*activeCam);

    // UV行列
    if (resource_->GetMaterialData()) {
        resource_->GetMaterialData()->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
    }

    // 平面情報をワールド空間で更新
    {
        // 平行移動を除いた行列
        Matrix4x4 m = resource_->transformationMatrix_.world;
        m.m[3][0] = m.m[3][1] = m.m[3][2] = 0.0f;
        m.m[3][3] = 1.0f;

        Vector3 nLocal{ 0.0f, 0.0f, -1.0f };
        Vector3 nWorld{
            nLocal.x * m.m[0][0] + nLocal.y * m.m[1][0] + nLocal.z * m.m[2][0],
            nLocal.x * m.m[0][1] + nLocal.y * m.m[1][1] + nLocal.z * m.m[2][1],
            nLocal.x * m.m[0][2] + nLocal.y * m.m[1][2] + nLocal.z * m.m[2][2]
        };
        nWorld = Math::Normalize(nWorld);

        Vector3 pWorld{
            resource_->transformationMatrix_.world.m[3][0],
            resource_->transformationMatrix_.world.m[3][1],
            resource_->transformationMatrix_.world.m[3][2]
        };

        info_.normal = nWorld;
        info_.distance = (nWorld.x * pWorld.x) + (nWorld.y * pWorld.y) + (nWorld.z * pWorld.z);
    }

    isDirty_ = false;
    lastViewMatrix_ = activeCam->GetViewMatrix();
    lastProjectionMatrix_ = activeCam->GetPerspectiveFovMatrix();
}

void PlaneClass::Draw() {
    Draw(false);
}

void PlaneClass::Draw(bool isUI) {
    if (!resource_ || !drawManager_ || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        // 単位正方形 (-0.5 to 0.5) を想定し、最大スケールから半径を算出
        float maxScale = (std::max)({ resource_->transform_.scale.x, resource_->transform_.scale.y, resource_->transform_.scale.z });
        float finalRadius = 0.7071f * maxScale; // 0.5 * sqrt(2)

        Sphere boundingSphere;
        boundingSphere.center = resource_->transform_.translate;
        boundingSphere.radius = finalRadius * 1.1f; // 10%のマージン

        if (!Collision::IsCollision(activeCam->GetFrustum(), boundingSphere)) {
            return; // 描画スキップ
        }
    }

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &activeCam->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &activeCam->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    // --- 【追加】描画直前のバッファ同期 ---
    resource_->SyncBeforeDraw();

    if (isUI) {
        drawManager_->SubmitUI3D(resource_.get());
    } else {
        drawManager_->SubmitStandard3D(resource_.get(), nullptr, castShadows_);
    }
}

void PlaneClass::Debug([[maybe_unused]] const char* planeName) {
#if defined USE_IMGUI
    std::string name = std::string("Plane: ") + planeName;

    ImGui::Begin(name.c_str());

    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->GetMaterialData());
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
    ui_->DebugUvTransform(resource_->uvTransform_);

    ImGui::End();
#endif
}

void PlaneClass::SyncBeforeDraw() {
    if (resource_) {
        resource_->SyncBeforeDraw();
    }
}

