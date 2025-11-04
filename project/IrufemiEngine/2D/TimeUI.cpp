#define NOMINMAX

#include "TimeUI.h"
#include <algorithm>

using namespace KamataEngine;

TimeUI::TimeUI() = default;
TimeUI::~TimeUI() { Shutdown(); }

// ムーブ
TimeUI::TimeUI(TimeUI&& rhs) noexcept { *this = std::move(rhs); }
TimeUI& TimeUI::operator=(TimeUI&& rhs) noexcept {
	if (this == &rhs)
		return *this;
	Shutdown();
	std::copy(std::begin(rhs.sprMM_), std::end(rhs.sprMM_), std::begin(sprMM_));
	sprColon_ = rhs.sprColon_;
	std::copy(std::begin(rhs.sprSS_), std::end(rhs.sprSS_), std::begin(sprSS_));
	sprComma_ = rhs.sprComma_;
	std::copy(std::begin(rhs.sprCC_), std::end(rhs.sprCC_), std::begin(sprCC_));
	sprSecOnly_ = rhs.sprSecOnly_;
	std::copy(std::begin(rhs.sprSS2_), std::end(rhs.sprSS2_), std::begin(sprSS2_));
	sprComma2_ = rhs.sprComma2_;
	std::copy(std::begin(rhs.sprCC2_), std::end(rhs.sprCC2_), std::begin(sprCC2_));
	fps_ = rhs.fps_;
	cell_ = rhs.cell_;
	texNum_ = rhs.texNum_;
	texMark_ = rhs.texMark_;
	mode_ = rhs.mode_;
	baseMMSSCC_ = rhs.baseMMSSCC_;
	baseSecOnlyRight_ = rhs.baseSecOnlyRight_;
	baseSSCC_ = rhs.baseSSCC_;
	rhs.ResetAll_();
	rhs.texNum_ = rhs.texMark_ = 0;
	return *this;
}

// ===== 基本セット =====
void TimeUI::SetResources(uint32_t numHandle, uint32_t markHandle, float cellPx) {
	const bool texChanged = (texNum_ != numHandle) || (texMark_ != markHandle);
	const bool cellChanged = (cell_ != cellPx);
	texNum_ = numHandle;
	texMark_ = markHandle;
	cell_ = cellPx;

	if (texChanged || cellChanged) {
		if (sprMM_[0]) {
			ResetMMSSCC_();
			EnsureMMSSCC_();
		}
		if (sprSecOnly_[0]) {
			ResetSecOnly_();
			EnsureSecOnly_();
		}
		if (sprSS2_[0]) {
			ResetSSCC_();
			EnsureSSCC_();
		}
	}
}
void TimeUI::SetFPS(uint32_t fps) { fps_ = std::max(1u, fps); }
uint32_t TimeUI::GetFPS() const { return fps_; }

// ===== モード =====
void TimeUI::SetMode(Mode m) { mode_ = m; }
TimeUI::Mode TimeUI::GetMode() const { return mode_; }

// ===== 位置 =====
void TimeUI::SetPosMMSSCC(const Vector2& leftTop) {
	baseMMSSCC_ = leftTop;
	EnsureMMSSCC_();
	if (!sprMM_[0])
		return;
	Vector2 p = baseMMSSCC_;
	sprMM_[0]->SetPosition(p);
	p.x += cell_;
	sprMM_[1]->SetPosition(p);
	p.x += cell_;
	sprColon_->SetPosition(p);
	p.x += cell_;
	sprSS_[0]->SetPosition(p);
	p.x += cell_;
	sprSS_[1]->SetPosition(p);
	p.x += cell_;
	sprComma_->SetPosition(p);
	p.x += cell_;
	sprCC_[0]->SetPosition(p);
	p.x += cell_;
	sprCC_[1]->SetPosition(p);
}
void TimeUI::SetPosSecondsOnlyRight(const Vector2& rightTop) {
	baseSecOnlyRight_ = rightTop;
	EnsureSecOnly_();
	if (!sprSecOnly_[0])
		return;
	Vector2 rp = baseSecOnlyRight_;
	for (auto* s : sprSecOnly_) {
		s->SetPosition(rp);
		rp.x -= cell_;
	}
}
void TimeUI::SetPosSSCC(const Vector2& leftTop) {
	baseSSCC_ = leftTop;
	EnsureSSCC_();
	if (!sprSS2_[0])
		return;
	Vector2 p2 = baseSSCC_;
	sprSS2_[0]->SetPosition(p2);
	p2.x += cell_;
	sprSS2_[1]->SetPosition(p2);
	p2.x += cell_;
	sprComma2_->SetPosition(p2);
	p2.x += cell_;
	sprCC2_[0]->SetPosition(p2);
	p2.x += cell_;
	sprCC2_[1]->SetPosition(p2);
}
void TimeUI::SetPosForCurrentMode(const Vector2& pos) {
	switch (mode_) {
	case Mode::MMSSCC:
		SetPosMMSSCC(pos);
		break; // 左端
	case Mode::SecondsOnly:
		SetPosSecondsOnlyRight(pos);
		break; // 右端
	case Mode::SSCC:
		SetPosSSCC(pos);
		break; // 左端
	}
}

// ===== 描画（現在モードのみ）=====
void TimeUI::Draw(const uint32_t& flametime) {
	switch (mode_) {
	case Mode::MMSSCC:
		EnsureMMSSCC_();
		DrawMMSSCC_(flametime);
		break;
	case Mode::SecondsOnly:
		EnsureSecOnly_();
		DrawSecondsOnly_(flametime);
		break;
	case Mode::SSCC:
		EnsureSSCC_();
		DrawSSCC_(flametime);
		break;
	}
}

// ===== 解放 =====
void TimeUI::Shutdown() { ResetAll_(); }

// ===== 内部処理 =====
TimeSplit TimeUI::SplitFromFrames(uint32_t frames) const {
	const uint64_t totalCenti = (static_cast<uint64_t>(frames) * 100ull) / static_cast<uint64_t>(fps_);
	return TimeSplit{static_cast<uint32_t>(totalCenti / 6000ull), static_cast<uint32_t>((totalCenti / 100ull) % 60ull), static_cast<uint32_t>(totalCenti % 100ull)};
}
void TimeUI::SetDigitByRect(Sprite* spr, uint32_t digit) const {
	const float x = cell_ * static_cast<float>(digit % 10);
	spr->SetTextureRect({x, 0.0f}, {cell_, cell_});
}
void TimeUI::SetMarkByIndex(Sprite* spr, uint32_t index) const {
	const float x = cell_ * static_cast<float>(index);
	spr->SetTextureRect({x, 0.0f}, {cell_, cell_});
}
Sprite* TimeUI::CreateNumSprite_(const Vector2& pos) const {
	Sprite* s = Sprite::Create(texNum_, pos);
	s->SetSize({cell_, cell_});
	s->SetTextureRect({0.0f, 0.0f}, {cell_, cell_});
	return s;
}
Sprite* TimeUI::CreateMarkSprite_(const Vector2& pos, uint32_t markIndex) const {
	Sprite* s = Sprite::Create(texMark_, pos, Vector4{0.0f, 0.0f, 1.0f, 1.0f});
	s->SetSize({cell_, cell_});
	SetMarkByIndex(s, markIndex);
	return s;
}
bool TimeUI::HasValidResources_() const { return texNum_ != 0 && texMark_ != 0 && cell_ > 0.0f; }
void TimeUI::EnsureMMSSCC_() {
	if (sprMM_[0] || !HasValidResources_())
		return;
	Vector2 p = baseMMSSCC_;
	sprMM_[0] = CreateNumSprite_(p);
	p.x += cell_;
	sprMM_[1] = CreateNumSprite_(p);
	p.x += cell_;
	sprColon_ = CreateMarkSprite_(p, 0);
	p.x += cell_;
	sprSS_[0] = CreateNumSprite_(p);
	p.x += cell_;
	sprSS_[1] = CreateNumSprite_(p);
	p.x += cell_;
	sprComma_ = CreateMarkSprite_(p, 1);
	p.x += cell_;
	sprCC_[0] = CreateNumSprite_(p);
	p.x += cell_;
	sprCC_[1] = CreateNumSprite_(p);
}
void TimeUI::EnsureSecOnly_() {
	if (sprSecOnly_[0] || !HasValidResources_())
		return;
	Vector2 rp = baseSecOnlyRight_;
	for (auto& s : sprSecOnly_) {
		s = CreateNumSprite_(rp);
		rp.x -= cell_;
	}
}
void TimeUI::EnsureSSCC_() {
	if (sprSS2_[0] || !HasValidResources_())
		return;
	Vector2 p2 = baseSSCC_;
	sprSS2_[0] = CreateNumSprite_(p2);
	p2.x += cell_;
	sprSS2_[1] = CreateNumSprite_(p2);
	p2.x += cell_;
	sprComma2_ = CreateMarkSprite_(p2, 1);
	p2.x += cell_;
	sprCC2_[0] = CreateNumSprite_(p2);
	p2.x += cell_;
	sprCC2_[1] = CreateNumSprite_(p2);
}
void TimeUI::ResetMMSSCC_() {
	auto kill = [](Sprite*& p) {
		if (p) {
			delete p;
			p = nullptr;
		}
	};
	for (auto& p : sprMM_)
		kill(p);
	kill(sprColon_);
	for (auto& p : sprSS_)
		kill(p);
	kill(sprComma_);
	for (auto& p : sprCC_)
		kill(p);
}
void TimeUI::ResetSecOnly_() {
	auto kill = [](Sprite*& p) {
		if (p) {
			delete p;
			p = nullptr;
		}
	};
	for (auto& p : sprSecOnly_)
		kill(p);
}
void TimeUI::ResetSSCC_() {
	auto kill = [](Sprite*& p) {
		if (p) {
			delete p;
			p = nullptr;
		}
	};
	for (auto& p : sprSS2_)
		kill(p);
	kill(sprComma2_);
	for (auto& p : sprCC2_)
		kill(p);
}
void TimeUI::ResetAll_() {
	ResetMMSSCC_();
	ResetSecOnly_();
	ResetSSCC_();
}

// ----- 個別描画（内部） -----
void TimeUI::DrawMMSSCC_(const uint32_t& flametime) {
	if (!sprMM_[0])
		return;
	const TimeSplit t = SplitFromFrames(flametime);
	SetDigitByRect(sprMM_[0], (t.mm / 10) % 10);
	SetDigitByRect(sprMM_[1], t.mm % 10);
	SetDigitByRect(sprSS_[0], (t.ss / 10) % 10);
	SetDigitByRect(sprSS_[1], t.ss % 10);
	SetDigitByRect(sprCC_[0], (t.cc / 10) % 10);
	SetDigitByRect(sprCC_[1], t.cc % 10);
	sprMM_[0]->Draw();
	sprMM_[1]->Draw();
	sprColon_->Draw();
	sprSS_[0]->Draw();
	sprSS_[1]->Draw();
	sprComma_->Draw();
	sprCC_[0]->Draw();
	sprCC_[1]->Draw();
}
void TimeUI::DrawSecondsOnly_(const uint32_t& flametime) {
	if (!sprSecOnly_[0])
		return;

	uint32_t v = 0;
	if (secondsRoundMode_ == SecondsOnlyRounding::Truncate) {
		// 切り捨て
		v = static_cast<uint32_t>(flametime / fps_);
	} else {
		// 切り上げ（ただし 0 フレームは 0 秒）
		// frames>0 かつ frames%fps_!=0 なら次の秒へ
		v = (flametime == 0) ? 0u : static_cast<uint32_t>((static_cast<uint64_t>(flametime) + fps_ - 1) / fps_);
	}

	int i = 0;
	do {
		SetDigitByRect(sprSecOnly_[i], v % 10);
		sprSecOnly_[i]->Draw(); // 右端から描画
		v /= 10;
		++i;
	} while (v > 0 && i < static_cast<int>(sprSecOnly_.size()));
}
void TimeUI::DrawSSCC_(const uint32_t& flametime) {
	if (!sprSS2_[0])
		return;
	const TimeSplit t = SplitFromFrames(flametime);
	SetDigitByRect(sprSS2_[0], (t.ss / 10) % 10);
	SetDigitByRect(sprSS2_[1], t.ss % 10);
	SetDigitByRect(sprCC2_[0], (t.cc / 10) % 10);
	SetDigitByRect(sprCC2_[1], t.cc % 10);
	sprSS2_[0]->Draw();
	sprSS2_[1]->Draw();
	sprComma2_->Draw();
	sprCC2_[0]->Draw();
	sprCC2_[1]->Draw();
}

void TimeUI::SetSecondsOnlyRoundingMode(SecondsOnlyRounding mode) { secondsRoundMode_ = mode; }
TimeUI::SecondsOnlyRounding TimeUI::GetSecondsOnlyRoundingMode() const { return secondsRoundMode_; }