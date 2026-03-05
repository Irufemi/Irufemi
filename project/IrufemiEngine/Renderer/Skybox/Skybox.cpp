#include "Skybox.h"

#include "Application/camera/Camera.h"
#include "engine/IrufemiEngine.h"
#include "engine/directX/DirectXCommon.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "function/Math.h"

IrufemiEngine* Skybox::engine_ = nullptr;


// コンストラクタ
Skybox::Skybox() {}
// デストラクタ
Skybox::~Skybox() {
    UnMapResource();
}

void Skybox::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // 箱を作る
    // 次にSkyboxを表現する箱の頂点を作成する。CG2で球を作成したが、今回は箱である。
    // 原点を中心として、幅2m、高さ2mの箱を作る(x, y, zそれぞれ、-1~1ということ)
    // 以下の点に注意すること
    // 内側から箱を見るので、カリングの向きが逆
    // Skyboxではtexcoordもnormalも使わないので適当に埋めておけば良い
    // 可能なら専用の頂点構造を作ると良い

    // 右面 (+X)
    vertexDataList_.push_back({ { 1.0f, 1.0f, 1.0f, 1.0f }, {}, {} });   // 0
    vertexDataList_.push_back({ { 1.0f, 1.0f, -1.0f, 1.0f }, {}, {} });  // 1
    vertexDataList_.push_back({ { 1.0f, -1.0f, 1.0f, 1.0f }, {}, {} });  // 2
    vertexDataList_.push_back({ { 1.0f, -1.0f, -1.0f, 1.0f }, {}, {} }); // 3
    // 左面 (-X)
    vertexDataList_.push_back({ { -1.0f, 1.0f, -1.0f, 1.0f }, {}, {} }); // 4
    vertexDataList_.push_back({ { -1.0f, 1.0f, 1.0f, 1.0f }, {}, {} });  // 5
    vertexDataList_.push_back({ { -1.0f, -1.0f, -1.0f, 1.0f }, {}, {} });// 6
    vertexDataList_.push_back({ { -1.0f, -1.0f, 1.0f, 1.0f }, {}, {} }); // 7
    // 前面 (+Z)
    vertexDataList_.push_back({ { -1.0f, 1.0f, 1.0f, 1.0f }, {}, {} });  // 8
    vertexDataList_.push_back({ { 1.0f, 1.0f, 1.0f, 1.0f }, {}, {} });   // 9
    vertexDataList_.push_back({ { -1.0f, -1.0f, 1.0f, 1.0f }, {}, {} }); // 10
    vertexDataList_.push_back({ { 1.0f, -1.0f, 1.0f, 1.0f }, {}, {} });  // 11
    // 後面 (-Z)
    vertexDataList_.push_back({ { 1.0f, 1.0f, -1.0f, 1.0f }, {}, {} });  // 12
    vertexDataList_.push_back({ { -1.0f, 1.0f, -1.0f, 1.0f }, {}, {} }); // 13
    vertexDataList_.push_back({ { 1.0f, -1.0f, -1.0f, 1.0f }, {}, {} }); // 14
    vertexDataList_.push_back({ { -1.0f, -1.0f, -1.0f, 1.0f }, {}, {} });// 15
    // 上面 (+Y)
    vertexDataList_.push_back({ { -1.0f, 1.0f, -1.0f, 1.0f }, {}, {} }); // 16
    vertexDataList_.push_back({ { 1.0f, 1.0f, -1.0f, 1.0f }, {}, {} });  // 17
    vertexDataList_.push_back({ { -1.0f, 1.0f, 1.0f, 1.0f }, {}, {} });  // 18
    vertexDataList_.push_back({ { 1.0f, 1.0f, 1.0f, 1.0f }, {}, {} });   // 19
    // 下面 (-Y)
    vertexDataList_.push_back({ { -1.0f, -1.0f, 1.0f, 1.0f }, {}, {} }); // 20
    vertexDataList_.push_back({ { 1.0f, -1.0f, 1.0f, 1.0f }, {}, {} });  // 21
    vertexDataList_.push_back({ { -1.0f, -1.0f, -1.0f, 1.0f }, {}, {} });// 22
    vertexDataList_.push_back({ { 1.0f, -1.0f, -1.0f, 1.0f }, {}, {} });  // 23

    // 右面
    indexDataList_.push_back(0); indexDataList_.push_back(2); indexDataList_.push_back(1);
    indexDataList_.push_back(1); indexDataList_.push_back(2); indexDataList_.push_back(3);
    // 左面
    indexDataList_.push_back(4); indexDataList_.push_back(6); indexDataList_.push_back(5);
    indexDataList_.push_back(5); indexDataList_.push_back(6); indexDataList_.push_back(7);
    // 前面
    indexDataList_.push_back(8); indexDataList_.push_back(10); indexDataList_.push_back(9);
    indexDataList_.push_back(9); indexDataList_.push_back(10); indexDataList_.push_back(11);
    // 後面
    indexDataList_.push_back(12); indexDataList_.push_back(14); indexDataList_.push_back(13);
    indexDataList_.push_back(13); indexDataList_.push_back(14); indexDataList_.push_back(15);
    // 上面
    indexDataList_.push_back(16); indexDataList_.push_back(18); indexDataList_.push_back(17);
    indexDataList_.push_back(17); indexDataList_.push_back(18); indexDataList_.push_back(19);
    // 下面
    indexDataList_.push_back(20); indexDataList_.push_back(22); indexDataList_.push_back(21);
    indexDataList_.push_back(21); indexDataList_.push_back(22); indexDataList_.push_back(23);

    CreateResource();

    MapResource();

    //頂点バッファ

    vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(vertexDataList_.size());

    std::copy(vertexDataList_.begin(), vertexDataList_.end(), vertexData_);

    /*頂点インデックス*/

    ///Index用のあれやこれやを作る

    indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    //リソースの先頭のアドレスから使う
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    //使用するリソースのサイズ
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(indexDataList_.size());
    //インデックスはint32_tとする
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    ///IndexResourceにデータを書き込む

    //インデックスリソースにデータを書き込む

    std::copy(indexDataList_.begin(), indexDataList_.end(), indexData_);

    // material
    materialData_->color = { 1.0f,1.0f,1.0f,1.0f };

    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    transformationMatrix_.WVP = Math::Multiply(worldMatrix, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    *transformationData_ = transformationMatrix_;

    TextureManager* textureManager_ = engine_->GetTextureManager();

    auto textureNames = textureManager_->GetTextureNames();
    std::sort(textureNames.begin(), textureNames.end());
    if (!textureNames.empty()) {

        textureHandle_ = textureManager_->GetTextureHandle(textureName);

        // コンボボックス用に selectedIndex を初期化
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        if (it != textureNames.end()) {
            selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
        } else {
            selectedTextureIndex_ = 0;
        }

    } else {
        textureHandle_.ptr = 0;
    }


}

void Skybox::Update() {

    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    transformationMatrix_.WVP = Math::Multiply(worldMatrix, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 更新されたWVP行列をGPUリソースにコピーする
    if (transformationData_) {
        *transformationData_ = transformationMatrix_;
    }
}

void Skybox::Draw() {

    DrawManager* drawManager = engine_->GetDrawManager();

    engine_->ApplySkyboxPSO();

    drawManager->DrawSkybox(vertexBufferView_, indexBufferView_, materialResource_, transformationResource_, textureHandle_, static_cast<UINT>(indexDataList_.size()));

}

void Skybox::Debug() {
}

void Skybox::CreateResource() {

    DirectXCommon* dxCommon = engine_->GetDirectXCommon();

    char buf[256];
    if (!vertexDataList_.empty()) {
        vertexResource_ = dxCommon->CreateBufferResource(sizeof(VertexData) * static_cast<size_t>(vertexDataList_.size()));
        snprintf(buf, sizeof(buf), "Created SkyboxResource at %p in %s:%d\n", vertexResource_.Get(), __FILE__, __LINE__);
        OutputDebugStringA(buf);
    }
    if (!indexDataList_.empty()) {
        indexResource_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * static_cast<size_t>(indexDataList_.size()));
        snprintf(buf, sizeof(buf), "Created SkyboxResource at %p in %s:%d\n", indexResource_.Get(), __FILE__, __LINE__);
        OutputDebugStringA(buf);
    }
    materialResource_ = dxCommon->CreateBufferResource(sizeof(SkyboxMaterial));
    snprintf(buf, sizeof(buf), "Created SkyboxResource at %p in %s:%d\n", materialResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
    transformationResource_ = dxCommon->CreateBufferResource(sizeof(SkyboxTransformationMatrix));
    snprintf(buf, sizeof(buf), "Created SkyboxResource at %p in %s:%d\n", transformationResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
}

void Skybox::MapResource() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    if (materialResource_) {
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    }
    if (transformationResource_) {
        transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));
    }
}

void Skybox::UnMapResource() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
    }
    if (materialResource_) {
        materialResource_->Unmap(0, nullptr);
    }
    if (transformationResource_) {
        transformationResource_->Unmap(0, nullptr);
    }
}