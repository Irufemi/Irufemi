#include "WeaponTrail.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Engine/Manager/DrawManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Core/Math/Math.h"

WeaponTrail::WeaponTrail() = default;
WeaponTrail::~WeaponTrail() = default;

void WeaponTrail::Initialize(IrufemiEngine* engine, const std::string& texturePath, const Vector4& color) {
    engine_ = engine;
    texturePath_ = texturePath;
    baseColor_ = color;
    isStopped_ = true;
    points_.clear();

    resource_ = std::make_unique<Object3DResource>();
    
    int numLayers = 3; // 中央、上、下の3層構造
    int maxVertices = kMaxPoints * 2 * numLayers;
    int maxIndices = (kMaxPoints - 1) * 6 * numLayers;

    resource_->vertexDataList_.resize(maxVertices);
    resource_->indexDataList_.resize(maxIndices);
    resource_->CreateResource();

    if (engine_) {
        if (auto tm = engine_->GetTextureManager()) {
            resource_->textureHandle_ = tm->GetTextureHandle(texturePath_);
        }
    }
    
    resource_->GetMaterialData()->color = baseColor_;
    resource_->GetMaterialData()->enableLighting = false; // エフェクトのためライティング無効
    resource_->GetMaterialData()->useClampSampler = 0; // Wrap
    resource_->GetMaterialData()->hasTexture = true;
    
    resource_->transform_.scale = {1,1,1};
    resource_->transform_.rotate = {0,0,0};
    resource_->transform_.translate = {0,0,0};
    resource_->isDirtyBuffer_[0] = true;
    resource_->isDirtyBuffer_[1] = true;
    resource_->isDirtyBuffer_[2] = true;

    resource_->Map();
}

void WeaponTrail::AddPoint(const Vector3& basePos, const Vector3& tipPos) {
    if (isStopped_) {
        points_.clear();
        isStopped_ = false;
    }

    TrailPoint p;
    p.basePos = basePos;
    p.tipPos = tipPos;
    p.age = kMaxLifeTime;

    points_.push_back(p);

    if (points_.size() > kMaxPoints) {
        points_.erase(points_.begin());
    }
}

void WeaponTrail::StopTrail() {
    isStopped_ = true;
}

void WeaponTrail::Update() {
    if (points_.empty()) return;

    // 寿命を減らす
    for (auto it = points_.begin(); it != points_.end(); ) {
        it->age--;
        if (it->age <= 0) {
            it = points_.erase(it);
        } else {
            ++it;
        }
    }

    if (points_.size() < 2) {
        resource_->indexCount_ = 0;
        return;
    }

    // 頂点とインデックスを更新
    if (!resource_->vertexData_) {
        resource_->Map();
    }

    if (resource_->vertexData_ && resource_->indexData_) {
        int vIndex = 0;
        int iIndex = 0;
        
        // ミルフィーユ状に3枚の平面（リボン）を生成して厚みを出す
        float layerOffsets[] = { 0.0f, 0.6f, -0.6f };

        for (int layer = 0; layer < 3; ++layer) {
            float yOffset = layerOffsets[layer];
            int pIndex = 0;
            int layerStartIndex = vIndex;
            float numSegments = static_cast<float>(points_.size() - 1);

            for (const auto& pt : points_) {
                float alpha = static_cast<float>(pt.age) / static_cast<float>(kMaxLifeTime);
                Vector4 color = baseColor_;
                color.w *= alpha;
                
                // 上下の層は少し薄くすることで、中心が一番濃い立体感を出す
                if (layer > 0) {
                    color.w *= 0.5f; 
                }

                // 頂点の「年齢(age)」ベースでV座標を計算することで、
                // トレイルが短い（振り始めの）状態でもテクスチャが潰れず、常に先端が濃く描画されるようにする
                float v = 1.0f - alpha;

                // 根本 (Base)
                resource_->vertexData_[vIndex].position = { pt.basePos.x, pt.basePos.y + yOffset, pt.basePos.z, 1.0f };
                resource_->vertexData_[vIndex].texcoord = { 0.0f, v };
                resource_->vertexData_[vIndex].normal = { 0.0f, 1.0f, 0.0f };
                resource_->vertexData_[vIndex].color = color;
                vIndex++;

                // 先端 (Tip)
                resource_->vertexData_[vIndex].position = { pt.tipPos.x, pt.tipPos.y + yOffset, pt.tipPos.z, 1.0f };
                resource_->vertexData_[vIndex].texcoord = { 1.0f, v };
                resource_->vertexData_[vIndex].normal = { 0.0f, 1.0f, 0.0f };
                resource_->vertexData_[vIndex].color = color;
                vIndex++;

                pIndex++;
            }

            for (size_t i = 0; i < points_.size() - 1; ++i) {
                uint32_t bottomLeft = layerStartIndex + static_cast<uint32_t>(i * 2);
                uint32_t bottomRight = layerStartIndex + static_cast<uint32_t>(i * 2 + 1);
                uint32_t topLeft = layerStartIndex + static_cast<uint32_t>((i + 1) * 2);
                uint32_t topRight = layerStartIndex + static_cast<uint32_t>((i + 1) * 2 + 1);

                // Triangle 1
                resource_->indexData_[iIndex++] = bottomLeft;
                resource_->indexData_[iIndex++] = topLeft;
                resource_->indexData_[iIndex++] = bottomRight;

                // Triangle 2
                resource_->indexData_[iIndex++] = bottomRight;
                resource_->indexData_[iIndex++] = topLeft;
                resource_->indexData_[iIndex++] = topRight;
            }
        }

        resource_->indexCount_ = static_cast<uint32_t>(iIndex);
    }
}

void WeaponTrail::SyncBeforeDraw() {
    if (resource_ && points_.size() >= 2) {
        // Transformがダミーでも、World行列計算のため必要
        resource_->UpdateTransform(*engine_->GetCameraManager()->GetActiveCamera());
        resource_->SyncBeforeDraw();
    }
}

void WeaponTrail::Draw() {
    if (resource_ && points_.size() >= 2 && engine_) {
        // ステートを保存
        BlendMode prevBlend = engine_->currentBlend_;
        PSOManager::DepthWrite prevDepth = engine_->currentDepth_;
        PSOManager::CullMode prevCull = engine_->currentCull_;

        // エフェクト用のステート設定
        engine_->SetBlend(BlendMode::kBlendModeAdd);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine_->SetCull(PSOManager::CullMode::None); // 両面描画

        if (auto dm = engine_->GetDrawManager()) {
            dm->SubmitStandard3D(resource_.get(), nullptr, false);
        }

        // ステート復元
        engine_->SetBlend(prevBlend);
        engine_->SetDepthWrite(prevDepth);
        engine_->SetCull(prevCull);
    }
}
