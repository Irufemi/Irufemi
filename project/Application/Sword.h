#pragma once
#include <memory>
#include <functional>// コールバック用

#include "math/shape/OBB.h"
#include "math/Vector3.h"
#include "math/Transform.h"

class Player;
class Camera;
class ObjClass;

class Sword
{
public:
	void Initialize(Camera* camera, const Vector3& pos);
	void Update();
	void Draw();

	void UpdateOBB();

	
	void SetPosition(const Vector3& pos);
	void SetTransform(const Transform& t);

	
	void StartSlash(const Transform& anchor, float duration = 0.28f);

	
	const OBB& GetOBB() const { return obb_; }
	bool IsSlashing() const { return isSlashing_; }


	uint32_t GetCurrentSlashId() const { return currentSlashId_; }

	// 現在のTransformを取得
	const Transform& GetTransform() const { return transform_; }

	// 斬撃開始時のコールバック設定
	void SetOnSlashStart(std::function<void(const Transform&)> callback) {
		onSlashStart_ = callback;
	}

private:
	void CreateObj(Camera* camera);

	OBB obb_{};
	
	float width_ = 0.2f;

	float height_ = 2.7f;

	float depth_ = 0.2f;

	Camera* camera_ = nullptr;

	Transform transform_;

private:
	std::unique_ptr<ObjClass> swordModel_ = nullptr;

	
	bool isSlashing_ = false;
	float slashTimer_ = 0.0f;
	float slashDuration_ = 0.2f; 
	Transform slashStartTransform_{};
	Transform slashEndTransform_{};

	
	uint32_t currentSlashId_ = 0;

	std::function<void(const Transform&)> onSlashStart_;
};