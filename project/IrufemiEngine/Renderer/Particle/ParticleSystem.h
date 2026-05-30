#include "../Core/IRenderable.h"
#pragma once
#include "Engine/Graphics/Camera/Camera.h"
#include "ParticleResource.h"
#include "Data/Particle.h"
#include "Data/Emitter.h"
#include "IParticleBehavior.h"
#include "../LineInstanced/LineClass.h"
#include "../../Engine/Core/Type/BlendMode.h"
#include "../../Engine/Graphics/Pipeline/PSOManager.h"
#include "../../Engine/Core/Math/Vector3.h"
#include "../../Engine/Core/Math/Vector4.h"
#include "../../Engine/Core/Math/Matrix4x4.h"
#include "../../Engine/Core/Math/Geometry/AABB.h"
#include "../../Engine/Core/Type/PrimitiveType.h"
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

/**
 * @class ParticleSystem
 * @brief CPU制御のパーティクルシステムを管理するクラス
 * @details 粒子の放出（Emit）、更新（Update）、インスタンシングによる一括描画（Draw）を制御します。
 *          IParticleBehavior を通じて粒子の挙動（移動、色変化等）をカスタマイズ可能です。
 */
class ParticleSystem : public IRenderable {
public:
	ParticleSystem() = default;
	~ParticleSystem();

	/** @name 初期化・更新・描画 */
	///@{
	/**
	 * @brief 初期化
	 * @param[in] camera 描画に使用するカメラ（ビルボード計算等に使用）
	 * @param[in] textureName 使用するテクスチャのパス
	 * @param[in] type パーティクルの挙動タイプ
	 * @param[in] shape パーティクルの形状（板ポリゴン、メッシュ等）
	 */
	void Initialize(const std::string& textureName = "resources/circle.png", ParticleType type = ParticleType::Normal, PrimitiveType shape = PrimitiveType::Plane);
	/** @brief 全粒子の更新とエミッターからの放出 */
	void Update();
	/** @brief インスタンシング描画の実行 */
	void SyncBeforeDraw() override;
    void Draw() override;
	/** @brief メイン更新がスキップされた場合のGPU同期処理 */
	void SyncGPUData();
	/** @brief DebugUI を使用したパラメータ調整機能 */
	void Debug(const char* particleName = "");
	///@}

	/** @name リソース情報取得 */
	///@{
	uint32_t GetInstanceCount() const { return numInstance_; }
	ParticleResource* GetD3D12Resource() { return resource_.get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { 
		uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
		return resource_ ? resource_->instancingSrvHandleGPU_[lastUpdateFrameIndex_] : D3D12_GPU_DESCRIPTOR_HANDLE{}; 
	}
	///@}

	/** @name 静的マネージャ設定 */
	///@{
	static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
	static void SetDrawManager(DrawManager* drawM) { s_drawManager_ = drawM; }
	static void SetDebugUI(DebugUI* ui) { s_ui_ = ui; }
	static void SetSrvPool(DescriptorPool* pool) { s_srvPool_ = pool; }
	static void SetEngine(class IrufemiEngine* engine) { s_engine_ = engine; }
	///@}

	/** @name エミッタープロパティ設定 */
	///@{
	/** @brief 放出位置を設定 */
	void SetEmitterPosition(const Vector3& position);
	/** @brief 放出範囲（AABB）を設定 */
	void SetEmitterArea(const Vector3& area);
	/** @brief 初速の範囲を設定 */
	void SetEmitterVelocity(const Vector3& minVel, const Vector3& maxVel);
	/** @brief 放出頻度（秒間何回更新か）を設定 */
	void SetEmitterFrequency(float frequency);
	/** @brief 1回の放出で生成する数 */
	void SetEmitterCount(uint32_t count);
	/** @brief スケールの変化範囲（開始時と終了時）を設定 */
	void SetParticleScale(const Vector3& start, const Vector3& end);
	/** @brief 色の変化範囲（開始時と終了時）を設定 */
	void SetParticleColor(const Vector4& start, const Vector4& end);
	/** @brief パーティクルの色変化モードを設定 */
	void SetParticleColorMode(ParticleColorMode mode);
	/** @brief 主要なエミッタープロパティを一括設定 */
	void SetEmitterProperties(
		const Vector3& position,
		const Vector3& area,
		const Vector3& minVel,
		const Vector3& maxVel,
		float frequency,
		uint32_t count);
	/** @brief 使用するテクスチャを切り替える */
	void SetTexture(const std::string& textureFilePath);
	///@}

	/** @name 特殊演出 */
	///@{
	/** @brief 指定位置でヒットエフェクト（バースト放出）を再生 */
	void PlayHitEffect(const Vector3& position);
	/** @brief ヒットエフェクトを再生する(数と位置を指定) */
	void PlayHitEffect(const Vector3& position, uint32_t count);

	/** @brief Ring形状の放出パラメータ設定 */
	void SetRingParameters(float innerRadius, float outerRadius,
	                       float startAngleDeg, float endAngleDeg,
	                       uint32_t segmentCount, bool verticalUV = false);

	/** @brief Cylinder形状の放出パラメータ設定 */
	void SetCylinderParameters(float radius, float height, uint32_t segmentCount, bool flipV = false);
	///@}

	/** @name デバッグ描画 */
	///@{
	void DrawAABB(const AABB& aabb, const Vector4& color);
	///@}

	/** @name 描画設定（パイプライン） */
	///@{
	void SetBlend(BlendMode blend) { selectedBlend_ = blend; }
	void SetDepthWrite(PSOManager::DepthWrite depth) { selectedDepth_ = depth; }
	void SetCull(PSOManager::CullMode cull) { selectedCull_ = cull; }
	void SetUVTransform(const Matrix4x4& transform) {
		if (resource_) {
			resource_->GetMaterialData()->uvTransform = transform;
		}
	}
	///@}

	/** @name デバッグ表示切り替え */
	///@{
	void SetShowEmitterAABB(bool show) { showEmitterAABB_ = show; }
	bool IsShowEmitterAABB() const { return showEmitterAABB_; }
	void SetShowFieldAABB(bool show) { showFieldAABB_ = show; }
	bool IsShowFieldAABB() const { return showFieldAABB_; }

	void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
	bool IsCullingEnabled() const { return isCullingEnabled_; }
	///@}

private:
	/** @brief パーティクルの挙動（ Behavior ）を動的に切り替える */
	void ChangeBehavior(ParticleType type, bool force = false);
	/** @brief 新しい粒子を 1 つ生成する */
	Particle MakeNewParticle(std::mt19937& randomEngine, const Emitter& emitter);
	/** @brief エミッターから粒子を複数生成（放出）する */
	std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine);

private:
	static constexpr uint32_t kNumMaxInstance_ = ParticleResource::kNumMaxInstance;

	std::unique_ptr<ParticleResource> resource_ = nullptr;
	uint32_t numInstance_ = 0;
	uint32_t instancingSrvIndex_[kMaxFramesInFlight] = { UINT32_MAX, UINT32_MAX, UINT32_MAX };
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_[kMaxFramesInFlight]{} ;
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_[kMaxFramesInFlight]{} ; // インデックス解放用に保持


	Matrix4x4 billboardMatrix_{};
	Matrix4x4 backToFrontMatrix_{};
	bool useBillboard_ = true;
	bool isUpdate_ = true;
	uint32_t lastUpdateFrameIndex_ = static_cast<uint32_t>(-1);

	std::list<Particle> particles_; ///< 生存しているパーティクルのリスト
	Emitter emitter_; ///< パーティクル放出器の設定
	std::unique_ptr<IParticleBehavior> behavior_ = nullptr; ///< 挙動ロジック（Strategyパターン）
	ParticleType particleType_ = ParticleType::Normal; ///< 現在の挙動タイプ
	PrimitiveType primitiveShape_ = PrimitiveType::Plane; ///< 使用する形状タイプ

	std::random_device seedGenerator_;
	std::mt19937 randomEngine_;

	static TextureManager* textureManager_;
	static DrawManager* s_drawManager_;
	static DebugUI* s_ui_;
	static DescriptorPool* s_srvPool_;
	static class IrufemiEngine* s_engine_;

	int selectedTextureIndex_ = 0; ///< 選択中のテクスチャインデックス

	// Ring 用パラメータ(デフォルト)
	float ringInnerRadius_ = 0.2f;  ///< 内半径
	float ringOuterRadius_ = 0.5f;  ///< 外半径
	float ringStartAngleDeg_ = 0.0f;  ///< 開始角度（度）
	float ringEndAngleDeg_ = 360.0f; ///< 終了角度（度）
	uint32_t ringSegmentCount_ = 32; ///< 分割数
	bool ringVerticalUV_ = false;   ///< UVを垂直方向に貼るか

	// Cylinder 用パラメータ(デフォルト)
	float cylinderRadius_ = 0.5f;  ///< 半径
	float cylinderHeight_ = 1.0f;  ///< 高さ
	uint32_t cylinderSegmentCount_ = 32; ///< 分割数
	bool cylinderFlipV_ = false;   ///< V反転

	std::unique_ptr<Line3DRegion> debugLineRegion_; ///< 範囲表示用のライン描画

	// デバッグ表示フラグ
	bool showEmitterAABB_ = true; ///< エミッター範囲の表示
	bool showFieldAABB_ = true;   ///< フィールド範囲の表示
	bool isCullingEnabled_ = true; ///< 視錐台カリングの有効フラグ

	// 描画設定のセッター(Debug UI で設定され、Draw の直前にエンジンへ反映する)
	BlendMode selectedBlend_ = BlendMode::kBlendModeAdd; ///< ブレンドモード
	PSOManager::DepthWrite selectedDepth_ = PSOManager::DepthWrite::Disable; ///< デプス書き込み
	PSOManager::CullMode selectedCull_ = PSOManager::CullMode::None; ///< カリングモード
};

