#include "Player.h"
#include "Sword.h"

void Player::MoveBy(const Vector3& delta)
{
    transform_.translate += delta;
    
    // プレイヤー移動に剣を追従（非スラッシュ時 or スラッシュ中はアンカー更新）
    if (sword_) {
        if (sword_->IsSlashing()) {
            // スラッシュ中はアンカー（プレイヤー位置等）を更新して追従
            sword_->UpdateSlashAnchor(transform_);
        } else {
            // 非スラッシュ時はTransformを直接同期
            sword_->SetTransform(transform_);
        }
    }

    UpdateOBB();
}
