#pragma once

#include <memory>

#include "math/shape/OBB.h"
#include "math/Vector3.h"
#include "math/Transform.h"

// 前方宣言
class Camera;
class ObjClass;
class InputManager;

class Player {
public:
	Player();
	~Player();
	void Initialize(Camera* camera, const Vector3 & pos, InputManager* input);
	void Update();
	void Draw();

	void UpdateOBB();

	// OBB の取得
	const OBB& GetOBB() const;

	// 衝突時の処理
	void HandleCollision();

private:

	OBB obb_{};

	Vector3 velocity = {};

	float width_ = 2.0f;

	float height_ = 2.0f;

	float depth_ = 2.0f;

	static inline const float kAcceleration = 0.2f;

private:

	std::unique_ptr<ObjClass> model_ = nullptr;

	Transform transform_;

	Camera* camera_ = nullptr;

	InputManager* input_ = nullptr;

private:
	void Move();
};
