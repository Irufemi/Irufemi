#include "ParticleSystem.h"
#include "Math.h"
#include "function/Math.h"
#include "manager/DebugUI.h"
#include "engine/directX/DirectXCommon.h"
#include "engine/directX/DescriptorPool.h"
#include <algorithm>

DescriptorPool* ParticleSystem::s_srvPool_ = nullptr;
TextureManager* ParticleSystem::s_textureManager_ = nullptr;
DebugUI* ParticleSystem::s_ui_ = nullptr;

ParticleSystem::~ParticleSystem() {
    if (instancingSrvIndex_ != UINT32_MAX && s_srvPool_ && resource_) {
        if (auto* dx = resource_->GetDirectXCommon()) {
            s_srvPool_->FreeAfterFence(instancingSrvIndex_, dx->GetFenceValue());
        }
        instancingSrvIndex_ = UINT32_MAX;
        instancingSrvHandleCPU_ = {};
        instancingSrvHandleGPU_ = {};
    }
}

void ParticleSystem::Initialize(Camera* camera, const std::string& textureName, ParticleType type, PrimitiveShape shape) {
    this->camera_ = camera;
    this->primitiveShape_ = shape;

    isUpdate_ = true;
    randomEngine_.seed(seedGenerator_());

    // 初回呼び出し時のみリソースを生成
    if (!resource_) {
        resource_ = std::make_unique<D3D12ResourceUtilParticle>();
    }
    if (!instancingResource_) {
        // Instancing 用バッファ
        instancingResource_ = resource_->GetDirectXCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance_);
        instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));
    }

    // 振る舞いを設定
    ChangeBehavior(type, true); // 強制的に更新

    // 単位行列を書きこんでおく
    particles_.clear();
    numInstance_ = 0;

    // backToFrontMatrix_の設定(面の向きをカメラの方向にしてあるのでここは調整なし。0でOK)
    backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);

    /// カメラの回転を適用する
    billbordMatrix_ = Math::MakeIdentity4x4();
    billbordMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billbordMatrix_.m[3][0] = 0.0f;
    billbordMatrix_.m[3][1] = 0.0f;
    billbordMatrix_.m[3][2] = 0.0f;

    D3D12_SHADER_RESOURCE_VIEW_DESC instancingDesc{};
    instancingDesc.Format = DXGI_FORMAT_UNKNOWN;
    instancingDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingDesc.Buffer.FirstElement = 0;
    instancingDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    instancingDesc.Buffer.NumElements = kNumMaxInstance_;
    instancingDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

    // SRV スロット確保（初回のみ）
    if (instancingSrvIndex_ == UINT32_MAX) {
        auto* alloc = s_srvPool_;
        if (!alloc) {
            OutputDebugStringA("ParticleSystem::Initialize: SRV allocator is null\n");
        } else {
            uint32_t idx = alloc->Allocate();
            if (idx == DescriptorPool::kInvalid) {
                OutputDebugStringA("ParticleSystem::Initialize: SRV Allocate failed\n");
            } else {
                instancingSrvIndex_ = idx;
                instancingSrvHandleCPU_ = alloc->GetCPUHandle(idx);
                instancingSrvHandleGPU_ = alloc->GetGPUHandle(idx);
            }
        }
    }

    // 既存の静的インデックス運用は廃止。確保できている場合のみ SRV を作成
    if (instancingSrvHandleCPU_.ptr != 0) {
        resource_->GetDirectXCommon()->GetDevice()->CreateShaderResourceView(instancingResource_.Get(), &instancingDesc, instancingSrvHandleCPU_);
    }

    // 頂点/インデックスデータをクリア
    resource_->vertexDataList_.clear();
    resource_->indexDataList_.clear();

    switch (primitiveShape_) {
    case PrimitiveShape::Plane:
    {
        //左下
        resource_->vertexDataList_.push_back({ { -0.5f,-0.5f,0.0f,1.0f }, { 0.0f,1.0f } });
        //左上
        resource_->vertexDataList_.push_back({ { -0.5f,0.5f,0.0f,1.0f  }, { 0.0f,0.0f} });
        //右下
        resource_->vertexDataList_.push_back({ { 0.5f,-0.5f,0.0f,1.0f }, { 1.0f,1.0f } });
        //右上
        resource_->vertexDataList_.push_back({ { 0.5f,0.5f,0.0f,1.0f }, { 1.0f,0.0f } });

        for (uint32_t i = 0; i < static_cast<uint32_t>(resource_->vertexDataList_.size()); ++i) {
            resource_->vertexDataList_[i].normal.x = 0.0f;
            resource_->vertexDataList_[i].normal.y = 0.0f;
            resource_->vertexDataList_[i].normal.z = -1.0f;
        }

        resource_->indexDataList_.push_back(0);
        resource_->indexDataList_.push_back(1);
        resource_->indexDataList_.push_back(2);
        resource_->indexDataList_.push_back(1);
        resource_->indexDataList_.push_back(3);
        resource_->indexDataList_.push_back(2);
    }
    break;
    case PrimitiveShape::Sphere:
    {
        const uint32_t kSubdivision = 16;
        const float kRadius = 0.5f;
        const uint32_t kLatCount = kSubdivision; // 緯度分割数
        const uint32_t kLonCount = kSubdivision; // 経度分割数

        // 頂点データの生成
        for (uint32_t lat = 0; lat <= kLatCount; ++lat) {
            float theta = static_cast<float>(lat) / kLatCount * std::numbers::pi_v<float>;
            for (uint32_t lon = 0; lon <= kLonCount; ++lon) {
                float phi = static_cast<float>(lon) / kLonCount * 2.0f * std::numbers::pi_v<float>;

                VertexData vertex;
                vertex.position.x = kRadius * std::sin(theta) * std::cos(phi);
                vertex.position.y = kRadius * std::cos(theta);
                vertex.position.z = kRadius * std::sin(theta) * std::sin(phi);
                vertex.position.w = 1.0f;

                vertex.normal = { vertex.position.x, vertex.position.y, vertex.position.z };
                vertex.texcoord = { static_cast<float>(lon) / kLonCount, static_cast<float>(lat) / kLatCount };

                resource_->vertexDataList_.push_back(vertex);
            }
        }

        // インデックスデータの生成
        for (uint32_t lat = 0; lat < kLatCount; ++lat) {
            for (uint32_t lon = 0; lon < kLonCount; ++lon) {
                uint32_t i0 = lat * (kLonCount + 1) + lon;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (lat + 1) * (kLonCount + 1) + lon;
                uint32_t i3 = i2 + 1;

                resource_->indexDataList_.push_back(i0);
                resource_->indexDataList_.push_back(i2);
                resource_->indexDataList_.push_back(i1);

                resource_->indexDataList_.push_back(i1);
                resource_->indexDataList_.push_back(i2);
                resource_->indexDataList_.push_back(i3);
            }
        }
    }
    break;
    }

    // リソースのメモリを確保（または再利用）
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    //頂点バッファ

    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};

    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());

    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    //リソースの先頭のアドレスから使う
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    //使用するリソースのサイズ
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(resource_->indexDataList_.size());
    //インデックスはint32_tとする
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    ///IndexResourceにデータを書き込む

    //インデックスリソースにデータを書き込む

    std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);

    //マテリアル

    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->materialData_->enableLighting = true;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();

    if (s_textureManager_) {
        auto textureNames = s_textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        if (!textureNames.empty()) {

            resource_->textureHandle_ = s_textureManager_->GetTextureHandle(textureName);

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

    if (isUpdate_ && particleType_ != ParticleType::kHitEffect) {
        emitter_.frequencyTime += kDeltatime_; // 時刻を進める
        if (emitter_.frequency <= emitter_.frequencyTime) { // 頻度より大きいなら発生
            particles_.splice(particles_.end(), Emit(emitter_, randomEngine_)); // 発生処理
            emitter_.frequencyTime -= emitter_.frequency; // 余計に過ぎた時間も加味して頻度計算する
        }
    }

    /// カメラの回転を適用する
    billbordMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billbordMatrix_.m[3][0] = 0.0f;
    billbordMatrix_.m[3][1] = 0.0f;
    billbordMatrix_.m[3][2] = 0.0f;

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
            Matrix4x4 worldMatrix = Math::MakeIdentity4x4();
            if (useBillbord_) {
                worldMatrix = Math::Multiply(Math::Multiply(scaleMatrix, billbordMatrix_), translateMatrix);
            } else {
                Matrix4x4 rotateMatrix = Math::MakeRotateZMatrix(particleIterator->transform.rotate.z);
                worldMatrix = Math::Multiply(scaleMatrix, rotateMatrix);
                worldMatrix = Math::Multiply(worldMatrix, translateMatrix);
            }
            Matrix4x4 worldViewProjectionMatrix = Math::Multiply(worldMatrix, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
            instancingData_[numInstance_].world = worldMatrix;
            instancingData_[numInstance_].WVP = worldViewProjectionMatrix;
            instancingData_[numInstance_].color = particleIterator->color;

            numInstance_++; // 生きているParticleの数を1つカウントする

        }

        ++particleIterator; // 次のイテレーターに進める
    }
    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
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
    if (!s_textureManager_) {
        return;
    }
    auto textureNames = s_textureManager_->GetTextureNames();
    std::sort(textureNames.begin(), textureNames.end());
    auto it = std::find(textureNames.begin(), textureNames.end(), textureFilePath);

    if (it != textureNames.end()) {
        resource_->textureHandle_ = s_textureManager_->GetTextureHandle(textureFilePath);
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
    if (particleType_ == ParticleType::kHitEffect) {
        emitter_.transform.translate = position;
        particles_.splice(particles_.end(), Emit(emitter_, randomEngine_));
    }
}

void ParticleSystem::Debug([[maybe_unused]] const char* particleName) {

#if USE_IMGUI
    if (s_ui_) {
        std::string name = std::string("Particle: ") + particleName;

        //ImGui

        //ウィンドウを作り出す
        ImGui::Begin(name.c_str());

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
                ImGui::Checkbox("useBillbord", &useBillbord_);

                ImGui::Separator();

                // PrimitiveShapeの選択UI
                const char* primitiveShapeNames[] = { "Plane", "Sphere" };
                int currentShape = static_cast<int>(primitiveShape_);
                if (ImGui::Combo("Primitive Shape", &currentShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                    if (primitiveShape_ != static_cast<PrimitiveShape>(currentShape)) {
                        primitiveShape_ = static_cast<PrimitiveShape>(currentShape);
                        std::string currentTextureName = "resources/circle.png";
                        if (s_textureManager_) {
                            auto textureNames = s_textureManager_->GetTextureNames();
                            std::sort(textureNames.begin(), textureNames.end());
                            if (selectedTextureIndex_ >= 0 && selectedTextureIndex_ < textureNames.size()) {
                                currentTextureName = textureNames[selectedTextureIndex_];
                            }
                        }
                        Initialize(camera_, currentTextureName, particleType_, primitiveShape_);
                    }
                }

                // ParticleTypeの選択UI
                const char* particleTypeNames[] = { "Normal", "AccelerationField", "HitEffect" };
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
                behavior_->Debug(&emitter_, s_ui_);
                ImGui::EndTabItem();
            }

            // レンダリングタブ
            if (ImGui::BeginTabItem("Rendering")) {
                s_ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
                s_ui_->DebugMaterialBy3D(resource_->materialData_);
                s_ui_->DebugUvTransform(resource_->uvTransform_);
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
        useBillbord_ = false;
    } else {
        useBillbord_ = true;
    }
}