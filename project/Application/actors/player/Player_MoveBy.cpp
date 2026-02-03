#include "Player.h"

void Player::MoveBy(const Vector3& delta)
{
    transform_.translate += delta;
    UpdateOBB();
}
