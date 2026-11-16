#pragma once
#include "vec3.h"
#include <tuple>
#include <vector>
#include <algorithm>

struct Material {
    float refractive_index = 1;
    float albedo[4] = {2, 0, 0, 0};
    vec3 diffuse_color = {0, 0, 0};
    float specular_exponent = 0;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

inline constexpr Material ivory      = {1.0, {0.9,  0.5, 0.1, 0.0}, {0.4, 0.4, 0.3},   50.};
inline constexpr Material glass      = {1.5, {0.0,  0.9, 0.1, 0.8}, {0.6, 0.7, 0.8},  125.};
inline constexpr Material red_rubber = {1.0, {1.4,  0.3, 0.0, 0.0}, {0.3, 0.1, 0.1},   10.};
inline constexpr Material mirror     = {1.0, {0.0, 16.0, 0.8, 0.0}, {1.0, 1.0, 1.0}, 1425.};

inline constexpr Sphere spheres[] = {
    {{-3,    0,   -16}, 2,       ivory},
    {{-1.0, -1.5, -12}, 2,       glass},
    {{ 1.5, -0.5, -18}, 3, red_rubber},
    {{ 7,    5,   -18}, 4,      mirror}
};

inline constexpr vec3 lights[] = {
    {-20, 20,  20},
    { 30, 50, -25},
    { 30, 20,  30}
};

inline std::tuple<bool, float> ray_sphere_intersect(const vec3& orig, const vec3& dir, const Sphere& s) {
    vec3 L = s.center - orig;
    float tca = L * dir;
    float d2 = L * L - tca * tca;
    if (d2 > s.radius * s.radius) return {false, 0};
    float thc = std::sqrt(s.radius * s.radius - d2);
    float t0 = tca - thc, t1 = tca + thc;
    if (t0 > .001) return {true, t0};
    if (t1 > .001) return {true, t1};
    return {false, 0};
}

inline std::tuple<bool, vec3, vec3, Material> scene_intersect(const vec3& orig, const vec3& dir) {
    vec3 pt, N;
    Material material;

    float nearest_dist = 1e10;
    if (std::abs(dir.y) > .001) {
        float d = -(orig.y + 4) / dir.y;
        vec3 p = orig + dir * d;
        if (d > .001 && d < nearest_dist && std::abs(p.x) < 10 && p.z < -10 && p.z > -30) {
            nearest_dist = d;
            pt = p;
            N = {0, 1, 0};
            material.diffuse_color = (int(.5 * pt.x + 1000) + int(.5 * pt.z)) & 1 ? vec3{.3, .3, .3} : vec3{.3, .2, .1};
        }
    }

    for (const Sphere& s : spheres) {
        auto [intersection, d] = ray_sphere_intersect(orig, dir, s);
        if (!intersection || d > nearest_dist) continue;
        nearest_dist = d;
        pt = orig + dir * nearest_dist;
        N = (pt - s.center).normalized();
        material = s.material;
    }
    return {nearest_dist < 1000, pt, N, material};
}