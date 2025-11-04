#define NOMINMAX
#include "SpriteRegion.h"
#include "function/Math.h"
#include "engine/directX/DirectXCommon.h"
#include "manager/DrawManager.h"
#include "engine/DescriptorAllocator.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <cassert>

static inline float Smooth01(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
static inline Vector2 LerpVec2(const Vector2& a, const Vector2& b, float t) {
    return Vector2{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}
static inline Vector4 LerpVec4(const Vector4& a, const Vector4& b, float t) {
    return Vector4{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
}

// 静的メンバ
DirectXCommon* SpriteRegion::dx_ = nullptr;
DrawManager* SpriteRegion::drawManager_ = nullptr;
DescriptorAllocator* SpriteRegion::s_srvAllocator_ = nullptr;

void SpriteRegion::Initialize(Camera* camera, const std::string& textureName) {
    camera_ = camera;
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(camera_, textureName);

    // 中央アンカー推奨（サイズ中心で拡大縮小する）
    sprite_->SetAnchor(0.5f, 0.5f);
    sprite_->SetFlip(false, false);

    // 予備確保
    particles_.reserve(std::min<size_t>(maxParticles_, size_t(2048)));

    // 乱数初期化
    randomEngine_.seed(rndDevice_());
}

void SpriteRegion::AddInstance(const Transform& t) {
    if (!sprite_) return;

    for (auto& p : particles_) {
        if (!p.active && p.delay <= 0.0f && !p.emitShardsOnDeath) {
            p.transform = t;
            p.startScale = Vector2{ t.scale.x, t.scale.y };
            p.endScale = p.startScale;
            p.startColor = sprite_->GetColor();
            p.endColor = p.startColor;
            p.color = p.startColor;
            p.life = 1.0f;
            p.age = 0.0f;
            p.delay = 0.0f;
            p.active = true;
            return;
        }
    }
    if (particles_.size() < maxParticles_) {
        Particle newP;
        newP.transform = t;
        newP.startScale = Vector2{ t.scale.x, t.scale.y };
        newP.endScale = newP.startScale;
        newP.startColor = sprite_->GetColor();
        newP.endColor = newP.startColor;
        newP.color = newP.startColor;
        newP.life = 1.0f;
        newP.age = 0.0f;
        newP.delay = 0.0f;
        newP.active = true;
        particles_.push_back(std::move(newP));
    }
}

void SpriteRegion::ClearInstances() {
    particles_.clear();
    activeCount_ = 0;
}

UINT SpriteRegion::GetIndexCount() const {
    auto* res = GetSpriteResource();
    return res ? static_cast<UINT>(res->indexDataList_.size()) : 0;
}

void SpriteRegion::SpawnShardsAt(const Vector3& center, int count, float sizePx, float lifetime) {
    if (!sprite_) return;
    std::uniform_real_distribution<float> angDist(0.0f, 2.0f * 3.14159265358979323846f);
    std::uniform_real_distribution<float> spdDist(sizePx * 20.0f, sizePx * 120.0f);
    std::uniform_real_distribution<float> rotDist(-8.0f, 8.0f);

    for (int i = 0; i < count; ++i) {
        Particle* slot = nullptr;
        for (auto& p : particles_) {
            if (!p.active && !p.emitShardsOnDeath) { slot = &p; break; }
        }
        if (!slot) {
            if (particles_.size() < maxParticles_) {
                particles_.emplace_back();
                slot = &particles_.back();
            } else {
                continue;
            }
        }

        float angle = angDist(randomEngine_);
        float speed = spdDist(randomEngine_);
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;

        slot->transform.translate = center;
        slot->transform.scale = Vector3{ sizePx, sizePx, 1.0f };
        slot->startScale = Vector2{ sizePx, sizePx };
        slot->endScale = Vector2{ sizePx * 0.3f, sizePx * 0.3f };
        slot->startColor = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
        slot->endColor = Vector4{ 1.0f, 1.0f, 1.0f, 0.0f };
        slot->velocity = Vector2{ vx, vy };
        slot->rotationSpeed = rotDist(randomEngine_);
        slot->life = lifetime;
        slot->age = 0.0f;
        slot->delay = 0.0f;
        slot->active = true;
        slot->emitShardsOnDeath = false;
        slot->shardsSpawned = false;
    }
}

void SpriteRegion::EmitSquareWave(
    const Vector2& center,
    int cols, int rows,
    float cellSizePx,
    float spacingPx,
    float brightDuration,
    int shardsPerCell,
    float shardSizePx,
    float shardLifetime,
    float waveSpeedPxPerSec
) {
    if (!sprite_) return;
    if (cols <= 0 || rows <= 0) return;

    float totalW = cols * cellSizePx + (cols - 1) * spacingPx;
    float totalH = rows * cellSizePx + (rows - 1) * spacingPx;
    float startX = center.x - totalW * 0.5f + cellSizePx * 0.5f;
    float startY = center.y - totalH * 0.5f + cellSizePx * 0.5f;

    for (int j = 0; j < rows; ++j) {
        for (int i = 0; i < cols; ++i) {
            Vector2 pos{
                startX + i * (cellSizePx + spacingPx),
                startY + j * (cellSizePx + spacingPx)
            };
            float dxv = pos.x - center.x;
            float dyv = pos.y - center.y;
            float dist = std::sqrt(dxv * dxv + dyv * dyv);
            float delay = dist / std::max(1.0f, waveSpeedPxPerSec);

            Particle* slot = nullptr;
            for (auto& p : particles_) {
                if (!p.active && !p.emitShardsOnDeath) { slot = &p; break; }
            }
            if (!slot) {
                if (particles_.size() < maxParticles_) {
                    particles_.emplace_back();
                    slot = &particles_.back();
                } else {
                    continue;
                }
            }

            slot->transform.translate = Vector3{ pos.x, pos.y, 0.0f };
            slot->transform.scale = Vector3{ 0.0f, 0.0f, 1.0f };
            slot->startScale = Vector2{ 0.0f, 0.0f };
            slot->endScale = Vector2{ cellSizePx * 1.05f, cellSizePx * 1.05f };
            slot->startColor = Vector4{ 1.0f, 1.0f, 1.0f, 0.0f };
            slot->endColor = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
            slot->velocity = Vector2{ 0.0f, 0.0f };
            slot->rotationSpeed = 0.0f;
            slot->life = brightDuration;
            slot->age = 0.0f;
            slot->delay = delay;
            slot->active = false;
            slot->emitShardsOnDeath = true;
            slot->shardsSpawned = false;
            slot->shardsCount = shardsPerCell;
            slot->shardSize = shardSizePx;
            slot->shardLifetime = shardLifetime;
        }
    }
}

void SpriteRegion::Update(float dt) {
    if (!sprite_) return;
    if (dt <= 0.0f) return;

    for (auto& p : particles_) {
        if (!p.active) {
            if (p.delay > 0.0f) {
                p.delay -= dt;
                if (p.delay <= 0.0f) {
                    p.active = true;
                    p.age = 0.0f;
                    p.transform.scale = Vector3{ p.startScale.x, p.startScale.y, 1.0f };
                    p.color = p.startColor;
                } else {
                    continue;
                }
            } else {
                if (!p.active) continue;
            }
        }

        p.age += dt;
        float t = (p.life > 0.0f) ? std::clamp(p.age / p.life, 0.0f, 1.0f) : 1.0f;
        float e = Smooth01(t);

        Vector2 curScale = LerpVec2(p.startScale, p.endScale, e);
        p.transform.scale.x = curScale.x;
        p.transform.scale.y = curScale.y;

        p.color = LerpVec4(p.startColor, p.endColor, e);

        p.transform.translate.x += p.velocity.x * dt;
        p.transform.translate.y += p.velocity.y * dt;
        p.transform.rotate.z += p.rotationSpeed * dt;

        if (p.age >= p.life) {
            if (p.emitShardsOnDeath && !p.shardsSpawned) {
                SpawnShardsAt(p.transform.translate, p.shardsCount, p.shardSize, p.shardLifetime);
                p.shardsSpawned = true;
            }
            p.active = false;
        }
    }
}

void SpriteRegion::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    if (!dx_) return;
    if (instanceCount == 0) return;

    if (instanceCount > instanceCapacity_ || !instanceBuffer_) {
        instanceCapacity_ = std::max<uint32_t>(instanceCount, std::max<uint32_t>(256, instanceCapacity_ * 2));
        const size_t byteSize = sizeof(InstanceData) * instanceCapacity_;

        instanceBuffer_.Reset();
        instanceBuffer_ = dx_->CreateBufferResource(byteSize);
        instanceData_ = nullptr;
        instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));
        assert(instanceData_ && "Instance buffer map failed");

        // SRV は一度作れば再利用（Descriptor は固定スロット）
        EnsureInstancingSRV();
        // SRV のストライド/NumElements は最大容量で定義する
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = instanceCapacity_;
        srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        dx_->GetDevice()->CreateShaderResourceView(instanceBuffer_.Get(), &srvDesc, instancingSrvCPU_);
    }
}

void SpriteRegion::EnsureInstancingSRV() {
    if (!s_srvAllocator_) return;
    if (instancingSrvIndex_ != UINT32_MAX) return;

    instancingSrvIndex_ = s_srvAllocator_->Allocate();
    instancingSrvCPU_ = s_srvAllocator_->GetCPUHandle(instancingSrvIndex_);
    instancingSrvGPU_ = s_srvAllocator_->GetGPUHandle(instancingSrvIndex_);
}

void SpriteRegion::BuildInstanceBuffer(bool force) {
    // 有効なインスタンス数をカウント
    activeCount_ = 0;
    for (const auto& p : particles_) if (p.active) ++activeCount_;
    if (activeCount_ == 0) return;

    // バッファ確保/リサイズ
    CreateOrResizeInstanceBuffer(static_cast<uint32_t>(activeCount_));

    // 書き込み
    Matrix4x4 ortho = camera_->GetOrthographicMatrix();
    InstanceData* dst = instanceData_;
    for (const auto& p : particles_) {
        if (!p.active) continue;

        Matrix4x4 world = Math::MakeAffineMatrix(p.transform.scale, p.transform.rotate, p.transform.translate);
        Matrix4x4 wvp = Math::Multiply(world, ortho);

        dst->WVP = wvp;
        dst->color = p.color;
        ++dst;
    }
}

void SpriteRegion::Draw(bool debugUpdate, bool useCpuFallback) {
    if (!sprite_) return;

    if (useCpuFallback || !dx_ || !drawManager_ || !s_srvAllocator_) {
        // CPUフォールバック（既存の逐次描画）
        for (auto& p : particles_) {
            if (!p.active) continue;

            sprite_->SetSize(p.transform.scale.x, p.transform.scale.y);
            sprite_->SetRotation(p.transform.rotate.z);
            sprite_->SetPosition(p.transform.translate.x, p.transform.translate.y, p.transform.translate.z);
            sprite_->SetColor(p.color);
            sprite_->Update(debugUpdate);
            sprite_->Draw();
        }
        return;
    }

    // GPUインスタンシング
    BuildInstanceBuffer(false);
    if (activeCount_ == 0) return;
    // ここでは PSO の切り替えは行いません。プロジェクト側で Sprite-Instanced 用 PSO をバインドしてください。
    drawManager_->DrawSpriteRegion(this);
}