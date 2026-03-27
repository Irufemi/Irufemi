#pragma once

// 敵の行動状態を独立させることで、どこからでも安全に参照できるようにします
enum class EnemyState {
    Idle,        // 待機
    Attack_Beam, // ビーム攻撃
    Attack_Stomp, // スタンプ攻撃
    Damaged      // 被弾
};