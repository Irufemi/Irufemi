#pragma once

#include <list>
#include <memory>
#include "math/Vector3.h"
#include "math/Transform.h"
#include "math/shape/OBB.h"

class Wall;
class Camera;
class ObjClass;

class Enemy {
public:
	Enemy();
	~Enemy();
	void Initialize(Camera* camera, Vector3 pos);

	void Update(const std::list<Wall*>& walls);
	void Draw();

	void UpdateOBB();

	const OBB& GetOBB() const;

	void HandleCollision();

	bool IsAlive() const { return alive_; }
	void Kill() { alive_ = false; }

private:
	OBB obb_{};

	float speed;

	bool alive_ = true;

	float width_ = 2.0f;

	float height_ = 2.0f;

	float depth_ = 2.0f;

private:
	std::unique_ptr<ObjClass> model_ = nullptr;
	Transform transform_;
	Camera* camera_ = nullptr;
};