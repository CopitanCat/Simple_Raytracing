#pragma once
#include "vec3.h"
#include "raytracer.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <omp.h>

inline std::vector<vec3> render_sequential(int width, int height, float fov) {
    std::vector<vec3> framebuffer(width * height);
    for (int pix = 0; pix < width * height; pix++) {
        framebuffer[pix] = compute_pixel(pix, width, height, fov);
    }
    return framebuffer;
}

inline std::vector<vec3> render_parallel(int width, int height, float fov, int rank, int size) {
    int base_rows = height / size;
    int remainder = height % size;

    int start_row = rank * base_rows + std::min(rank, remainder);
    int local_rows = base_rows + (rank < remainder ? 1 : 0);
    int start_pix = start_row * width;
    int local_pixels = local_rows * width;

    std::vector<vec3> local_buffer(local_pixels);

    #pragma omp parallel for schedule(dynamic, 16)
    for (int i = 0; i < local_pixels; i++) {
        local_buffer[i] = compute_pixel(start_pix + i, width, height, fov);
    }

    std::vector<int> recvcounts(size);
    std::vector<int> displs(size);
    if (rank == 0) {
        int offset_elements = 0;
        for (int r = 0; r < size; r++) {
            int r_rows = base_rows + (r < remainder ? 1 : 0);
            recvcounts[r] = r_rows * width * 3; // 3 float'а на vec3 (надежнее, чем MPI_BYTE)
            displs[r] = offset_elements;
            offset_elements += recvcounts[r];
        }
    }

    std::vector<vec3> full_framebuffer;
    if (rank == 0) {
        full_framebuffer.resize(width * height);
    }

    MPI_Gatherv(
        local_buffer.data(), local_pixels * 3, MPI_FLOAT,
        rank == 0 ? full_framebuffer.data() : nullptr,
        recvcounts.data(), displs.data(), MPI_FLOAT,
        0, MPI_COMM_WORLD
    );

    return full_framebuffer;
}

inline bool verify_correctness(const std::vector<vec3>& seq, const std::vector<vec3>& par, float eps = 1e-5f) {
    if (seq.size() != par.size()) return false;
    for (size_t i = 0; i < seq.size(); i++) {
        if (std::abs(seq[i].x - par[i].x) > eps ||
            std::abs(seq[i].y - par[i].y) > eps ||
            std::abs(seq[i].z - par[i].z) > eps) {
            std::cerr << "[ERROR] Mismatch at pixel " << i << "\n";
            return false;
        }
    }
    return true;
}

inline void save_ppm(const std::string& filename, const std::vector<vec3>& framebuffer, int width, int height) {
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (const vec3& color : framebuffer) {
        float max_val = std::max(1.f, std::max(color[0], std::max(color[1], color[2])));
        for (int chan : {0, 1, 2})
            ofs << (char)(255 * color[chan] / max_val);
    }
}