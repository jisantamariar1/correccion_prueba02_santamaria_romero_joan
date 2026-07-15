#ifndef FRACTAL_NEWTON_MPI_H 
#define FRACTAL_NEWTON_MPI_H 

#include <cstdint> 

void newton_mpi(double x_min, double y_min, double x_max, double y_max, 
    uint32_t width, uint32_t height, 
    uint32_t row_start, uint32_t row_end,
    uint32_t* pixel_buffer,
    int max_iteraciones,
    long long& local_iters,
    double& compute_time_ms
);

#endif