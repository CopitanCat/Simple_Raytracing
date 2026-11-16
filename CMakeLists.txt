cmake_minimum_required(VERSION 3.16)
project(HybridRayTracer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

find_package(OpenMP REQUIRED)
find_package(MPI QUIET)

add_executable(raytracer main.cpp)
target_link_libraries(raytracer PRIVATE OpenMP::OpenMP_CXX)

# Статически вшиваем только библиотеки MinGW/OpenMP, но оставляем динамическую линковку для msmpi.dll
target_link_options(raytracer PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,-Bstatic -lgomp -lpthread -Wl,-Bdynamic
)

if(MPI_CXX_FOUND)
    target_link_libraries(raytracer PRIVATE MPI::MPI_CXX)
else()
    target_include_directories(raytracer PRIVATE "C:/msys64/ucrt64/include")
    target_link_directories(raytracer PRIVATE "C:/msys64/ucrt64/lib")
    target_link_libraries(raytracer PRIVATE msmpi)
    set(MPIEXEC_EXECUTABLE "C:/Program Files/Microsoft MPI/Bin/mpiexec.exe" CACHE FILEPATH "" FORCE)
    set(MPIEXEC_NUMPROC_FLAG "-n" CACHE STRING "" FORCE)
endif()

set(WIDTH "1920" CACHE STRING "Image width")
set(HEIGHT "1080" CACHE STRING "Image height")

add_custom_target(benchmark
        DEPENDS raytracer
        USES_TERMINAL
        COMMENT "Running Sequential vs Parallel RayTracer Benchmark..."

        # 1. БЕЗ параллелизации (1 MPI процесс x 1 OpenMP поток)
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo ">>> [1/4] RUNNING SEQUENTIAL (1 MPI x 1 OpenMP)..."
        COMMAND ${CMAKE_COMMAND} -E env OMP_NUM_THREADS=1 "${MPIEXEC_EXECUTABLE}" ${MPIEXEC_NUMPROC_FLAG} 1 "$<TARGET_FILE:raytracer>" ${WIDTH} ${HEIGHT}

        # 2. ТОЛЬКО OpenMP (1 MPI процесс x 4 OpenMP потока)
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo ">>> [2/4] RUNNING SHARED MEMORY ONLY (1 MPI x 4 OpenMP)..."
        COMMAND ${CMAKE_COMMAND} -E env OMP_NUM_THREADS=4 "${MPIEXEC_EXECUTABLE}" ${MPIEXEC_NUMPROC_FLAG} 1 "$<TARGET_FILE:raytracer>" ${WIDTH} ${HEIGHT}

        # 3. ТОЛЬКО MPI (4 MPI процесса x 1 OpenMP поток)
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo ">>> [3/4] RUNNING DISTRIBUTED ONLY (4 MPI x 1 OpenMP)..."
        COMMAND ${CMAKE_COMMAND} -E env OMP_NUM_THREADS=1 "${MPIEXEC_EXECUTABLE}" ${MPIEXEC_NUMPROC_FLAG} 4 "$<TARGET_FILE:raytracer>" ${WIDTH} ${HEIGHT}

        # 4. ГИБРИДНАЯ версия (2 MPI процесса x 4 OpenMP потока)
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo ">>> [4/4] RUNNING HYBRID PARALLEL (2 MPI x 4 OpenMP)..."
        COMMAND ${CMAKE_COMMAND} -E env OMP_NUM_THREADS=4 "${MPIEXEC_EXECUTABLE}" ${MPIEXEC_NUMPROC_FLAG} 2 "$<TARGET_FILE:raytracer>" ${WIDTH} ${HEIGHT}
)