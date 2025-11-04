#pragma once
#include "Sprite.h"
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>

class NumberText {
public:
	NumberText();
	~NumberText();

	// コピー禁止 / ムーブ可
	NumberText(const NumberText&) = delete;
	NumberText& operator=(const NumberText&) = delete;
	NumberText(NumberText&&) noexcept;
	NumberText& operator=(NumberText&&) noexcept;

	// 初期化（1文字 = 横32 × 縦64 を想定）
	void Initialize(Camera* camera,
		const std::string& textureName = "resources/texture/text_num.png",
		float cellW = 32.0f, float cellH = 64.0f, size_t maxDigits = 8);

	// 終了処理
	void Shutdown();

	// 右上基準位置（rightTop.x が右端の座標）
	void SetPosRightTop(const Vector2& rightTop);
	const Vector2& GetRightTop() const { return baseRightTop_; }

	// 桁数を変更（再生成）
	void SetMaxDigits(size_t maxDigits);

	// 文字間隔（px）
	void SetTracking(float trackingPx);
	float GetTracking() const { return tracking_; }

	// 1桁サイズ（px）
	void SetCellSize(float w, float h);
	float GetCellW() const { return cellW_; }
	float GetCellH() const { return cellH_; }

	// 全体倍率（1.0 = 元サイズ）
	void SetScale(float s);
	float GetScale() const { return scale_; }

	// 指定桁数の横幅を取得（配置計算用）
	float GetWidthForDigits(size_t digits) const;

	// 色を一括設定
	void SetColor(const Vector4& color);

	// 描画（右寄せ）
	void DrawNumber(uint64_t value);
	void DrawString(const std::string& digits); // '0'～'9' のみ

	// ImGui デバッグUI（サイズ/トラッキング/scale を編集）
	void DebugImGui(const char* label = "NumberText");

private:
	// 内部
	void EnsureDigits_();
	void ResetDigits_();
	bool IsReady_() const;
	void UpdateLayout_() const;
	void SetDigitRect_(Sprite* spr, uint32_t digit) const;
	Sprite* CreateDigitSpriteAt_(size_t index) const;

private:
	Camera* camera_ = nullptr;
	std::string textureName_ = "resources/texture/text_num.png";
	float cellW_ = 32.0f;
	float cellH_ = 64.0f;
	size_t maxDigits_ = 8;
	float tracking_ = 0.0f; // px

	// 全体倍率（1.0 = 元サイズ）
	float scale_ = 1.0f;

	// 右上基準位置（右端の座標）
	Vector2 baseRightTop_{ 400.0f, 32.0f };

	// [0] = 右端（1の位）、[1] = その左... の順
	std::vector<Sprite*> digits_;
};