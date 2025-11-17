#include "ParticleClass.h"
#include "Math.h"
#include "../externals/imgui/imgui.h"
#include "engine/directX/DirectXCommon.h"
#include "engine/directX/DescriptorPool.h" // 追加
#include <algorithm>

DescriptorPool* ParticleClass::s_srvPool_ = nullptr;
TextureManager* ParticleClass::s_textureManager_ = nullptr;
DebugUI*        ParticleClass::s_ui_ = nullptr;

ParticleClass::~ParticleClass() {
    if (instancingSrvIndex_ != UINT32_MAX && s_srvPool_ && resource_) {
        if (auto* dx = resource_->GetDirectXCommon()) {
            s_srvPool_->FreeAfterFence(instancingSrvIndex_, dx->GetFenceValue());
        }
        instancingSrvIndex_ = UINT32_MAX;
        instancingSrvHandleCPU_ = {};
        instancingSrvHandleGPU_ = {};
    }
}

void ParticleClass::Initialize(Camera* camera, const std::string& textureName, ParticleType type) {
    this->camera_ = camera;
    this->particleType_ = type;

    useBillbord_ = true;
    isUpdate_ = true;
    randomEngine_.seed(seedGenerator_());

    // Instancing 用バッファ
    instancingResource_ = resource_->GetDirectXCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance_);
    instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));


    // countが3コのemitterを作成しておく
    emitter_.count = 3;
    emitter_.frequency = 0.5f; // 0.5秒ごとに発生
    emitter_.frequencyTime = 0.0f; // 発生頻度用の時刻、0で初期化
    emitter_.transform.translate = { 0.0f,0.0f,0.0f };
    emitter_.transform.rotate = { 0.0f,0.0f,0.0f };
    emitter_.transform.scale = { 1.0f,1.0f,1.0f };
    emitter_.area = { 2.0f, 2.0f, 2.0f };
    emitter_.velocityMin = { -1.0f, -1.0f, -1.0f };
    emitter_.velocityMax = { 1.0f, 1.0f, 1.0f };

    switch (particleType_) {
    case ParticleType::kAccelerationField:
        accelerationField_.acceleration = { 15.0f,0.0f,0.0f };
        accelerationField_.area.min = { -1.0f,-1.0f,-1.0f };
        accelerationField_.area.max = { 1.0f,1.0f,1.0f };
        break;
    }

    // 単位行列を書きこんでおく
    particles_.clear();
    for (uint32_t i = 0; i < kNumMaxInstance_; ++i) {
        particles_.push_back(MakeNewParticle(randomEngine_, emitter_));
    }

    /// カメラの回転を適用する
    billbordMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billbordMatrix_.m[3][0] = 0.0f;
    billbordMatrix_.m[3][1] = 0.0f;
    billbordMatrix_.m[3][2] = 0.0f;

    for (std::list<Particle>::iterator particleIterator = particles_.begin(); particleIterator != particles_.end(); ++particleIterator) {
        // 位置と速度を[-1,1]でランダムに初期化
        Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(particleIterator->transform.scale);
        Matrix4x4 translateMatrix = Math::MakeTranslateMatrix(particleIterator->transform.translate);
        Matrix4x4 worldMatrix = Math::MakeIdentity4x4();
        if (useBillbord_) {
            worldMatrix = Math::Multiply(Math::Multiply(scaleMatrix, billbordMatrix_), translateMatrix);
        } else {
            worldMatrix = Math::MakeAffineMatrix(particleIterator->transform.scale, particleIterator->transform.rotate, particleIterator->transform.translate);
        }
        Matrix4x4 worldViewProjectionMatrix = Math::Multiply(worldMatrix, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
        instancingData_[numInstance_].world = worldMatrix;
        instancingData_[numInstance_].WVP = worldViewProjectionMatrix;
        instancingData_[numInstance_].color = particleIterator->color;
    }

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
            OutputDebugStringA("ParticleClass::Initialize: SRV allocator is null\n");
        } else {
            uint32_t idx = alloc->Allocate();
            if (idx == DescriptorPool::kInvalid) {
                OutputDebugStringA("ParticleClass::Initialize: SRV Allocate failed\n");
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

    // D3D12ResourceUtilを生成
    resource_ = std::make_unique<D3D12ResourceUtilParticle>();

    //左下
    resource_->vertexDataList_.push_back({ { -0.5f,-0.5f,0.0f,1.0f }, { 0.0f,1.0f } });
    //左上
    resource_->vertexDataList_.push_back({ { -0.5f,0.5f,0.0f,1.0f  }, { 0.0f,0.0f} });
    //右下
    resource_->vertexDataList_.push_back({ { 0.5f,-0.5f,0.0f,1.0f }, { 1.0f,1.0f } });
    //右上
    resource_->vertexDataList_.push_back({ { 0.5f,0.5f,0.0f,1.0f }, { 1.0f,0.0f } });

    for (uint32_t i = 0; i < static_cast<uint32_t>(resource_->vertexDataList_.size()); ++i) {
        resource_->vertexDataList_[i].normal.x = resource_->vertexDataList_[i].position.x;
        resource_->vertexDataList_[i].normal.y = resource_->vertexDataList_[i].position.y;
        resource_->vertexDataList_[i].normal.z = -1.0f;
    }

    resource_->indexDataList_.push_back(0);
    resource_->indexDataList_.push_back(1);
    resource_->indexDataList_.push_back(2);
    resource_->indexDataList_.push_back(1);
    resource_->indexDataList_.push_back(3);
    resource_->indexDataList_.push_back(2);

    // リソースのメモリを確保
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

void ParticleClass::Update(const char* particleName) {

#if defined(_DEBUG) || defined(DEVELOPMENT)
    if (s_ui_) {
        std::string name = std::string("Particle: ") + particleName;

        //ImGui

        //ウィンドウを作り出す
        ImGui::Begin(name.c_str());

        if (ImGui::Button("Add Particle")) {
            particles_.splice(particles_.end(), Emit(emitter_, randomEngine_));
        }

        ImGui::Checkbox("update", &isUpdate_);

        ImGui::Checkbox("useBillbord", &useBillbord_);

        if (ImGui::CollapsingHeader("Emitter")) {
            ImGui::DragFloat3("Translate", &emitter_.transform.translate.x, 0.01f, -100.0f, 100.0f);
            ImGui::DragFloat3("Area", &emitter_.area.x, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat3("Velocity Min", &emitter_.velocityMin.x, 0.1f, -10.0f, 10.0f);
            ImGui::DragFloat3("Velocity Max", &emitter_.velocityMax.x, 0.1f, -10.0f, 10.0f);
            ImGui::DragInt("Count", reinterpret_cast<int*>(&emitter_.count), 1, 1, 100);
            ImGui::DragFloat("Frequency", &emitter_.frequency, 0.01f, 0.01f, 10.0f);
        }

        // ParticleTypeの選択UI
        const char* particleTypeNames[] = { "AccelerationField" };
        int currentType = static_cast<int>(particleType_);
        if (ImGui::Combo("Particle Type", &currentType, particleTypeNames, IM_ARRAYSIZE(particleTypeNames))) {
            particleType_ = static_cast<ParticleType>(currentType);
        }

        switch (particleType_) {
        case ParticleType::kAccelerationField:
            ImGui::DragFloat3("Acceleration", &accelerationField_.acceleration.x, 0.1f);
            ImGui::DragFloat3("Area Min", &accelerationField_.area.min.x, 0.1f);
            ImGui::DragFloat3("Area Max", &accelerationField_.area.max.x, 0.1f);
            break;
        }

        s_ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

        s_ui_->DebugMaterialBy3D(resource_->materialData_);

        s_ui_->DebugUvTransform(resource_->uvTransform_);

        if (ImGui::CollapsingHeader("InstanceTransform")) {

            uint32_t index = 0;

            for (Particle& particle : particles_) {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%d", index++);
                s_ui_->TextTransform(particle.transform, buf);
            }
        }

        //入力終了
        ImGui::End();
    }
#endif // _DEBUG

    if (isUpdate_) {
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
                switch (particleType_) {
                case ParticleType::kAccelerationField:
                    accelerationField_.Apply(*particleIterator, kDeltatime_);
                    break;
                }
                particleIterator->currentTime += kDeltatime_; // 経過時間を足す
                particleIterator->transform.translate += particleIterator->velocity * kDeltatime_;  // 速度を反映させる
            }

            float alpha = 1.0f - (particleIterator->currentTime / particleIterator->lifeTime);
            Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(particleIterator->transform.scale);
            Matrix4x4 translateMatrix = Math::MakeTranslateMatrix(particleIterator->transform.translate);
            Matrix4x4 worldMatrix = Math::MakeIdentity4x4();
            if (useBillbord_) {
                worldMatrix = Math::Multiply(Math::Multiply(scaleMatrix, billbordMatrix_), translateMatrix);
            } else {
                worldMatrix = Math::MakeAffineMatrix(particleIterator->transform.scale, particleIterator->transform.rotate, particleIterator->transform.translate);
            }
            Matrix4x4 worldViewProjectionMatrix = Math::Multiply(worldMatrix, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
            instancingData_[numInstance_].world = worldMatrix;
            instancingData_[numInstance_].WVP = worldViewProjectionMatrix;
            instancingData_[numInstance_].color = particleIterator->color;
            instancingData_[numInstance_].color.w = alpha;

            numInstance_++; // 生きているParticleの数を1つカウントする

        }

        ++particleIterator; // 次のイテレーターに進める
    }

    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

}

void ParticleClass::SetAccelerationField(const Vector3& center, const Vector3& size, const Vector3& acceleration) {
    Vector3 halfSize = { size.x / 2.0f, size.y / 2.0f, size.z / 2.0f };
    accelerationField_.area.min = { center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z };
    accelerationField_.area.max = { center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z };
    accelerationField_.acceleration = acceleration;
}

void ParticleClass::SetEmitterPosition(const Vector3& position) {
    emitter_.transform.translate = position;
}

void ParticleClass::SetEmitterArea(const Vector3& area) {
    emitter_.area = area;
}

void ParticleClass::SetEmitterVelocity(const Vector3& minVel, const Vector3& maxVel) {
    emitter_.velocityMin = minVel;
    emitter_.velocityMax = maxVel;
}

void ParticleClass::SetEmitterFrequency(float frequency) {
    emitter_.frequency = frequency;
}

void ParticleClass::SetEmitterCount(uint32_t count) {
    emitter_.count = count;
}

void ParticleClass::SetEmitterProperties(
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

void ParticleClass::SetTexture(const std::string& textureFilePath) {
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

Particle ParticleClass::MakeNewParticle(std::mt19937& randomEngine, const Emitter& emitter) {
    std::uniform_real_distribution<float> distRange(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
    std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
    std::uniform_real_distribution<float> distVelocityX(emitter.velocityMin.x, emitter.velocityMax.x);
    std::uniform_real_distribution<float> distVelocityY(emitter.velocityMin.y, emitter.velocityMax.y);
    std::uniform_real_distribution<float> distVelocityZ(emitter.velocityMin.z, emitter.velocityMax.z);

    Particle particle;
    particle.transform.scale = { 1.0f,1.0f,1.0f };
    particle.transform.rotate = { 0.0f,0.0f,0.0f };
    Vector3 randomTranslate = {
        distRange(randomEngine) * emitter.area.x / 2.0f,
        distRange(randomEngine) * emitter.area.y / 2.0f,
        distRange(randomEngine) * emitter.area.z / 2.0f
    };
    particle.transform.translate = emitter.transform.translate + randomTranslate;
    particle.velocity = { distVelocityX(randomEngine), distVelocityY(randomEngine), distVelocityZ(randomEngine) };
    particle.color = { distColor(randomEngine),distColor(randomEngine),distColor(randomEngine) ,1.0f };
    particle.lifeTime = distTime(randomEngine);
    particle.currentTime = 0.0f;

    return particle;
}

std::list<Particle> ParticleClass::Emit(const Emitter& emitter, std::mt19937& randomEngine) {
    std::list<Particle> particles;
    for (uint32_t count = 0; count < emitter.count; ++count) {
        particles.push_back(MakeNewParticle(randomEngine, emitter));
    }
    return particles;
}

