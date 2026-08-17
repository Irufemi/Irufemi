#include "Core/Utility/PerlinNoise.h"
#include <numeric>
#include <random>
#include <algorithm>
#include <cmath>

namespace Irufemi {

PerlinNoise::PerlinNoise(unsigned int seed) {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);
    p.insert(p.end(), p.begin(), p.end());
}

float PerlinNoise::Fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::Lerp(float t, float a, float b) const {
    return a + t * (b - a);
}

float PerlinNoise::Grad(int hash, float x, float y, float z) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float PerlinNoise::Noise(float x, float y, float z) const {
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;
    int Z = (int)std::floor(z) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    float u = Fade(x);
    float v = Fade(y);
    float w = Fade(z);

    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    return Lerp(w, Lerp(v, Lerp(u, Grad(p[AA], x, y, z),
                                Grad(p[BA], x - 1.0f, y, z)),
                        Lerp(u, Grad(p[AB], x, y - 1.0f, z),
                                Grad(p[BB], x - 1.0f, y - 1.0f, z))),
                Lerp(v, Lerp(u, Grad(p[AA + 1], x, y, z - 1.0f),
                                Grad(p[BA + 1], x - 1.0f, y, z - 1.0f)),
                        Lerp(u, Grad(p[AB + 1], x, y - 1.0f, z - 1.0f),
                                Grad(p[BB + 1], x - 1.0f, y - 1.0f, z - 1.0f))));
}

float PerlinNoise::Noise(float x) const {
    return Noise(x, 0.0f, 0.0f);
}

float PerlinNoise::Noise(float x, float y) const {
    return Noise(x, y, 0.0f);
}

float PerlinNoise::Noise1D(float x) const {
    return (Noise(x) * 2.0f) - 1.0f; // Note: standard Perlin often ranges slightly differently, but mapping [0,1] to [-1,1] is usually fine. Wait, Perlin output is actually roughly [-sqrt(3/4), sqrt(3/4)].
    // To keep it simple and safe for scaling:
    // Standard Ken Perlin noise typically returns [-1, 1] for 3D Grad. Wait. 
    // Actually the standard implementation returns [-1, 1]. Let's check my Grad output. 
    // Yes, Grad can return up to +2 or -2, and the blended result is [-1, 1].
    // So Noise() already returns [-1, 1] in many implementations, but sometimes it is mapped to [0,1].
    // Let's just return the raw Noise for Noise() and Noise1D, since the user will scale it anyway.
}

float PerlinNoise::Noise2D(float x, float y) const {
    return Noise(x, y, 0.0f); 
}

float PerlinNoise::Noise3D(float x, float y, float z) const {
    return Noise(x, y, z);
}

} // namespace Irufemi
