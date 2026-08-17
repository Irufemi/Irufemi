#include "Renderer/System/Data/RenderData.h"
#include "Renderer/Object/PrimitiveManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Renderer/Camera/Camera.h"

// --- TransformComponent ---

void PrimitiveTransform::UpdateTransform(Object3DResource* resource, const Camera& camera) {
    if (!resource) return;

    // 行列の更新
    // 既存の Object3DResource::UpdateTransform は内部で world 行列を再計算するため、
    // ここでは Irufemi::Transform の値を resource に同期させるだけで済む
    resource->transform_ = transform;
    resource->UpdateTransform(camera);

    isDirty = false;
}

// --- MeshComponent ---

void MeshDesc::ChangeMesh(Irufemi::PrimitiveType newType) {
    type = newType;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = primitiveManager_->GetStandardResource(type);

    bool isNewResource = false;
    if (!resource) {
        resource = std::make_unique<Object3DResource>();
        isNewResource = true;
    }

    // リソースの共有設定
    resource->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource->indexBufferView_ = primitiveResource.indexBufferView;
    resource->indexCount_ = primitiveResource.indexCount;

    // 定数バッファ等の生成は最初だけ行う
    if (isNewResource) {
        resource->CreateResource();
        resource->Map();
    }
}

void MeshDesc::ChangeMesh(const PrimitiveData& data) {
    if (!resource) {
        resource = std::make_unique<Object3DResource>();
        resource->CreateResource();
        resource->Map();
    }

    // 動的にリソースを生成し、自身で所有権を持つ
    PrimitiveResource customResource;
    primitiveManager_->CreateGPUResource(data, customResource);

    resource->vertexResource_ = customResource.vertexResource;
    resource->indexResource_ = customResource.indexResource;
    resource->vertexBufferView_ = customResource.vertexBufferView;
    resource->indexBufferView_ = customResource.indexBufferView;
    resource->indexCount_ = customResource.indexCount;
}

// --- MaterialComponent ---

void MaterialDesc::UpdateMaterial(Object3DResource* resource, TextureManager* textureManager) {
    if (!resource || !resource->GetMaterialData()) return;

    // マテリアルパラメータの反映
    resource->GetMaterialData()->color = color;
    resource->GetMaterialData()->enableLighting = enableLighting;
    resource->GetMaterialData()->lightingMode = lightingMode;
    resource->GetMaterialData()->metallic = metallic;
    resource->GetMaterialData()->roughness = roughness;
    resource->GetMaterialData()->hasTexture = !texturePath.empty();
    resource->GetMaterialData()->uvTransform = uvTransform;
    resource->GetMaterialData()->alphaReference = alphaReference;
    resource->GetMaterialData()->useClampSampler = useClampSampler;

    // テクスチャハンドルの更新（変更検知）
    if (textureManager && texturePath != loadedTexturePath) {
        if (textureHandle.IsValid()) {
            textureManager->ReleaseTexture(textureHandle);
        }
        if (!texturePath.empty()) {
            textureHandle = textureManager->LoadTexture(texturePath);
        } else {
            textureHandle = ResourceHandle(); // 無効ハンドル
        }
        loadedTexturePath = texturePath;
    }

    if (textureManager) {
        resource->textureHandle_ = textureHandle;
        resource->GetMaterialData()->hasTexture = (textureManager->Resolve(textureHandle).ptr != textureManager->GetWhiteTextureHandle().ptr);
    } else {
        resource->GetMaterialData()->hasTexture = false;
    }

    resource->MarkAsDirty();
}

void MaterialDesc::Release(TextureManager* textureManager) {
    if (textureManager && textureHandle.IsValid()) {
        textureManager->ReleaseTexture(textureHandle);
        textureHandle = ResourceHandle();
        loadedTexturePath.clear();
    }
}
