#ifndef FRACTAL_NEWTON_MPI_H 
#define FRACTAL_NEWTON_MPI_H 

#include <cstdint> 

void newton_mpi(double x_min, double y_min, double x_max, double y_max, 
    uint32_t width, uint32_t height, 
    uint32_t row_start, uint32_t row_end,
    uint32_t* pixel_buffer,
    int max_iteraciones,
    long long& local_iters, // referencia para devolver la suma acumulada de iteraciones de este rank (64 bits evita desbordamientos)
    double& compute_time_ms // Referencia para devolver el tiempo de computo (en milisegundos) medido por este rank
);

#endif