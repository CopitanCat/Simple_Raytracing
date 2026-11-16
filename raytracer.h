#pragma once
#include "vec3.h"
#include "scene.h"
#include <algorithm>

inline vec3 cast_ray(const vec3& orig, const vec3& dir, const int depth = 0) {
    auto [hit, point, N, material] = scene_intersect(orig, dir);
    if (depth > 4 || !hit)
        return {0.2, 0.7, 0.8};

    vec3 reflect_dir = reflect(dir, N).normalized();
    vec3 refract_dir = refract(dir, N, material.refractive_index).normalized();
    vec3 reflect_color = cast_ray(point, reflect_dir, depth + 1);
    vec3 refract_color = cast_ray(point, refract_dir, depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (const vec3& light : lights) {
        vec3 light_dir = (light - point).normalized();
        auto [hit_sh, shadow_pt, trashnrm, trashmat] = scene_intersect(point, light_dir);
        if (hit_sh && (shadow_pt - point).norm() < (light - point).norm()) continue;
        diffuse_light_intensity += std::max(0.f, light_dir * N);
        specular_light_intensity += std::pow(std::max(0.f, -reflect(-light_dir, N) * dir), material.specular_exponent);
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] +
           vec3{1., 1., 1.} * specular_light_intensity * material.albedo[1] +
           reflect_color * material.albedo[2] +
           refract_color * material.albedo[3];
}

inline vec3 compute_pixel(int pix, int width, int height, float fov) {
    float dir_x =  (pix % width + 0.5f) - width / 2.f;
    float dir_y = -(pix / width + 0.5f) + height / 2.f;
    float dir_z = -height / (2.f * std::tan(fov / 2.f));
    return cast_ray(vec3{0, 0, 0}, vec3{dir_x, dir_y, dir_z}.normalized());
}