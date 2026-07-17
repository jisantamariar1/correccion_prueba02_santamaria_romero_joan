
#ifndef FRACTAL_BURNING_MPI_H 

#define FRACTAL_BURNING_MPI_H 

#include <cstdint> 


void burning_mpi(double x_min, double y_min, double x_max, double y_max, 
    uint32_t width, uint32_t height, 
    uint32_t row_start, uint32_t row_end,
    uint32_t* pixel_buffer,
    int* local_hist, // puntero al arreglo del histograma local para registrar las frecuencias de iteraciones sin hacer copias
    int max_iteraciones
);

#endif