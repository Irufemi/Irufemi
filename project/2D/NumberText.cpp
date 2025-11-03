#define NOMINMAX
#include "NumberText.h"
#include <algorithm>
#include "imgui.h"
#include <cmath>

NumberText::NumberText() = default;
NumberText::~NumberText() { Shutdown(); }

// ムーブ
NumberText::NumberText(NumberText&& rhs) noexcept {
	camera_ = rhs.camera_;
	textureName_ = std::move(rhs.textureName_);
	cellW_ = rhs.cellW_;
	cellH_ = rhs.cellH_;
	maxDigits_ = rhs.maxDigits_;
	tracking_ = rhs.tracking_;
	scale_ = rhs.scale_;
	baseRightTop_ = rhs.baseRightTop_;
	digits_ = std::move(rhs.digits_);
	// leave rhs in safe state
	rhs.camera_ = nullptr;
	rhs.maxDigits_ = 0;
	rhs.digits_.clear();
}
NumberText& NumberText::operator=(NumberText&& rhs) noexcept {
	if (this == &rhs) return *this;
	Shutdown();
	camera_ = rhs.camera_;
	textureName_ = std::move(rhs.textureName_);
	cellW_ = rhs.cellW_;
	cellH_ = rhs.cellH_;
	maxDigits_ = rhs.maxDigits_;
	tracking_ = rhs.tracking_;
	scale_ = rhs.scale_;
	baseRightTop_ = rhs.baseRightTop_;
	digits_ = std::move(rhs.digits_);
	rhs.camera_ = nullptr;
	rhs.maxDigits_ = 0;
	rhs.digits_.clear();
	return *this;
}

void NumberText::Initialize(Camera* camera, const std::string& textureName, float cellW, float cellH, size_t maxDigits) {
	camera_ = camera;
	textureName_ = textureName;
	cellW_ = cellW;
	cellH_ = cellH;
	maxDigits_ = std::max<size_t>(1, maxDigits);
	EnsureDigits_();
}

void NumberText::Shutdown() {
	ResetDigits_();
	camera_ = nullptr;
}

void NumberText::SetPosRightTop(const Vector2& rightTop) {
	baseRightTop_ = rightTop;
	UpdateLayout_();
}

void NumberText::SetMaxDigits(size_t maxDigits) {
	maxDigits_ = std::max<size_t>(1, maxDigits);
	ResetDigits_();
	EnsureDigits_();
}

void NumberText::SetTracking(float trackingPx) {
	tracking_ = trackingPx;
	UpdateLayout_();
}

void NumberText::SetCellSize(float w, float h) {
	cellW_ = std::max(1.0f, w);
	cellH_ = std::max(1.0f, h);
	for (auto* s : digits_) {
		if (s) {
			// スプライト表示サイズには scale を乗じる
			s->SetSize(cellW_ * scale_, cellH_ * scale_);
			// UV はピクセル指定なので再設定（テクスチャ切り出しは元ピクセル単位）
			SetDigitRect_(s, 0u);
		}
	}
	UpdateLayout_();
}

void NumberText::SetScale(float s) {
	// 限界は用途に合わせて調整可能
	const float newScale = std::max(0.01f, s);
	if (std::abs(newScale - scale_) < 1e-6f) return;
	scale_ = newScale;
	// スプライトの描画サイズを更新
	for (auto* sp : digits_) {
		if (sp) sp->SetSize(cellW_ * scale_, cellH_ * scale_);
	}
	UpdateLayout_();
}

float NumberText::GetWidthForDigits(size_t digits) const {
	if (digits == 0) return 0.0f;
	const float scaledW = cellW_ * scale_;
	const float scaledTracking = tracking_ * scale_;
	return static_cast<float>(digits) * scaledW + static_cast<float>((digits > 0) ? (digits - 1) : 0) * scaledTracking;
}

void NumberText::SetColor(const Vector4& color) {
	for (auto* s : digits_) {
		if (s) s->SetColor(color);
	}
}

void NumberText::DrawNumber(uint64_t value) {
	if (!IsReady_()) return;

	uint64_t v = value;
	int i = 0;

	if (v == 0) {
		SetDigitRect_(digits_[0], 0u);
		digits_[0]->Update(false);
		digits_[0]->Draw();
		return;
	}

	while (v > 0 && i < static_cast<int>(digits_.size())) {
		const uint32_t d = static_cast<uint32_t>(v % 10ull);
		SetDigitRect_(digits_[i], d);
		digits_[i]->Update(false);
		digits_[i]->Draw();
		v /= 10ull;
		++i;
	}
}

void NumberText::DrawString(const std::string& digits) {
	if (!IsReady_()) return;
	int i = 0;
	for (auto it = digits.rbegin(); it != digits.rend() && i < static_cast<int>(digits_.size()); ++it, ++i) {
		const char c = *it;
		if (c >= '0' && c <= '9') {
			SetDigitRect_(digits_[i], static_cast<uint32_t>(c - '0'));
			digits_[i]->Update(false);
			digits_[i]->Draw();
		}
	}
}

/* 内部 */
bool NumberText::IsReady_() const {
	return camera_ != nullptr && !textureName_.empty() && cellW_ > 0.0f && cellH_ > 0.0f && !digits_.empty();
}

void NumberText::EnsureDigits_() {
	if (!digits_.empty() || camera_ == nullptr) return;
	digits_.resize(maxDigits_, nullptr);
	for (size_t i = 0; i < maxDigits_; ++i) digits_[i] = CreateDigitSpriteAt_(i);
	UpdateLayout_();
}

void NumberText::ResetDigits_() {
	for (auto*& s : digits_) {
		if (s) { delete s; s = nullptr; }
	}
	digits_.clear();
}

Sprite* NumberText::CreateDigitSpriteAt_(size_t /*index*/) const {
	Sprite* s = new Sprite();
	s->Initialize(camera_, textureName_);
	s->SetAnchor(0.0f, 0.0f);      // 左上基準
	// 表示サイズには scale を乗じる
	s->SetSize(cellW_ * scale_, cellH_ * scale_);
	// 初期は '0'（切り出しは元ピクセル単位）
	SetDigitRect_(s, 0u);
	return s;
}

void NumberText::UpdateLayout_() const {
	if (digits_.empty()) return;
	const float scaledCellW = cellW_ * scale_;
	const float scaledTracking = tracking_ * scale_;
	for (size_t i = 0; i < digits_.size(); ++i) {
		// 右端から i 桁目の左上座標
		const float x = baseRightTop_.x - scaledCellW - (scaledCellW + scaledTracking) * static_cast<float>(i);
		const float y = baseRightTop_.y;
		digits_[i]->SetPositionTopLeft(x, y);
	}
}

void NumberText::SetDigitRect_(Sprite* spr, uint32_t digit) const {
	if (!spr) return;
	const uint32_t d = digit % 10u;
	// 切り出しは整数ピクセルで行う（テクスチャ幅320 を想定して cellW=32）
	const int iw = static_cast<int>(std::round(cellW_));
	const int ih = static_cast<int>(std::round(cellH_));
	const int x = iw * static_cast<int>(d);
	const int y = 0;
	const int w = iw;
	const int h = ih;
	// まず通常モードで試す。失敗したら autoResize=true のフォールバックを試す。
	bool ok = spr->SetTextureRectPixels(x, y, w, h, false);
	if (!ok) {
		// テクスチャ情報がまだ取れない場合があるためフォールバック
		spr->SetTextureRectPixels(x, y, w, h, true);
	}
}

void NumberText::DebugImGui(const char* label) {
	if (!ImGui::CollapsingHeader(label)) return;

	ImGui::TextUnformatted("Layout");
	Vector2 rt = baseRightTop_;
	if (ImGui::DragFloat2("RightTop", &rt.x, 1.0f)) {
		SetPosRightTop(rt);
	}
	float size[2]{ cellW_, cellH_ };
	if (ImGui::DragFloat2("Cell (W,H)", size, 0.5f, 1.0f, 2048.0f)) {
		SetCellSize(size[0], size[1]);
	}
	float sc = scale_;
	if (ImGui::DragFloat("Scale", &sc, 0.01f, 0.01f, 10.0f)) {
		SetScale(sc);
	}
	float tr = tracking_;
	if (ImGui::DragFloat("Tracking", &tr, 0.1f, -64.0f, 256.0f)) {
		SetTracking(tr);
	}

	int md = static_cast<int>(maxDigits_);
	if (ImGui::SliderInt("MaxDigits", &md, 1, 12)) {
		SetMaxDigits(static_cast<size_t>(md));
	}
}