#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "renderer.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width  = (argc > 1) ? std::stoi(argv[1]) : 1024;
    int height = (argc > 2) ? std::stoi(argv[2]) : 768;
    constexpr float fov = 1.05f;

    int omp_threads = omp_get_max_threads();
    int total_workers = size * omp_threads;

    std::vector<vec3> seq_framebuffer;
    double t_seq = 0.0;

    if (rank == 0) {
        double t0 = MPI_Wtime();
        seq_framebuffer = render_sequential(width, height, fov);
        t_seq = MPI_Wtime() - t0;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t_par_start = MPI_Wtime();

    std::vector<vec3> par_framebuffer = render_parallel(width, height, fov, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_par = MPI_Wtime() - t_par_start;

    if (rank == 0) {
        bool passed = verify_correctness(seq_framebuffer, par_framebuffer);
        double speedup = t_seq / t_par;
        double efficiency = (speedup / total_workers) * 100.0;

        std::cout << "========================================================\n";
        std::cout << " Resolution       : " << width << " x " << height << " (" << width * height << " pixels)\n";
        std::cout << " MPI Processes    : " << size << "\n";
        std::cout << " OpenMP Threads   : " << omp_threads << " per process\n";
        std::cout << " Total Workers (P): " << total_workers << "\n";
        std::cout << "--------------------------------------------------------\n";
        std::cout << std::fixed << std::setprecision(4);
        std::cout << " Sequential Time (T1) : " << t_seq << " sec\n";
        std::cout << " Parallel Time   (Tp) : " << t_par << " sec\n";
        std::cout << " Speedup         (Sp) : " << speedup << "x\n";
        std::cout << " Efficiency      (Ep) : " << std::setprecision(2) << efficiency << "%\n";
        std::cout << " Correctness Test     : " << (passed ? "PASSED (Exact Match)" : "FAILED") << "\n";
        std::cout << "========================================================\n";

        save_ppm("./out_parallel.ppm", par_framebuffer, width, height);
    }

    MPI_Finalize();
    return 0;
}