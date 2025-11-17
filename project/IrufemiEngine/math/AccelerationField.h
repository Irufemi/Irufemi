#pragma once

#include "Vector3.h"
#include "shape/AABB.h"
#include "shape/Particle.h"
#include "function/Math.h"

struct AccelerationField {
    Vector3 acceleration; //!< 加速度
    AABB area; //!< 範囲

    void Apply(Particle& particle, float deltaTime) const {
        if (Math::IsCollision(area, particle.transform.translate)) {
            particle.velocity += acceleration * deltaTime;
        }
    }
};