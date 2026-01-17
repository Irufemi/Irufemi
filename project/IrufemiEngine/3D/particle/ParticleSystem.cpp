#define NOMINMAX
#include "ParticleSystem.h"
#include "Math.h"
#include "function/Math.h"
#include "manager/DebugUI.h"
#include "engine/directX/DirectXCommon.h"
#include "engine/directX/DescriptorPool.h"
#include "engine/IrufemiEngine.h"
#include "manager/DrawManager.h"
#include <algorithm>
#include <numbers>

DescriptorPool* ParticleSystem::s_srvPool_ = nullptr;
TextureManager* ParticleSystem::s_textureManager_ = nullptr;
DrawManager* ParticleSystem::s_drawManager_ = nullptr;
IrufemiEngine* ParticleSystem::s_engine_ = nullptr;
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

void ParticleSystem::Initialize(Camera* camera, const std::string& textureName, ParticleType type, ParticlePrimitiveShape shape) {
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

    // SRV スロット確保(初回のみ)
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
    case ParticlePrimitiveShape::Plane:
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
    case ParticlePrimitiveShape::Sphere:
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
    case ParticlePrimitiveShape::Ring:
    {
        // パラメータ化したリング生成
        const uint32_t divisions = ringSegmentCount_;
        const float startRad = ringStartAngleDeg_ * (std::numbers::pi_v<float> / 180.0f);
        float endRad = ringEndAngleDeg_ * (std::numbers::pi_v<float> / 180.0f);
        // end が start 以下なら一周分を付加する(負方向の弧も扱いたい場合は要調整)
        if (endRad <= startRad) endRad += 2.0f * std::numbers::pi_v<float>;
        const float arc = endRad - startRad;
        const float radianPerDivide = arc / static_cast<float>(divisions);

        for (uint32_t i = 0; i < divisions; ++i) {
            float a0 = startRad + static_cast<float>(i) * radianPerDivide;
            float a1 = startRad + static_cast<float>(i + 1) * radianPerDivide;

            float s0 = std::sin(a0);
            float c0 = std::cos(a0);
            float s1 = std::sin(a1);
            float c1 = std::cos(a1);

            float u = static_cast<float>(i) / static_cast<float>(divisions);
            float uNext = static_cast<float>(i + 1) / static_cast<float>(divisions);

            VertexData v0, v1, v2, v3;
            // XY平面上に作成(外周→内周の順)
            v0.position = { c0 * ringOuterRadius_, s0 * ringOuterRadius_, 0.0f, 1.0f };
            v1.position = { c1 * ringOuterRadius_, s1 * ringOuterRadius_, 0.0f, 1.0f };
            v2.position = { c0 * ringInnerRadius_, s0 * ringInnerRadius_, 0.0f, 1.0f };
            v3.position = { c1 * ringInnerRadius_, s1 * ringInnerRadius_, 0.0f, 1.0f };

            // UV の縦／横切替
            if (ringVerticalUV_) {
                v0.texcoord = { 0.0f, u };
                v1.texcoord = { 0.0f, uNext };
                v2.texcoord = { 1.0f, u };
                v3.texcoord = { 1.0f, uNext };
            } else {
                v0.texcoord = { u, 0.0f };
                v1.texcoord = { uNext, 0.0f };
                v2.texcoord = { u, 1.0f };
                v3.texcoord = { uNext, 1.0f };
            }

            // 法線はZ-
            v0.normal = v1.normal = v2.normal = v3.normal = { 0.0f, 0.0f, -1.0f };

            // 基点インデックス(既に頂点が入っている可能性があるため現在サイズを基準にする)
            uint32_t baseIndex = static_cast<uint32_t>(resource_->vertexDataList_.size());
            resource_->vertexDataList_.push_back(v0);
            resource_->vertexDataList_.push_back(v1);
            resource_->vertexDataList_.push_back(v2);
            resource_->vertexDataList_.push_back(v3);

            resource_->indexDataList_.push_back(baseIndex + 0);
            resource_->indexDataList_.push_back(baseIndex + 2);
            resource_->indexDataList_.push_back(baseIndex + 1);

            resource_->indexDataList_.push_back(baseIndex + 1);
            resource_->indexDataList_.push_back(baseIndex + 2);
            resource_->indexDataList_.push_back(baseIndex + 3);
        }
    }
    break;
    case ParticlePrimitiveShape::Cylinder:
    {
        const uint32_t kCylinderDivide = cylinderSegmentCount_;
        const float kRadius = cylinderRadius_;
        const float kHeight = cylinderHeight_;
        const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

        for (uint32_t i = 0; i < kCylinderDivide; ++i) {
            float rad = static_cast<float>(i) * radianPerDivide;
            float radNext = static_cast<float>(i + 1) * radianPerDivide;

            float sin = std::sin(rad);
            float cos = std::cos(rad);
            float sinNext = std::sin(radNext);
            float cosNext = std::cos(radNext);

            float u = static_cast<float>(i) / float(kCylinderDivide);
            float uNext = static_cast<float>(i + 1) / float(kCylinderDivide);

            float v0 = cylinderFlipV_ ? 1.0f : 0.0f;
            float v1 = cylinderFlipV_ ? 0.0f : 1.0f;

            VertexData vBottom, vTop, vBottomNext, vTopNext;

            // 頂点データ
            vBottom.position = { cos * kRadius, -kHeight / 2.0f, sin * kRadius, 1.0f };
            vBottom.texcoord = { u, v0 };
            vBottom.normal = { cos, 0.0f, sin };

            vTop.position = { cos * kRadius, kHeight / 2.0f, sin * kRadius, 1.0f };
            vTop.texcoord = { u, v1 };
            vTop.normal = { cos, 0.0f, sin };

            vBottomNext.position = { cosNext * kRadius, -kHeight / 2.0f, sinNext * kRadius, 1.0f };
            vBottomNext.texcoord = { uNext, v0 };
            vBottomNext.normal = { cosNext, 0.0f, sinNext };

            vTopNext.position = { cosNext * kRadius, kHeight / 2.0f, sinNext * kRadius, 1.0f };
            vTopNext.texcoord = { uNext, v1 };
            vTopNext.normal = { cosNext, 0.0f, sinNext };

            uint32_t baseIndex = static_cast<uint32_t>(resource_->vertexDataList_.size());
            resource_->vertexDataList_.push_back(vBottom);
            resource_->vertexDataList_.push_back(vTop);
            resource_->vertexDataList_.push_back(vBottomNext);
            resource_->vertexDataList_.push_back(vTopNext);

            resource_->indexDataList_.push_back(baseIndex);
            resource_->indexDataList_.push_back(baseIndex + 1);
            resource_->indexDataList_.push_back(baseIndex + 2);

            resource_->indexDataList_.push_back(baseIndex + 1);
            resource_->indexDataList_.push_back(baseIndex + 3);
            resource_->indexDataList_.push_back(baseIndex + 2);
        }
    }
    break;
    case ParticlePrimitiveShape::Cube:
    {
        // 単位立方体(中心原点、辺長 = 1.0f)
        const float h = 0.5f;

        struct FaceDef { Vector3 n; std::array<Vector3,4> pos; std::array<Vector2,4> uv; };
        std::array<FaceDef,6> faces = {
            // +X (右面)
            FaceDef{ {1.0f,0.0f,0.0f},
                { Vector3{h,h,h}, Vector3{h,h,-h}, Vector3{h,-h,-h}, Vector3{h,-h,h} },
                { Vector2{1.0f,0.0f}, Vector2{0.0f,0.0f}, Vector2{0.0f,1.0f}, Vector2{1.0f,1.0f} } },
            // -X (左面)
            FaceDef{ {-1.0f,0.0f,0.0f},
                { Vector3{-h,h,-h}, Vector3{-h,h,h}, Vector3{-h,-h,h}, Vector3{-h,-h,-h} },
                { Vector2{0.0f,0.0f}, Vector2{1.0f,0.0f}, Vector2{1.0f,1.0f}, Vector2{0.0f,1.0f} } },
            // +Y (上面)
            FaceDef{ {0.0f,1.0f,0.0f},
                { Vector3{-h,h,h}, Vector3{h,h,h}, Vector3{h,h,-h}, Vector3{-h,h,-h} },
                { Vector2{0.0f,0.0f}, Vector2{1.0f,0.0f}, Vector2{1.0f,1.0f}, Vector2{0.0f,1.0f} } },
            // -Y (下面)
            FaceDef{ {0.0f,-1.0f,0.0f},
                { Vector3{-h,-h,-h}, Vector3{h,-h,-h}, Vector3{h,-h,h}, Vector3{-h,-h,h} },
                { Vector2{0.0f,0.0f}, Vector2{1.0f,0.0f}, Vector2{1.0f,1.0f}, Vector2{0.0f,1.0f} } },
            // +Z (前面)
            FaceDef{ {0.0f,0.0f,1.0f},
                { Vector3{h,h,h}, Vector3{-h,h,h}, Vector3{-h,-h,h}, Vector3{h,-h,h} },
                { Vector2{0.0f,0.0f}, Vector2{1.0f,0.0f}, Vector2{1.0f,1.0f}, Vector2{0.0f,1.0f} } },
            // -Z (後面)
            FaceDef{ {0.0f,0.0f,-1.0f},
                { Vector3{-h,h,-h}, Vector3{h,h,-h}, Vector3{h,-h,-h}, Vector3{-h,-h,-h} },
                { Vector2{0.0f,0.0f}, Vector2{1.0f,0.0f}, Vector2{1.0f,1.0f}, Vector2{0.0f,1.0f} } }
        };

        for (const auto& f : faces) {
            // 頂点作成(面ごとに4頂点を追加)
            VertexData v0{}, v1{}, v2{}, v3{};
            v0.position = { f.pos[0].x, f.pos[0].y, f.pos[0].z, 1.0f }; v0.texcoord = f.uv[0];
            v1.position = { f.pos[1].x, f.pos[1].y, f.pos[1].z, 1.0f }; v1.texcoord = f.uv[1];
            v2.position = { f.pos[2].x, f.pos[2].y, f.pos[2].z, 1.0f }; v2.texcoord = f.uv[2];
            v3.position = { f.pos[3].x, f.pos[3].y, f.pos[3].z, 1.0f }; v3.texcoord = f.uv[3];

            // 面法線で頂点法線を統一
            v0.normal = v1.normal = v2.normal = v3.normal = f.n;

            uint32_t baseIndex = static_cast<uint32_t>(resource_->vertexDataList_.size());
            resource_->vertexDataList_.push_back(v0);
            resource_->vertexDataList_.push_back(v1);
            resource_->vertexDataList_.push_back(v2);
            resource_->vertexDataList_.push_back(v3);

            // 仮三角(0,1,2) の法線を計算して面法線と向きが合うかチェック
            Vector3 p0{ resource_->vertexDataList_[baseIndex + 0].position.x, resource_->vertexDataList_[baseIndex + 0].position.y, resource_->vertexDataList_[baseIndex + 0].position.z };
            Vector3 p1{ resource_->vertexDataList_[baseIndex + 1].position.x, resource_->vertexDataList_[baseIndex + 1].position.y, resource_->vertexDataList_[baseIndex + 1].position.z };
            Vector3 p2{ resource_->vertexDataList_[baseIndex + 2].position.x, resource_->vertexDataList_[baseIndex + 2].position.y, resource_->vertexDataList_[baseIndex + 2].position.z };

            Vector3 e0 = Math::Subtract(p1, p0);
            Vector3 e1 = Math::Subtract(p2, p0);
            Vector3 triNormal = Math::Normalize(Math::Cross(e0, e1));

            float dot = Math::Dot(triNormal, f.n);

            if (dot >= 0.0f) {
                // 三角形法線が面法線と同じ向き → この順で追加(外向き)
                resource_->indexDataList_.push_back(baseIndex + 0);
                resource_->indexDataList_.push_back(baseIndex + 1);
                resource_->indexDataList_.push_back(baseIndex + 2);

                resource_->indexDataList_.push_back(baseIndex + 0);
                resource_->indexDataList_.push_back(baseIndex + 2);
                resource_->indexDataList_.push_back(baseIndex + 3);
            } else {
                // 向きが逆ならワインディングを反転して追加
                resource_->indexDataList_.push_back(baseIndex + 2);
                resource_->indexDataList_.push_back(baseIndex + 1);
                resource_->indexDataList_.push_back(baseIndex + 0);

                resource_->indexDataList_.push_back(baseIndex + 3);
                resource_->indexDataList_.push_back(baseIndex + 2);
                resource_->indexDataList_.push_back(baseIndex + 0);
            }
        }
    }
    break;
    case ParticlePrimitiveShape::Tetrahedron:
    {
        // 正四面体(辺長 = s)を生成。底面を水平(XZ平面)に配置する。
        const float s = 0.5f; // 全体スケール(既存のスケールと整合)
        const float R = 1.0f / std::sqrt(3.0f);               // 辺長1の正三角形の外接円半径
        const float baseToApex = std::sqrt(2.0f / 3.0f);      // (a + b) の長さ(辺長=1 のとき)
        const float b = baseToApex / 4.0f;                   // 基底面の y = -b(重心を原点に揃えるための設定)
        const float a = 3.0f * b;                            // 頂点の y = +a(重心が原点になるよう a = 3b)
        // スケール適用済みの頂点
        Vector3 apex = { 0.0f,  a * s, 0.0f };                  // 頂点(上)
        Vector3 v0 = { 0.0f, -b * s,  R * s };               // 底面頂点 0
        Vector3 v1 = { -0.5f * s, -b * s, -R * 0.5f * s };   // 底面頂点 1
        Vector3 v2 = { 0.5f * s, -b * s, -R * 0.5f * s };   // 底面頂点 2

        // 面の定義(各面は三角形)
        std::vector<std::tuple<Vector3, Vector3, Vector3>> faces = {
            { v0, v1, v2 },        // 底面(XZ平面上)
            { apex, v0, v1 },
            { apex, v1, v2 },
            { apex, v2, v0 }
        };

        // UV は単純に三角形マッピング
        Vector2 uv0 = { 0.5f, 0.0f };
        Vector2 uv1 = { 0.0f, 1.0f };
        Vector2 uv2 = { 1.0f, 1.0f };

        for (const auto& face : faces) {
            uint32_t baseIndex = static_cast<uint32_t>(resource_->vertexDataList_.size());

            Vector3 p0 = std::get<0>(face);
            Vector3 p1 = std::get<1>(face);
            Vector3 p2 = std::get<2>(face);

            // 面法線(右手系クロス)を計算
            Vector3 e0 = Math::Subtract(p1, p0);
            Vector3 e1 = Math::Subtract(p2, p0);
            Vector3 triNormal = Math::Normalize(Math::Cross(e0, e1));

            // 頂点データ作成(面ごとに法線を統一して追加)
            VertexData vd0{}, vd1{}, vd2{};
            vd0.position = { p0.x, p0.y, p0.z, 1.0f };
            vd1.position = { p1.x, p1.y, p1.z, 1.0f };
            vd2.position = { p2.x, p2.y, p2.z, 1.0f };

            vd0.normal = vd1.normal = vd2.normal = triNormal;

            vd0.texcoord = uv0;
            vd1.texcoord = uv1;
            vd2.texcoord = uv2;

            resource_->vertexDataList_.push_back(vd0);
            resource_->vertexDataList_.push_back(vd1);
            resource_->vertexDataList_.push_back(vd2);

            // 三角形の重心を計算して法線が外向きになるようワインディングを決定
            Vector3 centroid{
                (p0.x + p1.x + p2.x) / 3.0f,
                (p0.y + p1.y + p2.y) / 3.0f,
                (p0.z + p1.z + p2.z) / 3.0f
            };
            float dot = Math::Dot(triNormal, centroid);
            if (dot >= 0.0f) {
                // 法線が外向き(重心方向と同じ向き)ならそのまま追加
                resource_->indexDataList_.push_back(baseIndex + 0);
                resource_->indexDataList_.push_back(baseIndex + 1);
                resource_->indexDataList_.push_back(baseIndex + 2);
            } else {
                // 内向きならワインディングを反転
                resource_->indexDataList_.push_back(baseIndex + 2);
                resource_->indexDataList_.push_back(baseIndex + 1);
                resource_->indexDataList_.push_back(baseIndex + 0);
            }
        }
    }
    break;
    }

    // リソースのメモリを確保(または再利用)
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
    resource_->materialData_->useClampSampler = (primitiveShape_ == ParticlePrimitiveShape::Ring || primitiveShape_ == ParticlePrimitiveShape::Cylinder);

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
                Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(particleIterator->transform.rotate.x, particleIterator->transform.rotate.y, particleIterator->transform.rotate.z);
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

#if USE_IMGUI
    for (auto& line : debugLines_) {
        line->Update();
    }
#endif
}

void ParticleSystem::Draw()
{
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
            s_drawManager_->DrawParticle(this);
        }

        // エンジン状態を復元(PSOの切り替えは呼び出し側で制御するため Apply は行わない)
        s_engine_->SetBlend(prevBlend);
        s_engine_->SetDepthWrite(prevDepth);
        s_engine_->SetCull(prevCull);
    } else {
        // エンジン参照がない場合は従来通り(安全策)
        if (s_drawManager_) {
            s_drawManager_->DrawParticle(this);
        }
    }

    // 2) デバッグ線(AABB 等)を描画(Line PSO を確実にバインド)
#if USE_IMGUI
    if (!debugLines_.empty()) {
        if (s_engine_) {
            s_engine_->ApplyLinePSO();
        }
        for (auto& line : debugLines_) {
            if (line && s_drawManager_) s_drawManager_->DrawLine3D(line.get());
        }
    }
#endif
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

void ParticleSystem::PlayHitEffect(const Vector3& position, uint32_t count) {
	if (particleType_ == ParticleType::kHitEffect) {
		Emitter customEmitter = emitter_;
		customEmitter.transform.translate = position;
		customEmitter.count = count;
		particles_.splice(particles_.end(), Emit(customEmitter, randomEngine_));
	}
}

void ParticleSystem::Debug([[maybe_unused]] const char* particleName) {

#if USE_IMGUI
    debugLines_.clear();

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
                ImGui::Checkbox("useBillbord", &useBillbord_);

                ImGui::Separator();

                // PrimitiveShapeの選択UI
                const char* primitiveShapeNames[] = { "Plane", "Sphere", "Ring", "Cylinder", "Cube", "Tetrahedron" };
                int currentShape = static_cast<int>(primitiveShape_);
                if (ImGui::Combo("Primitive Shape", &currentShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                    if (primitiveShape_ != static_cast<ParticlePrimitiveShape>(currentShape)) {
                        primitiveShape_ = static_cast<ParticlePrimitiveShape>(currentShape);
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

            // レンダリングタブ
            if (ImGui::BeginTabItem("Rendering")) {
                s_ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
                s_ui_->DebugMaterialParticle(resource_->materialData_);
                s_ui_->DebugUvTransform(resource_->uvTransform_);

                // --- Ring パラメータ UI ---
                if (primitiveShape_ == ParticlePrimitiveShape::Ring) {
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
                        if (s_textureManager_) {
                            auto textureNames = s_textureManager_->GetTextureNames();
                            std::sort(textureNames.begin(), textureNames.end());
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
                if (primitiveShape_ == ParticlePrimitiveShape::Cylinder) {
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
                        if (s_textureManager_) {
                            auto textureNames = s_textureManager_->GetTextureNames();
                            std::sort(textureNames.begin(), textureNames.end());
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
        useBillbord_ = false;
    } else {
        useBillbord_ = true;
    }
}

void ParticleSystem::DrawAABB(const AABB& aabb, const Vector4& color)
{
#if USE_IMGUI
    Vector3 vertices[8];
    vertices[0] = { aabb.min.x, aabb.min.y, aabb.min.z };
    vertices[1] = { aabb.max.x, aabb.min.y, aabb.min.z };
    vertices[2] = { aabb.min.x, aabb.max.y, aabb.min.z };
    vertices[3] = { aabb.max.x, aabb.max.y, aabb.min.z };
    vertices[4] = { aabb.min.x, aabb.min.y, aabb.max.z };
    vertices[5] = { aabb.max.x, aabb.min.y, aabb.max.z };
    vertices[6] = { aabb.min.x, aabb.max.y, aabb.max.z };
    vertices[7] = { aabb.max.x, aabb.max.y, aabb.max.z };

    uint32_t indices[] = {
        0, 1, 1, 3, 3, 2, 2, 0, // Bottom face
        4, 5, 5, 7, 7, 6, 6, 4, // Top face
        0, 4, 1, 5, 2, 6, 3, 7  // Connecting edges
    };

    for (int i = 0; i < 12; ++i) {
        auto line = std::make_unique<Line3DClass>();
        line->Initialize(camera_, vertices[indices[i * 2]], vertices[indices[i * 2 + 1]], color);
        debugLines_.push_back(std::move(line));
    }
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