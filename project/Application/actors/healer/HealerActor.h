#pragma once

#include "math/Transform.h"
#include "math/Vector3.h"
#include "math/shape/OBB.h"

#include <memory>
#include <list>

#include "3D/ObjClass.h"

// 前方宣言
class Camera;
class Wall;

class HealerActor
{
public:
	HealerActor();
	~HealerActor();
	void Initialize(Camera* camera, const Vector3& pos);
	void Update();
	void Draw();

	Vector3 GetPosition() const;
	void MoveTowards(const Vector3& target, float speed, const std::list<Wall*>& walls);
	void RefreshTransform();

	void SetAssigned(bool assigned);
	bool IsAssigned() const;

	void SetAlive(bool alive);	
	bool IsAlive() const;

	void SetTargetPosition(const Vector3& pos);
	const Vector3& GetTargetPosition() const;


	void UpdateOBB();

	const OBB& GetOBB() const;

	void HandleCollision();


private:

	std::unique_ptr<ObjClass> model_ = nullptr;
	Transform transform_;
	Camera* camera_ = nullptr;
	bool assigned_ = false;
	OBB obb_{};

	bool alive_ = true;

	Vector3 targetPosition_{};

	float width_ = 0.5f;

	float height_ = 0.5f;

	float depth_ = 0.5f;
};
