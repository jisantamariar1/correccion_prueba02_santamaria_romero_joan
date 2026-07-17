#include "fractal_newton_mpi.h" 
#include <complex>          
#include "palette.h" 
#include <cmath> 
#include <chrono>

uint32_t acotado_newton(double x, double y, int& iter_salida, int max_iteraciones) {
    std::complex<double> z(x, y);
    
    // Raíces cúbicas 
    const std::complex<double> w0(1.0, 0.0);
    const std::complex<double> w1(-0.5, std::sqrt(3.0)/2.0);
    const std::complex<double> w2(-0.5, -std::sqrt(3.0)/2.0);
    const double epsilon = 1e-4;
    
    int iter = 0;
    int root = -1; // -1 significa que no ha convergido a ninguna

    while (iter < max_iteraciones) {
        double abs_z = std::abs(z);
        
        // Condición de escape
        if (abs_z > 2.0) {
            break;
        }

        // Condición de convergencia
        if (std::abs(z - w0) < epsilon) { root = 0; break; }
        if (std::abs(z - w1) < epsilon) { root = 1; break; }
        if (std::abs(z - w2) < epsilon) { root = 2; break; }

        // Evitar división para cero protegiendo el denominador
        if (abs_z < 1e-9) {
            break;
        }

        // zn+1 = zn - (zn^3 - 1) / (3 * zn^2)
        std::complex<double> z2 = z * z;
        std::complex<double> z3 = z2 * z;
        z = z - (z3 - 1.0) / (3.0 * z2);
        
        iter++;
    }

    iter_salida = iter;

    if (root != -1) { // Si convergió a alguna raíz 
        // calcula un índice único: separa por color de raíz (x5), le suma las iteraciones para el degradado y limita el rango a 15 usando módulo
        int index = (root * 5 + iter) % 16;
        return color_ramp[index];
    }
    
    return 0xFF000000; // Negro si no converge o si escapa
}

void newton_mpi(double x_min, double y_min, double x_max, double y_max, 
    uint32_t width, uint32_t height, 
    uint32_t row_start, uint32_t row_end,
    uint32_t* pixel_buffer,
    int max_iteraciones,
    long long& local_iters,
    double& compute_time_ms
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    local_iters = 0;

    //tamaño de paso de cada pixel en el plano complejo
    double dx = (x_max - x_min) / width;   
    double dy = (y_max - y_min) / height;  

    for(int j = row_start; j < row_end; j++) {
        for(int i = 0; i < width; i++) {
    
            //mapea la posicion del pixel con sus cooredenadas reales en el plano (x,y)
            double x = x_min + i * dx;
            double y = y_min + j * dy;

            //ejecuta el fractal y obtiene el color e iteraciones
            int iteraciones = 0;
            uint32_t color = acotado_newton(x, y, iteraciones, max_iteraciones);

            //guarda el color en el indice y acumula el costo matematico
            pixel_buffer[(j - row_start) * width + i] = color;
            local_iters += iteraciones;
        }
    }

    //calcula el tiempo trascurrido
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;
    compute_time_ms = duration.count();
}