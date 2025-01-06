#pragma once
#include <cmath>
#include <random>
#include <array>

class PerlinNoise {
private:
    std::array<int, 512> p;

    static double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    static double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    static double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    PerlinNoise(unsigned int seed = std::random_device{}()) {
        // Initialize the permutation array
        std::array<int, 256> permutation;
        for(int i = 0; i < 256; i++) {
            permutation[i] = i;
        }

        std::default_random_engine engine(seed);
        std::shuffle(permutation.begin(), permutation.end(), engine);

        // Duplicate the permutation array to avoid overflow
        for(int i = 0; i < 512; i++) {
            p[i] = permutation[i & 255];
        }
    }

    double noise(double x, double y, double z) const {
        // Find unit cube that contains point
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        // Find relative x, y, z of point in cube
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        // Compute fade curves for each of x, y, z
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        // Hash coordinates of the 8 cube corners
        int A  = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B  = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        // Add blended results from 8 corners of cube
        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                     grad(p[BA], x-1, y, z)),
                             lerp(u, grad(p[AB], x, y-1, z),
                                     grad(p[BB], x-1, y-1, z))),
                     lerp(v, lerp(u, grad(p[AA+1], x, y, z-1),
                                     grad(p[BA+1], x-1, y, z-1)),
                             lerp(u, grad(p[AB+1], x, y-1, z-1),
                                     grad(p[BB+1], x-1, y-1, z-1))));
    }

    // Utility function to generate octaves of noise
    double octaveNoise(double x, double y, double z, int octaves, double persistence = 0.5) const {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double maxValue = 0.0;

        for(int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }

        return total / maxValue;
    }


    // Helper function to normalize noise to a desired range
    static double normalize(double value, double min, double max) {
        return (value + 1.0) * 0.5 * (max - min) + min;
    }
};