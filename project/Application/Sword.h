#pragma once
#include <memory>

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

	
	void StartSlash(const Transform& anchor);

	
	const OBB& GetOBB() const { return obb_; }
	bool IsSlashing() const { return isSlashing_; }

	// Get current slash id (incremented on each StartSlash) so collision logic can avoid multiple hits per slash
	uint32_t GetCurrentSlashId() const { return currentSlashId_; }

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

	// Incremented each time a slash starts. Used by GameScene to prevent multiple hits from the same slash.
	uint32_t currentSlashId_ = 0;
};