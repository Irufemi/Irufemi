#include "Skybox.h"

#include "Engine/IrufemiEngine.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Manager/DrawManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Application/camera/Camera.h"
#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif
#include "../../Engine/Manager/PrimitiveManager.h"

IrufemiEngine* Skybox::engine_ = nullptr;


// 郢ｧ・ｳ郢晢ｽｳ郢ｧ・ｹ郢晏現ﾎ帷ｹｧ・ｯ郢ｧ・ｿ
Skybox::Skybox() {}
// 郢昴・縺帷ｹ晏現ﾎ帷ｹｧ・ｯ郢ｧ・ｿ
Skybox::~Skybox() {
    UnMapResource();
}

void Skybox::Initialize(Camera* camera, const std::string& textureName) {
    this->camera_ = camera;

    // PrimitiveManager 邵ｺ荵晢ｽ臥ｹｧ・ｹ郢ｧ・ｫ郢ｧ・､郢晄㈱繝｣郢ｧ・ｯ郢ｧ・ｹ騾包ｽｨ邵ｺ・ｮ陟厄ｽ｢霑･・ｶ繝ｻ繝ｻube繝ｻ蟲ｨ・定愾髢・ｾ繝ｻ
    PrimitiveManager* primitiveManager = PrimitiveManager::GetInstance();
    const PrimitiveData& primitiveData = primitiveManager->GetPrimitiveData(PrimitiveType::Skybox);

    vertexDataList_ = primitiveData.vertices;
    indexDataList_ = primitiveData.indices;

    CreateResource();
    MapResource();

    // 鬯・ｉ縺帷ｹ晁・繝｣郢晁ｼ斐＜邵ｺ・ｮ髫ｪ・ｭ陞ｳ繝ｻ
    vertexBufferView_ = {};
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexDataList_.size());

    std::copy(vertexDataList_.begin(), vertexDataList_.end(), vertexData_);

    // 郢ｧ・､郢晢ｽｳ郢昴・繝｣郢ｧ・ｯ郢ｧ・ｹ郢晁・繝｣郢晁ｼ斐＜邵ｺ・ｮ髫ｪ・ｭ陞ｳ繝ｻ
    indexBufferView_ = {};
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indexDataList_.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    std::copy(indexDataList_.begin(), indexDataList_.end(), indexData_);

    // 郢晁ｼ釆帷ｹｧ・ｰ隴厄ｽｴ隴・ｽｰ
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();

    TextureManager* textureManager = engine_->GetTextureManager();

    if (textureName == "whiteCubeMap") {
        textureHandle_ = textureManager->GetWhiteCubeMapHandle();
    } else {
        auto textureNames = textureManager->GetCubeMapNamesForDebug();
        if (!textureNames.empty()) {
            textureHandle_ = textureManager->GetTextureHandle(textureName);

            // 郢ｧ・ｳ郢晢ｽｳ郢晄㈱繝ｻ郢昴・縺醍ｹｧ・ｹ騾包ｽｨ邵ｺ・ｫ selectedIndex 郢ｧ雋槭・隴帶ｺｷ蝟ｧ
            auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
            if (it != textureNames.end()) {
                selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
            } else {
                selectedTextureIndex_ = 0;
            }
        } else {
            // 郢昴・縺醍ｹｧ・ｹ郢昶・ﾎ慕ｸｺ迹夲ｽｦ荵昶命邵ｺ荵晢ｽ臥ｸｺ・ｪ邵ｺ繝ｻﾂ竏壺穐邵ｺ貅倥・郢晢ｽｪ郢ｧ・ｹ郢晏現窶ｲ驕ｨ・ｺ邵ｺ・ｮ陜｣・ｴ陷ｷ蛹ｻ繝ｻ騾具ｽｽ郢ｧ・ｭ郢晢ｽ･郢晢ｽｼ郢晄じ繝ｻ郢昴・繝ｻ郢ｧ蜑・ｽｽ・ｿ騾包ｽｨ
            textureHandle_ = textureManager->GetWhiteCubeMapHandle();
            selectedTextureIndex_ = 0;
        }
    }
}

void Skybox::Update() {

    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    // 郢ｧ・ｷ郢ｧ・ｧ郢晢ｽｼ郢敖郢晢ｽｼ陋幢ｽｴ邵ｺ・ｧgCamera郢ｧ蜑・ｽｽ・ｿ騾包ｽｨ邵ｺ蜷ｶ・狗ｹｧ蛹ｻ竕ｧ邵ｺ・ｫ邵ｺ・ｪ邵ｺ・｣邵ｺ貅倪螺郢ｧ莉抃P邵ｺ・ｮ髫ｪ閧ｲ・ｮ蜉ｱ・帝ｵ竏ｫ謇・
    transformationMatrix_.WVP = Math::MakeIdentity4x4();
    transformationMatrix_.World = worldMatrix;
    // 郢晁ｼ釆帷ｹｧ・ｰ隴厄ｽｴ隴・ｽｰ
    MarkAsDirty();
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void Skybox::SyncBeforeDraw() {
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    if (isDirtyBuffer_[frameIndex]) {
        transformationBuffer_.Update(transformationMatrix_, frameIndex);
        isDirtyBuffer_[frameIndex] = false;
    }
}

void Skybox::Draw() {
    if (!vertexResource_ || !indexResource_ || !camera_ || !engine_) return;

    // 郢ｧ・ｫ郢晢ｽ｡郢晢ｽｩ邵ｺ・ｮ髯ｦ謔溘・邵ｺ謔滂ｽ､逕ｻ蟲ｩ邵ｺ霈費ｽ檎ｸｺ貅伉ｰ邵ｲ竏壹′郢晄じ縺夂ｹｧ・ｧ郢ｧ・ｯ郢晞メ繝ｻ闖ｴ阮吮ｲ陞溽判蟲ｩ邵ｺ霈費ｽ檎ｸｺ貅伉ｰ郢昶・縺臥ｹ昴・縺・
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    SyncBeforeDraw();

    DrawManager* drawManager = engine_->GetDrawManager();

    engine_->ApplySkyboxPSO();

    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    drawManager->DrawSkybox(vertexBufferView_, indexBufferView_, materialBuffer_.GetResource(frameIndex), transformationBuffer_.GetResource(frameIndex), textureHandle_, static_cast<UINT>(indexDataList_.size()));

}

void Skybox::Debug() {
#ifdef USE_IMGUI
        if (ImGui::CollapsingHeader("Skybox")) {
            TextureManager* textureManager = engine_->GetTextureManager();
            auto textureNames = textureManager->GetCubeMapNamesForDebug();

            if (!textureNames.empty()) {
            if (ImGui::Combo("Texture", &selectedTextureIndex_, [](void* data, int idx) {
                auto* names = reinterpret_cast<std::vector<std::string>*>(data);
                if (idx < 0 || idx >= static_cast<int>(names->size())) return (const char*)nullptr;
                return (*names)[idx].c_str();
            }, &textureNames, static_cast<int>(textureNames.size()))) {
                // 鬩包ｽｸ隰壽ｧｭ窶ｲ陞溽判蟲ｩ邵ｺ霈費ｽ檎ｸｺ繝ｻ
                std::string selectedName = textureNames[selectedTextureIndex_];
                if (selectedName == "whiteCubeMap") {
                    textureHandle_ = textureManager->GetWhiteCubeMapHandle();
                } else if (selectedName == "white") {
                    textureHandle_ = textureManager->GetWhiteTextureHandle();
                } else {
                    textureHandle_ = textureManager->GetTextureHandle(selectedName);
                }
            }
            uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
            if (materialBuffer_[frameIndex]) {
                ImGui::SliderFloat("Intensity", &materialBuffer_[frameIndex]->intensity, 0.0f, 10.0f);
            }
        }
    }
#endif
}

void Skybox::CreateResource() {

    DirectXCommon* dxCommon = engine_->GetDrawManager()->GetDxCommon();

    // 鬯・ｉ縺帷ｹ晢ｽｻ郢ｧ・､郢晢ｽｳ郢昴・繝｣郢ｧ・ｯ郢ｧ・ｹ邵ｺ・ｮ鬮ｱ蜥丞飭郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ邵ｺ・ｯ陷雁・ｽｸﾂ郢晁・繝｣郢晁ｼ斐＜邵ｺ・ｮ邵ｺ・ｾ邵ｺ・ｾ
    if (!vertexResource_) {
        vertexResource_ = dxCommon->CreateBufferResource(sizeof(VertexData) * static_cast<size_t>(vertexDataList_.size()));
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(vertexDataList_.size());
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
    }
    if (!indexResource_) {
        indexResource_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * static_cast<size_t>(indexDataList_.size()));
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(indexDataList_.size());
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    }

    // 郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ邵ｺ・ｨ郢晢ｽｯ郢晢ｽｼ郢晢ｽｫ郢晉甥・､逕ｻ驪､霑･・ｶ隲ｷ荵晢ｽ堤ｹ晄ｧｭﾎ晉ｹ昶・繝ｰ郢昴・繝ｵ郢ｧ・｡邵ｺ・ｧ驕抵ｽｺ闖ｫ繝ｻ
    materialBuffer_.Initialize(dxCommon);
    transformationBuffer_.Initialize(dxCommon);
}

void Skybox::MapResource() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
        for (size_t i = 0; i < vertexDataList_.size(); ++i) {
            vertexData_[i] = vertexDataList_[i];
        }
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
        for (size_t i = 0; i < indexDataList_.size(); ++i) {
            indexData_[i] = indexDataList_[i];
        }
    }
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (materialBuffer_[i]) {
            // 陋ｻ譎・ｄ陋滂ｽ､
            materialBuffer_[i]->color = {1.0f, 1.0f, 1.0f, 1.0f};
            materialBuffer_[i]->intensity = 1.0f;
        }
        if (transformationBuffer_[i]) {
            // 陋ｻ譎・ｄ髯ｦ謔溘・
            transformationBuffer_[i]->WVP = Math::MakeIdentity4x4();
            transformationBuffer_[i]->World = Math::MakeIdentity4x4();
            transformationBuffer_[i]->WorldInverseTranspose = Math::MakeIdentity4x4();
        }
    }
}

void Skybox::UnMapResource() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
        indexData_ = nullptr;
    }
}