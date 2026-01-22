#pragma once

#include <list>
#include <memory>
#include "math/Vector3.h"
#include "math/Transform.h"
#include "math/shape/OBB.h"

class Wall;
class HealerActor;
class Camera;
class ObjClass;

class Enemy {
public:
	Enemy();
	~Enemy();
	void Initialize(Camera* camera, Vector3 pos);

	void Update(const std::list<Wall*>& walls, const std::list<HealerActor*>& healers);
	void Draw();

	void UpdateOBB();

	const OBB& GetOBB() const;

	void HandleCollision();

	bool IsAlive() const { return alive_; }
	void Kill();

private:
	OBB obb_{};

	float speed;

	bool alive_ = true;

	int respawnCounter_ = 0;
	static inline const int kRespawnFrames = 300; // フレーム数でリスポーンまでの待ち

	float width_ = 2.0f;

	float height_ = 2.0f;

	float depth_ = 2.0f;

	bool preferHealer_ = false;
	int preferHealerTimer_ = 0;
	static inline const int kPreferHealerFrames = 60;


private:
	std::unique_ptr<ObjClass> model_ = nullptr;
	Transform transform_;
	Camera* camera_ = nullptr;
};