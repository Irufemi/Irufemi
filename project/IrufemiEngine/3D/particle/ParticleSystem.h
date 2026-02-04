#pragma once
#include "Application/camera/Camera.h"
#include "source/D3D12ResourceUtil.h"
#include "math/shape/Particle.h"
#include "math/Emitter.h"
#include "3D/particle/IParticleBehavior.h"
#include "3D/LineClass.h"
#include "math/BlendMode.h"
#include "engine/directX/PSOManager.h"
#include <list>
#include <memory>
#include <random>
#include <string>
#include <vector>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;
class DescriptorPool;
class IrufemiEngine;

class ParticleSystem {
public:
	void EmitOnce();

	ParticleSystem() = default;
	~ParticleSystem();

	void Initialize(Camera* camera, const std::string& textureName = "resources/circle.png", ParticleType type = ParticleType::Normal, ParticlePrimitiveShape shape = ParticlePrimitiveShape::Plane);
	void Update();
	void Draw();
	void Debug(const char* particleName = "");

	uint32_t GetInstanceCount() const { return numInstance_; }
	D3D12ResourceUtilParticle* GetD3D12Resource() { return resource_.get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvHandleGPU_; }

	static void SetTextureManager(TextureManager* texM) { s_textureManager_ = texM; }
	static void SetDrawManager(DrawManager* drawM) { s_drawManager_ = drawM; }
	static void SetDebugUI(DebugUI* ui) { s_ui_ = ui; }
	static void SetSrvPool(DescriptorPool* pool) { s_srvPool_ = pool; }
	static void SetEngine(class IrufemiEngine* engine) { s_engine_ = engine; }

	const Vector3& GetEmitterPosition() const { return emitter_.transform.translate; }
	void SetEmitterPosition(const Vector3& position);
	void SetEmitterArea(const Vector3& area);
	void SetEmitterVelocity(const Vector3& minVel, const Vector3& maxVel);
	void SetEmitterFrequency(float frequency);
	void SetEmitterCount(uint32_t count);
	void SetParticleScale(const Vector3& start, const Vector3& end);
	void SetParticleColor(const Vector4& start, const Vector4& end);
	void SetParticleColorMode(ParticleColorMode mode);
	void SetEmitterProperties(
		const Vector3& position,
		const Vector3& area,
		const Vector3& minVel,
		const Vector3& maxVel,
		float frequency,
		uint32_t count);
	void SetTexture(const std::string& textureFilePath);

	// 単発エフェクトを再生する
	void PlayHitEffect(const Vector3& position);
	// 単発エフェクトを再生する(数と位置を指定)
	void PlayHitEffect(const Vector3& position, uint32_t count);

	// 追加: Ring パラメータ設定
	void SetRingParameters(float innerRadius, float outerRadius,
	                       float startAngleDeg, float endAngleDeg,
	                       uint32_t segmentCount, bool verticalUV = false);

	// 追加: Cylinder パラメータ設定
	void SetCylinderParameters(float radius, float height, uint32_t segmentCount, bool flipV = false);

	void DrawAABB(const AABB& aabb, const Vector4& color);

	// デバッグ表示切り替え
	void SetShowEmitterAABB(bool show) { showEmitterAABB_ = show; }
	bool IsShowEmitterAABB() const { return showEmitterAABB_; }
	void SetShowFieldAABB(bool show) { showFieldAABB_ = show; }
	bool IsShowFieldAABB() const { return showFieldAABB_; }

private:
	void ChangeBehavior(ParticleType type, bool force = false); // 追加
	Particle MakeNewParticle(std::mt19937& randomEngine, const Emitter& emitter);
	std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);

private:
	static constexpr uint32_t kNumMaxInstance_ = 4096;
	static constexpr float kDeltatime_ = 1.0f / 60.0f;

	std::unique_ptr<D3D12ResourceUtilParticle> resource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_ = nullptr;
	ParticleForGPU* instancingData_ = nullptr;
	uint32_t numInstance_ = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_{} ;
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_{} ;
	uint32_t instancingSrvIndex_ = UINT32_MAX;

	Camera* camera_ = nullptr;
	Matrix4x4 billbordMatrix_{};
	Matrix4x4 backToFrontMatrix_{};
	bool useBillbord_ = true;
	bool isUpdate_ = true;

	std::list<Particle> particles_;
	Emitter emitter_;
	std::unique_ptr<IParticleBehavior> behavior_ = nullptr; // 振る舞いクラスへのポインタ
	ParticleType particleType_ = ParticleType::Normal;
	ParticlePrimitiveShape primitiveShape_ = ParticlePrimitiveShape::Plane;

	std::random_device seedGenerator_;
	std::mt19937 randomEngine_;

	static TextureManager* s_textureManager_;
	static DrawManager* s_drawManager_;
	static DebugUI* s_ui_;
	static DescriptorPool* s_srvPool_;
	static class IrufemiEngine* s_engine_;

	int selectedTextureIndex_ = 0;

	// Ring 用パラメータ(デフォルト)
	float ringInnerRadius_ = 0.2f;
	float ringOuterRadius_ = 0.5f;
	float ringStartAngleDeg_ = 0.0f;
	float ringEndAngleDeg_ = 360.0f;
	uint32_t ringSegmentCount_ = 32;
	bool ringVerticalUV_ = false;

	// Cylinder 用パラメータ(デフォルト)
	float cylinderRadius_ = 0.5f;
	float cylinderHeight_ = 1.0f;
	uint32_t cylinderSegmentCount_ = 32;
	bool cylinderFlipV_ = false;

	std::vector<std::unique_ptr<Line3DClass>> debugLines_;

	// デバッグ表示フラグ
	bool showEmitterAABB_ = true;
	bool showFieldAABB_ = true;

	// 描画時の選択(Debug UI で設定され、Draw の直前にエンジンへ反映する)
	BlendMode selectedBlend_ = BlendMode::kBlendModeAdd; // デフォルト: Add(既存シーンと同等)
	PSOManager::DepthWrite selectedDepth_ = PSOManager::DepthWrite::Disable; // デフォルト: Disable(既存シーンと同等)
	PSOManager::CullMode selectedCull_ = PSOManager::CullMode::None; // デフォルト: None
};