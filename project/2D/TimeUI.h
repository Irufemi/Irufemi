#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstdint>

struct TimeSplit {
	uint32_t mm, ss, cc;
};

class TimeUI {
public:
	enum class Mode { MMSSCC, SecondsOnly, SSCC }; // ← これで選択

	// 総秒の丸め方式
	enum class SecondsOnlyRounding { Truncate, Ceil };

	TimeUI();
	~TimeUI();

	// コピー禁止 / ムーブ可
	TimeUI(const TimeUI&) = delete;
	TimeUI& operator=(const TimeUI&) = delete;
	TimeUI(TimeUI&&) noexcept;
	TimeUI& operator=(TimeUI&&) noexcept;

	// ---- 必須セット ----
	void SetResources(uint32_t numHandle, uint32_t markHandle, float cellPx);
	void SetFPS(uint32_t fps);
	uint32_t GetFPS() const;

	// ---- 表示モード ----
	void SetMode(Mode m);
	Mode GetMode() const;

	// ---- 位置（必要なものだけ呼べばOK）----
	void SetPosMMSSCC(const KamataEngine::Vector2& leftTop);
	// 総秒は右寄せ（rightTop が右端）
	void SetPosSecondsOnlyRight(const KamataEngine::Vector2& rightTop);
	void SetPosSSCC(const KamataEngine::Vector2& leftTop);

	// 現在モード用の位置だけを一発で設定（総秒は右端、他は左端）
	void SetPosForCurrentMode(const KamataEngine::Vector2& pos);

	// ---- 描画（現在モードのみ描く）----
	void Draw(const uint32_t& flametime);

	// 明示解放（任意）
	void Shutdown();

	//　丸め方式のセッター/ゲッター
	void SetSecondsOnlyRoundingMode(SecondsOnlyRounding mode);
	SecondsOnlyRounding GetSecondsOnlyRoundingMode() const;

private:
	// 分解・UV
	TimeSplit SplitFromFrames(uint32_t frames) const;
	void SetDigitByRect(KamataEngine::Sprite* spr, uint32_t digit) const;
	void SetMarkByIndex(KamataEngine::Sprite* spr, uint32_t index) const; // 0=':', 1=','

	// 生成ヘルパ
	KamataEngine::Sprite* CreateNumSprite_(const KamataEngine::Vector2& pos) const;
	KamataEngine::Sprite* CreateMarkSprite_(const KamataEngine::Vector2& pos, uint32_t markIndex) const;

	// グループ生成/破棄（必要なときだけ）
	void EnsureMMSSCC_();
	void EnsureSecOnly_();
	void EnsureSSCC_();
	void ResetMMSSCC_();
	void ResetSecOnly_();
	void ResetSSCC_();
	void ResetAll_();
	bool HasValidResources_() const;

	// 個別描画（内部用）
	void DrawMMSSCC_(const uint32_t& flametime);
	void DrawSecondsOnly_(const uint32_t& flametime);
	void DrawSSCC_(const uint32_t& flametime);

private:
	// mm:ss,cc
	KamataEngine::Sprite* sprMM_[2]{};
	KamataEngine::Sprite* sprColon_{};
	KamataEngine::Sprite* sprSS_[2]{};
	KamataEngine::Sprite* sprComma_{};
	KamataEngine::Sprite* sprCC_[2]{};
	// 総秒（右端から最大6桁）
	std::array<KamataEngine::Sprite*, 6> sprSecOnly_{};
	// ss,cc
	KamataEngine::Sprite* sprSS2_[2]{};
	KamataEngine::Sprite* sprComma2_{};
	KamataEngine::Sprite* sprCC2_[2]{};

	// 設定
	uint32_t fps_ = 60;
	float cell_ = 64.0f;
	uint32_t texNum_ = 0;
	uint32_t texMark_ = 0;
	Mode mode_ = Mode::MMSSCC;

	// 基準位置
	KamataEngine::Vector2 baseMMSSCC_{32.0f, 32.0f};
	KamataEngine::Vector2 baseSecOnlyRight_{400.0f, 32.0f};
	KamataEngine::Vector2 baseSSCC_{32.0f, 100.0f};

	// 追加：現在の丸め方式（既定=切り上げ）
	SecondsOnlyRounding secondsRoundMode_ = SecondsOnlyRounding::Ceil;
};
