#pragma once

#include "math/Transform.h"
#include "math/Vector3.h"

#include "memory.h"

#include "3D/ObjClass.h"

// 前方宣言
class Camera;

class HealerActor
{
public:
	HealerActor();
	~HealerActor();
	void Initialize(Camera* camera, const Vector3& pos);
	void Update();
	void Draw();

	Vector3 GetPosition() const;
	void MoveTowards(const Vector3& target, float speed);
	void RefreshTransform();

	void SetAssigned(bool assigned);
	bool IsAssigned() const;


private:

	std::unique_ptr<ObjClass> model_ = nullptr;
	Transform worldTransform_;
	Camera* camera_ = nullptr;
	bool assigned_ = false;

};
