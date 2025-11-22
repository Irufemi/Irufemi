#pragma once
#include "Application/camera/Camera.h"
#include "source/D3D12ResourceUtil.h"
#include "math/shape/Particle.h"
#include "math/Emitter.h"
#include "3D/particle/IParticleBehavior.h" // 追加
#include <list>
#include <memory>
#include <random>
#include <string>

// 前方宣言
class TextureManager;
class DebugUI;
class DescriptorPool;

class ParticleSystem {
public:
	ParticleSystem() = default;
	~ParticleSystem();

	void Initialize(Camera* camera, const std::string& textureName = "resources/circle.png", ParticleType type = ParticleType::Normal, PrimitiveShape shape = PrimitiveShape::Plane);
	void Update();
	void Draw();
	void Debug(const char* particleName = "");

	uint32_t GetInstanceCount() const { return numInstance_; }
	D3D12ResourceUtilParticle* GetD3D12Resource() { return resource_.get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvHandleGPU_; }

	static void SetTextureManager(TextureManager* texM) { s_textureManager_ = texM; }
	static void SetDebugUI(DebugUI* ui) { s_ui_ = ui; }
	static void SetSrvPool(DescriptorPool* pool) { s_srvPool_ = pool; }

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

	void PlayHitEffect(const Vector3& position);

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
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_{};
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_{};
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
	PrimitiveShape primitiveShape_ = PrimitiveShape::Plane;

	std::random_device seedGenerator_;
	std::mt19937 randomEngine_;

	static TextureManager* s_textureManager_;
	static DebugUI* s_ui_;
	static DescriptorPool* s_srvPool_;

	int selectedTextureIndex_ = 0;
};