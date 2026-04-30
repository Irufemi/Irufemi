#define NOMINMAX
#include "ParticleSystem.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Manager/DrawManager.h"
#include <algorithm>
#include <numbers>
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"

DescriptorPool* ParticleSystem::s_srvPool_ = nullptr;
TextureManager* ParticleSystem::textureManager_ = nullptr;
DrawManager* ParticleSystem::s_drawManager_ = nullptr;
IrufemiEngine* ParticleSystem::s_engine_ = nullptr;
DebugUI* ParticleSystem::s_ui_ = nullptr;

ParticleSystem::~ParticleSystem() {
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (instancingSrvIndex_[i] != UINT32_MAX && s_srvPool_ && resource_) {
            if (auto* dx = BaseResource::GetDirectXCommon()) {
                uint64_t fv = dx->GetFenceValue();
                s_srvPool_->FreeAfterFence(instancingSrvIndex_[i], fv);
            }
            instancingSrvIndex_[i] = UINT32_MAX;
            instancingSrvHandleCPU_[i] = {};
            instancingSrvHandleGPU_[i] = {};
        }
    }
}

void ParticleSystem::Initialize(Camera* camera, const std::string& textureName, ParticleType type, PrimitiveType shape) {
    this->camera_ = camera;
    this->primitiveShape_ = shape;

    isUpdate_ = true;
    randomEngine_.seed(seedGenerator_());

    // 初回呼び出し時のみリソースを生成
    if (!resource_) {
        resource_ = std::make_unique<ParticleResource>();
    }
    // リソース物理生成
    resource_->CreateResource();

    // デバッグ用の Line3DRegion を初期化
    if (!debugLineRegion_) {
        debugLineRegion_ = std::make_unique<Line3DRegion>();
        debugLineRegion_->Initialize(camera_);
    }

    // 振る舞いを設定
    ChangeBehavior(type, true); // 強制的に更新

    // 単位行列を書きこんでおく
    particles_.clear();
    numInstance_ = 0;

    // backToFrontMatrix_の設定(面の向きをカメラの方向にしてあるのでここは調整なし。0でOK)
    backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);

    /// カメラの回転を適用する
    billboardMatrix_ = Math::MakeIdentity4x4();
    billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billboardMatrix_.m[3][0] = 0.0f;
    billboardMatrix_.m[3][1] = 0.0f;
    billboardMatrix_.m[3][2] = 0.0f;

    D3D12_SHADER_RESOURCE_VIEW_DESC instancingDesc{};
    instancingDesc.Format = DXGI_FORMAT_UNKNOWN;
    instancingDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingDesc.Buffer.FirstElement = 0;
    instancingDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    instancingDesc.Buffer.NumElements = kNumMaxInstance_;
    instancingDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

    // SRV スロット確保(初回のみ)
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (instancingSrvIndex_[i] == UINT32_MAX) {
            auto* alloc = s_srvPool_;
            if (!alloc) {
                OutputDebugStringA("ParticleSystem::Initialize: SRV allocator is null\n");
            } else {
                uint32_t idx = alloc->Allocate();
                if (idx == DescriptorPool::kInvalid) {
                    OutputDebugStringA("ParticleSystem::Initialize: SRV Allocate failed\n");
                } else {
                    instancingSrvIndex_[i] = idx;
                    instancingSrvHandleCPU_[i] = alloc->GetCPUHandle(idx);
                    instancingSrvHandleGPU_[i] = alloc->GetGPUHandle(idx);
                }
            }
        }
    }

    // 既存の静的インデックス運用は廃止。確保できている場合のみ SRV を作成
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (instancingSrvHandleCPU_[i].ptr != 0 && resource_->instancingResource_[i]) {
            BaseResource::GetDirectXCommon()->GetDevice()->CreateShaderResourceView(resource_->instancingResource_[i].Get(), &instancingDesc, instancingSrvHandleCPU_[i]);
            resource_->instancingSrvHandleGPU_[i] = instancingSrvHandleGPU_[i];
        }
    }

    // 頂点/インデックスデータをクリア (共有リソース利用のため個別のリストを初期化)
    resource_->vertexDataList_.clear();
    resource_->indexDataList_.clear();

    // PrimitiveManager から共有リソースを取得
    const auto& primitiveRes = PrimitiveManager::GetInstance()->GetStandardResource(shape);
    resource_->vertexBufferView_ = primitiveRes.vertexBufferView;
    resource_->indexBufferView_ = primitiveRes.indexBufferView;
    resource_->indexCount_ = primitiveRes.indexCount;

    // 書き込めるようにする
    resource_->Map();

    //マテリアル

    resource_->GetMaterialData()->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->GetMaterialData()->uvTransform = Math::MakeIdentity4x4();
    resource_->GetMaterialData()->useClampSampler = (primitiveShape_ == PrimitiveType::Ring || primitiveShape_ == PrimitiveType::Cylinder);

    if (textureManager_) {
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        if (!textureNames.empty()) {
            resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);

            // コンボボックス用に selectedIndex を初期化
            auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
            if (it != textureNames.end()) {
                selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
            } else {
                selectedTextureIndex_ = 0;
            }
        }
    }
}

void ParticleSystem::Update() {

    if (isUpdate_ && particleType_ != ParticleType::kHitEffect && 
        particleType_ != ParticleType::kMuzzleSmoke && particleType_ != ParticleType::kMuzzleFlash &&
        particleType_ != ParticleType::kMissileFire && particleType_ != ParticleType::kMissileSmoke &&
        particleType_ != ParticleType::kBulletTrail && particleType_ != ParticleType::kEjectionMist) {
        emitter_.frequencyTime += kDeltatime_; // 時刻を進める
        if (emitter_.frequency <= emitter_.frequencyTime) { // 頻度より大きいなら発生
            particles_.splice(particles_.end(), Emit(emitter_, randomEngine_)); // 発生処理
            emitter_.frequencyTime -= emitter_.frequency; // 余計に過ぎた時間も加味して頻度計算する
        }
    }

    /// カメラの回転を適用する
    billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billboardMatrix_.m[3][0] = 0.0f;
    billboardMatrix_.m[3][1] = 0.0f;
    billboardMatrix_.m[3][2] = 0.0f;

    numInstance_ = 0; // 描画すべきインスタンス数

    for (std::list<Particle>::iterator particleIterator = particles_.begin(); particleIterator != particles_.end();) {

        if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) { // 生存時間を過ぎていたら更新せず描画対象にしない
            particleIterator = particles_.erase(particleIterator); // 生存時間が過ぎたParticleはlistから消す。戻り値が次のイテレーターとなる
            continue;
        }

        if (numInstance_ < kNumMaxInstance_) {
            if (isUpdate_) {
                // パーティクル自身の更新
                particleIterator->Update(kDeltatime_);
                // 振る舞い固有の更新
                behavior_->Update(*particleIterator, kDeltatime_);
            }

            Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(particleIterator->transform.scale);
            Matrix4x4 translateMatrix = Math::MakeTranslateMatrix(particleIterator->transform.translate);
            Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(particleIterator->transform.rotate.x, particleIterator->transform.rotate.y, particleIterator->transform.rotate.z);
            
            Matrix4x4 worldMatrix = Math::MakeIdentity4x4();
            if (useBillboard_) {
                // ビルボードの場合、スケール -> 個別回転(Z) -> ビルボード -> 平行移動 の順で適用
                // 面がカメラを向いた状態で個別に回転させる
                Matrix4x4 rotateZMatrix = Math::MakeRotateZMatrix(particleIterator->transform.rotate.z);
                worldMatrix = Math::Multiply(scaleMatrix, rotateZMatrix);
                worldMatrix = Math::Multiply(worldMatrix, billboardMatrix_);
                worldMatrix = Math::Multiply(worldMatrix, translateMatrix);
            } else {
                worldMatrix = Math::Multiply(scaleMatrix, rotateMatrix);
                worldMatrix = Math::Multiply(worldMatrix, translateMatrix);
            }
            uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
            if (resource_->instancingData_[frameIndex]) {
                resource_->instancingData_[frameIndex][numInstance_].world = worldMatrix;
                // WVPはシェーダー側で計算するため省略
                resource_->instancingData_[frameIndex][numInstance_].WVP = Math::MakeIdentity4x4();
                resource_->instancingData_[frameIndex][numInstance_].color = particleIterator->color;
            }

            numInstance_++; // 生きているParticleの数を1つカウントする

        }

        ++particleIterator; // 次のイテレーターに進める
    }
    resource_->GetMaterialData()->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
    
    resource_->SyncBeforeDraw();
    lastUpdateFrameIndex_ = BaseResource::GetDirectXCommon()->GetFrameIndex();

#if USE_IMGUI
    if (debugLineRegion_) {
        debugLineRegion_->Update();
    }
#endif
}

void ParticleSystem::Draw()
{
    if (!resource_ || !s_drawManager_ || !camera_ || (numInstance_ == 0 && particles_.empty())) {
        return;
    }

    SyncBeforeDraw();

    // 視錐台カリング
    if (isCullingEnabled_) {
        // 放出範囲の対角長(の半分)をベース半径とする
        float emitterRadius = (std::max)({ emitter_.area.x, emitter_.area.y, emitter_.area.z }) * 0.5f;

        // 速度の最大値を取得
        float maxVelX = (std::max)(std::abs(emitter_.velocityMin.x), std::abs(emitter_.velocityMax.x));
        float maxVelY = (std::max)(std::abs(emitter_.velocityMin.y), std::abs(emitter_.velocityMax.y));
        float maxVelZ = (std::max)(std::abs(emitter_.velocityMin.z), std::abs(emitter_.velocityMax.z));
        float maxVel = (std::sqrt)(maxVelX * maxVelX + maxVelY * maxVelY + maxVelZ * maxVelZ);

        // 粒子が拡散する最大距離（ライフタイムを考慮、安全のため5秒分）
        float spreadRadius = maxVel * 5.0f;

        Sphere boundingSphere;
        boundingSphere.center = emitter_.transform.translate;
        boundingSphere.radius = (emitterRadius + spreadRadius) * 1.1f; // 10%のマージン

        if (!Collision::IsCollision(camera_->GetFrustum(), boundingSphere)) {
            return; // 描画処理をスキップ
        }
    }

    // 1) パーティクル本体を描画(選択された Blend/Depth/Cull を描画直前にエンジンへセットして PSO を適用)
    if (s_engine_) {
        // 現在のエンジン状態を保存しておく
        BlendMode prevBlend = s_engine_->currentBlend_;
        PSOManager::DepthWrite prevDepth = s_engine_->currentDepth_;
        PSOManager::CullMode prevCull = s_engine_->currentCull_;

        // 選択値をエンジンにセット(描画直前)
        s_engine_->SetBlend(selectedBlend_);
        s_engine_->SetDepthWrite(selectedDepth_);
        s_engine_->SetCull(selectedCull_);
        s_engine_->ApplyParticlePSO();

        // 描画
        if (s_drawManager_) {
            s_drawManager_->SubmitParticle(resource_.get(), numInstance_);
        }

        // エンジン状態を復元(PSOの切り替えは呼び出し側で制御するため Apply は行わない)
        s_engine_->SetBlend(prevBlend);
        s_engine_->SetDepthWrite(prevDepth);
        s_engine_->SetCull(prevCull);
    } else {
        // エンジン参照がない場合は従来通り(安全策)
        if (s_drawManager_) {
            s_drawManager_->SubmitParticle(resource_.get(), numInstance_);
        }
    }

    // 2) デバッグ線(AABB 等)を描画(Line PSO を確実にバインド)
#if USE_IMGUI
    if (debugLineRegion_) {
        debugLineRegion_->Draw();
    }
#endif
}

void ParticleSystem::SyncBeforeDraw() {
    uint32_t currentFrameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
    
    // 今フレームですでにUpdate()が呼ばれている場合は、正しいデータが書き込み済みのためスキップ
    if (lastUpdateFrameIndex_ == currentFrameIndex) {
        resource_->SyncBeforeDraw();
        return;
    }

    // 前回のデータ元のフレームインデックス
    uint32_t prevFrameIndex = (currentFrameIndex + kMaxFramesInFlight - 1) % kMaxFramesInFlight;

    // 前フレームのデータをそのまま丸ごと現在のフレームのバッファへコピーする（明滅対策）
    if (numInstance_ > 0 && resource_->instancingData_[currentFrameIndex] && resource_->instancingData_[prevFrameIndex]) {
        std::memcpy(resource_->instancingData_[currentFrameIndex], 
                    resource_->instancingData_[prevFrameIndex], 
                    sizeof(ParticleForGPU) * numInstance_);
    }
    
    resource_->SyncBeforeDraw();
    lastUpdateFrameIndex_ = currentFrameIndex; // これ以降このフレームでの同期は不要
}

void ParticleSystem::SetEmitterPosition(const Vector3& position) {
    emitter_.transform.translate = position;
}

void ParticleSystem::SetEmitterArea(const Vector3& area) {
    emitter_.area = area;
}

void ParticleSystem::SetEmitterVelocity(const Vector3& minVel, const Vector3& maxVel) {
    emitter_.velocityMin = minVel;
    emitter_.velocityMax = maxVel;
}

void ParticleSystem::SetEmitterFrequency(float frequency) {
    emitter_.frequency = frequency;
}

void ParticleSystem::SetEmitterCount(uint32_t count) {
    emitter_.count = count;
}

void ParticleSystem::SetParticleScale(const Vector3& start, const Vector3& end) {
    emitter_.startScale = start;
    emitter_.endScale = end;
}

void ParticleSystem::SetParticleColor(const Vector4& start, const Vector4& end) {
    emitter_.startColor = start;
    emitter_.endColor = end;
}

// 既存のSetParticleColorの下に追加
void ParticleSystem::SetParticleColorMode(ParticleColorMode mode) {
    emitter_.colorMode = mode;
}

void ParticleSystem::SetEmitterProperties(
    const Vector3& position,
    const Vector3& area,
    const Vector3& minVel,
    const Vector3& maxVel,
    float frequency,
    uint32_t count) {
    SetEmitterPosition(position);
    SetEmitterArea(area);
    SetEmitterVelocity(minVel, maxVel);
    SetEmitterFrequency(frequency);
    SetEmitterCount(count);
}

void ParticleSystem::SetTexture(const std::string& textureFilePath) {
    if (!textureManager_) {
        return;
    }
    auto textureNames = textureManager_->GetTextureNamesForDebug();
    auto it = std::find(textureNames.begin(), textureNames.end(), textureFilePath);

    if (it != textureNames.end()) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureFilePath);
        selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
    }
}

Particle ParticleSystem::MakeNewParticle(std::mt19937& randomEngine, const Emitter& emitter) {
    std::uniform_real_distribution<float> distRange(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
    std::uniform_real_distribution<float> distVelocityX(emitter.velocityMin.x, emitter.velocityMax.x);
    std::uniform_real_distribution<float> distVelocityY(emitter.velocityMin.y, emitter.velocityMax.y);
    std::uniform_real_distribution<float> distVelocityZ(emitter.velocityMin.z, emitter.velocityMax.z);

    Particle particle;

    // 振る舞い固有の初期化
    behavior_->MakeNewParticle(particle, randomEngine, emitter);

    particle.transform.scale = particle.startScale;

    Vector3 randomTranslate = {
        distRange(randomEngine) * emitter.area.x / 2.0f,
        distRange(randomEngine) * emitter.area.y / 2.0f,
        distRange(randomEngine) * emitter.area.z / 2.0f
    };
    particle.transform.translate = emitter.transform.translate + randomTranslate;
    particle.velocity = { distVelocityX(randomEngine), distVelocityY(randomEngine), distVelocityZ(randomEngine) };

    // カラーモードに応じて色を決定
    switch (emitter.colorMode) {
    case ParticleColorMode::kNone:
        particle.startColor = emitter.startColor;
        particle.endColor = emitter.endColor;
        break;
    case ParticleColorMode::kRandom:
        particle.startColor = { distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), 1.0f };
        particle.endColor = particle.startColor;
        particle.endColor.w = 0.0f;
        break;
    case ParticleColorMode::kRed:
        particle.startColor = { distColor(randomEngine), 0.0f, 0.0f, 1.0f };
        particle.endColor = particle.startColor;
        particle.endColor.w = 0.0f;
        break;
    case ParticleColorMode::kGreen:
        particle.startColor = { 0.0f, distColor(randomEngine), 0.0f, 1.0f };
        particle.endColor = particle.startColor;
        particle.endColor.w = 0.0f;
        break;
    case ParticleColorMode::kBlue:
        particle.startColor = { 0.0f, 0.0f, distColor(randomEngine), 1.0f };
        particle.endColor = particle.startColor;
        particle.endColor.w = 0.0f;
        break;
    }

    particle.color = particle.startColor;
    particle.currentTime = 0.0f;

    return particle;
}

std::list<Particle> ParticleSystem::Emit(const Emitter& emitter, std::mt19937& randomEngine) {
    std::list<Particle> particles;
    for (uint32_t count = 0; count < emitter.count; ++count) {
        particles.push_back(MakeNewParticle(randomEngine, emitter));
    }
    return particles;
}

void ParticleSystem::PlayHitEffect(const Vector3& position) {
    if (particleType_ == ParticleType::kHitEffect || 
        particleType_ == ParticleType::kMuzzleSmoke || 
        particleType_ == ParticleType::kMuzzleFlash ||
        particleType_ == ParticleType::kMissileFire ||
        particleType_ == ParticleType::kMissileSmoke ||
        particleType_ == ParticleType::kBulletTrail ||
        particleType_ == ParticleType::kEjectionMist) {
        emitter_.transform.translate = position;
        particles_.splice(particles_.end(), Emit(emitter_, randomEngine_));
    }
}

void ParticleSystem::PlayHitEffect(const Vector3& position, uint32_t count) {
    if (particleType_ == ParticleType::kHitEffect || 
        particleType_ == ParticleType::kMuzzleSmoke || 
        particleType_ == ParticleType::kMuzzleFlash ||
        particleType_ == ParticleType::kMissileFire ||
        particleType_ == ParticleType::kMissileSmoke ||
        particleType_ == ParticleType::kBulletTrail ||
        particleType_ == ParticleType::kEjectionMist) {
        Emitter customEmitter = emitter_;
        customEmitter.transform.translate = position;
        customEmitter.count = count;
        particles_.splice(particles_.end(), Emit(customEmitter, randomEngine_));
    }
}

void ParticleSystem::Debug([[maybe_unused]] const char* particleName) {

#if USE_IMGUI
    if (debugLineRegion_) {
        debugLineRegion_->ClearInstances();
    }

    // Emitter AABB をフラグで制御して描画
    if (showEmitterAABB_) {
        AABB emitterAABB{
            .min = emitter_.transform.translate - emitter_.area / 2.0f,
            .max = emitter_.transform.translate + emitter_.area / 2.0f
        };
        DrawAABB(emitterAABB, { 0.0f, 1.0f, 0.0f, 1.0f });
    }

    if (s_ui_) {
        std::string name = std::string("Particle: ") + particleName;

        //ImGui

        //ウィンドウを作り出す
        ImGui::Begin(name.c_str());

        // ここで表示切替チェックボックスを追加
        ImGui::Checkbox("Show Emitter AABB", &showEmitterAABB_);
        ImGui::Checkbox("Show Field AABB", &showFieldAABB_);

        // PSO設定のデバッグUIを呼び出す
        s_ui_->DebugPsoSettings(&selectedBlend_, &selectedDepth_, &selectedCull_, "##Particle");

        if (ImGui::BeginTabBar("ParticleTabs")) {
            // Generalタブ
            if (ImGui::BeginTabItem("General")) {
                if (ImGui::Button("Add Particle")) {
                    switch (particleType_) {
                    case ParticleType::kHitEffect:
                        PlayHitEffect(emitter_.transform.translate);
                        break;
                    default:
                        particles_.splice(particles_.end(), Emit(emitter_, randomEngine_));
                        break;
                    }
                }

                ImGui::Checkbox("update", &isUpdate_);
                ImGui::Checkbox("useBillboard", &useBillboard_);
                ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);

                ImGui::Separator();

                // PrimitiveShapeの選択UI
                const char* primitiveShapeNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring", "Skybox" };
                int currentShape = static_cast<int>(primitiveShape_);
                if (ImGui::Combo("Primitive Shape", &currentShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                    if (primitiveShape_ != static_cast<PrimitiveType>(currentShape)) {
                        primitiveShape_ = static_cast<PrimitiveType>(currentShape);
                        std::string currentTextureName = "resources/circle.png";
                        if (textureManager_) {
                            auto textureNames = textureManager_->GetTextureNamesForDebug();
                            if (selectedTextureIndex_ >= 0 && selectedTextureIndex_ < static_cast<int>(textureNames.size())) {
                                currentTextureName = textureNames[selectedTextureIndex_];
                            }
                        }
                        Initialize(camera_, currentTextureName, particleType_, primitiveShape_);
                    }
                }

                // ParticleTypeの選択UI
                const char* particleTypeNames[] = { "Normal", "AccelerationField", "HitEffect", "Explosion" };
                int currentType = static_cast<int>(particleType_);
                if (ImGui::Combo("Particle Type", &currentType, particleTypeNames, IM_ARRAYSIZE(particleTypeNames))) {
                    ChangeBehavior(static_cast<ParticleType>(currentType));
                }
                ImGui::EndTabItem();
            }

            // Emitterタブ
            if (ImGui::BeginTabItem("Emitter")) {
                ImGui::DragFloat3("Translate", &emitter_.transform.translate.x, 0.01f, -100.0f, 100.0f);
                ImGui::DragFloat3("Area", &emitter_.area.x, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat3("Velocity Min", &emitter_.velocityMin.x, 0.1f, -10.0f, 10.0f);
                ImGui::DragFloat3("Velocity Max", &emitter_.velocityMax.x, 0.1f, -10.0f, 10.0f);
                ImGui::DragInt("Count", reinterpret_cast<int*>(&emitter_.count), 1, 1, 100);
                ImGui::DragFloat("Frequency", &emitter_.frequency, 0.01f, 0.01f, 10.0f);

                ImGui::Separator();
                ImGui::Text("Particle Lifetime Properties");
                ImGui::DragFloat3("Start Scale", &emitter_.startScale.x, 0.01f);
                ImGui::DragFloat3("End Scale", &emitter_.endScale.x, 0.01f);

                const char* colorModeNames[] = { "None", "Random", "Red", "Green", "Blue" };
                int currentMode = static_cast<int>(emitter_.colorMode);
                if (ImGui::Combo("Color Mode", &currentMode, colorModeNames, IM_ARRAYSIZE(colorModeNames))) {
                    emitter_.colorMode = static_cast<ParticleColorMode>(currentMode);
                }

                if (emitter_.colorMode == ParticleColorMode::kNone) {
                    ImGui::ColorEdit4("Start Color", &emitter_.startColor.x);
                    ImGui::ColorEdit4("End Color", &emitter_.endColor.x);
                }
                ImGui::EndTabItem();
            }

            // Fieldタブ
            if (ImGui::BeginTabItem("Behavior")) {
                behavior_->Debug(&emitter_, s_ui_, this);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Rendering")) {
                s_ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
                s_ui_->DebugMaterialByParticle(resource_->GetMaterialData());
                s_ui_->DebugUvTransform(resource_->uvTransform_);

                // --- Ring パラメータ UI ---
                if (primitiveShape_ == PrimitiveType::Ring) {
                    ImGui::Separator();
                    ImGui::Text("Ring Parameters");

                    // 現在の値をローカルにコピーして UI 編集(変更検出用)
                    float inner = ringInnerRadius_;
                    float outer = ringOuterRadius_;
                    float startDeg = ringStartAngleDeg_;
                    float endDeg = ringEndAngleDeg_;
                    int segments = static_cast<int>(ringSegmentCount_);
                    bool verticalUV = ringVerticalUV_;

                    bool changed = false;
                    if (ImGui::DragFloat("Inner Radius", &inner, 0.005f, 0.0f, 1000.0f)) changed = true;
                    if (ImGui::DragFloat("Outer Radius", &outer, 0.005f, 0.0f, 1000.0f)) changed = true;
                    if (ImGui::DragFloat("Start Angle (deg)", &startDeg, 0.5f, -360.0f, 360.0f)) changed = true;
                    if (ImGui::DragFloat("End Angle (deg)", &endDeg, 0.5f, -360.0f, 720.0f)) changed = true;
                    if (ImGui::DragInt("Segment Count", &segments, 1.0f, 3, 1024)) changed = true;
                    if (ImGui::Checkbox("Vertical UV", &verticalUV)) changed = true;

                    if (changed) {
                        // 安全化: segments を最低 3 に、inner/outer の順序を保証
                        segments = std::max(3, segments);
                        if (inner < 0.0f) inner = 0.0f;
                        if (outer < 0.0f) outer = 0.0f;
                        if (inner > outer) std::swap(inner, outer);

                        // 値をセットして Initialize で再生成
                        SetRingParameters(inner, outer, startDeg, endDeg, static_cast<uint32_t>(segments), verticalUV);

                        // 現在のテクスチャ名を復元して Initialize を呼ぶ(UI 保持のため)
                        std::string currentTextureName = "resources/circle.png";
                        if (textureManager_) {
                            auto textureNames = textureManager_->GetTextureNamesForDebug();
                            if (selectedTextureIndex_ >= 0 && selectedTextureIndex_ < static_cast<int>(textureNames.size())) {
                                currentTextureName = textureNames[selectedTextureIndex_];
                            } else {
                                currentTextureName = textureNames[0];
                            }
                        }
                        Initialize(camera_, currentTextureName, particleType_, primitiveShape_);
                    }
                }

                // --- Cylinder パラメータ UI ---
                if (primitiveShape_ == PrimitiveType::Cylinder) {
                    ImGui::Separator();
                    ImGui::Text("Cylinder Parameters");

                    float radius = cylinderRadius_;
                    float height = cylinderHeight_;
                    int segments = static_cast<int>(cylinderSegmentCount_);
                    bool flipV = cylinderFlipV_;

                    bool changed = false;
                    if (ImGui::DragFloat("Radius", &radius, 0.005f, 0.0f, 1000.0f)) changed = true;
                    if (ImGui::DragFloat("Height", &height, 0.01f, 0.0f, 1000.0f)) changed = true;
                    if (ImGui::DragInt("Segment Count", &segments, 1.0f, 3, 1024)) changed = true;
                    if (ImGui::Checkbox("Flip V", &flipV)) changed = true;

                    if (changed) {
                        segments = std::max(3, segments);
                        if (radius < 0.0f) radius = 0.0f;
                        if (height < 0.0f) height = 0.0f;

                        SetCylinderParameters(radius, height, static_cast<uint32_t>(segments), flipV);

                        std::string currentTextureName = "resources/circle.png";
                        if (textureManager_) {
                            auto textureNames = textureManager_->GetTextureNamesForDebug();
                            if (!textureNames.empty()) {
                                if (selectedTextureIndex_ >= 0 && selectedTextureIndex_ < static_cast<int>(textureNames.size())) {
                                    currentTextureName = textureNames[selectedTextureIndex_];
                                } else {
                                    currentTextureName = textureNames[0];
                                }
                            }
                        }
                        Initialize(camera_, currentTextureName, particleType_, primitiveShape_);
                    }
                }

                ImGui::EndTabItem();
            }

            // インスタンスタブ
            if (ImGui::BeginTabItem("Instances")) {
                uint32_t index = 0;
                for (Particle& particle : particles_) {
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "%d", index++);
                    s_ui_->TextTransform(particle.transform, buf);
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        //入力終了
        ImGui::End();
    }
#endif // _DEBUG

}

void ParticleSystem::ChangeBehavior(ParticleType type, bool force) {
    if (!force && particleType_ == type && behavior_) {
        return; // 同じ振る舞いなら何もしない
    }
    particleType_ = type;
    behavior_ = CreateParticleBehavior(type);
    behavior_->Initialize(&emitter_);

    // ビルボード設定も振る舞いに応じて変更
    if (type == ParticleType::kHitEffect) {
        useBillboard_ = false;
    } else {
        useBillboard_ = true;
    }
}

void ParticleSystem::DrawAABB(const AABB& aabb, const Vector4& color)
{
#if USE_IMGUI
    if (!debugLineRegion_) return;

    Vector3 vertices[8];
    vertices[0] = { aabb.min.x, aabb.min.y, aabb.min.z };
    vertices[1] = { aabb.max.x, aabb.min.y, aabb.min.z };
    vertices[2] = { aabb.min.x, aabb.max.y, aabb.min.z };
    vertices[3] = { aabb.max.x, aabb.max.y, aabb.min.z };
    vertices[4] = { aabb.min.x, aabb.min.y, aabb.max.z };
    vertices[5] = { aabb.max.x, aabb.min.y, aabb.max.z };
    vertices[6] = { aabb.min.x, aabb.max.y, aabb.max.z };
    vertices[7] = { aabb.max.x, aabb.max.y, aabb.max.z };

    // 底面
    debugLineRegion_->AddInstance(vertices[0], vertices[1], color);
    debugLineRegion_->AddInstance(vertices[1], vertices[3], color);
    debugLineRegion_->AddInstance(vertices[3], vertices[2], color);
    debugLineRegion_->AddInstance(vertices[2], vertices[0], color);

    // 上面
    debugLineRegion_->AddInstance(vertices[4], vertices[5], color);
    debugLineRegion_->AddInstance(vertices[5], vertices[7], color);
    debugLineRegion_->AddInstance(vertices[7], vertices[6], color);
    debugLineRegion_->AddInstance(vertices[6], vertices[4], color);

    // 側面
    debugLineRegion_->AddInstance(vertices[0], vertices[4], color);
    debugLineRegion_->AddInstance(vertices[1], vertices[5], color);
    debugLineRegion_->AddInstance(vertices[2], vertices[6], color);
    debugLineRegion_->AddInstance(vertices[3], vertices[7], color);
#endif
}

// SetRingParameters
void ParticleSystem::SetRingParameters(float innerRadius, float outerRadius,
    float startAngleDeg, float endAngleDeg,
    uint32_t segmentCount, bool verticalUV) {
    // 最低分割数を確保
    ringSegmentCount_ = std::max<uint32_t>(3, segmentCount);
    ringInnerRadius_ = innerRadius;
    ringOuterRadius_ = outerRadius;
    ringStartAngleDeg_ = startAngleDeg;
    ringEndAngleDeg_ = endAngleDeg;
    ringVerticalUV_ = verticalUV;
}

// SetCylinderParameters
void ParticleSystem::SetCylinderParameters(float radius, float height, uint32_t segmentCount, bool flipV) {
    cylinderRadius_ = radius;
    cylinderHeight_ = height;
    cylinderSegmentCount_ = std::max<uint32_t>(3, segmentCount);
    cylinderFlipV_ = flipV;
}