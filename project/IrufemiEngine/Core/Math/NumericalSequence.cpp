#include "Core/Math/NumericalSequence.h"
#include <cmath>

namespace Irufemi {
namespace Math::Sequence {

float CalculateArithmetic(float firstTerm, float difference, uint32_t index) {
    return firstTerm + (static_cast<float>(index) * difference);
}

float CalculateGeometric(float firstTerm, float ratio, uint32_t index) {
    return firstTerm * std::pow(ratio, static_cast<float>(index));
}

std::vector<float> GenerateArithmetic(float firstTerm, float difference, uint32_t count) {
    std::vector<float> sequence;
    sequence.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        sequence.push_back(CalculateArithmetic(firstTerm, difference, i));
    }
    return sequence;
}

std::vector<float> GenerateGeometric(float firstTerm, float ratio, uint32_t count) {
    std::vector<float> sequence;
    sequence.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        sequence.push_back(CalculateGeometric(firstTerm, ratio, i));
    }
    return sequence;
}
} // namespace Math::Sequence

} // namespace Irufemi
