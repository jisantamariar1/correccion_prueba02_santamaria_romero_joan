#include "fractal_burning_mpi.h" 
#include <complex>          
#include "palette.h" 
#include <cmath> //para el std::abs      

uint32_t acotado(double x, double y, int& iter_salida, int max_iteraciones) { //necesitamos el numero de iteraciones
    std::complex<double> c(x,y);
    //condicion de z0 =0
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;


    //condicion de escape
    //se usa valor absoluto
    while (iter < max_iteraciones && (zr * zr + zi * zi) < 4.0) {
        //zn+1 = (|Re(zn)| + i|Im(zn)|)^2 + c
        //sacamos el valor absoluto antes de multiplicar
        double abs_zr = std::abs(zr);
        double abs_zi = std::abs(zi);
        
        // Aplicamos binomio al cuadrado: (a + bi)^2 = (a^2 - b^2) + (2ab)i.
        double dr = abs_zr * abs_zr - abs_zi * abs_zi + c.real(); // Nueva parte real.
        double di = 2.0 * abs_zr * abs_zi + c.imag();     // Nueva parte imaginaria.
        
        zr = dr; // Actualizamos para la siguiente iteración.
        zi = di;
        iter++;
    }

    //exportamos la cantidad de iteraciones para el histograma
    iter_salida=iter;

    if(iter < max_iteraciones){
        int index = iter % PALETTE_SIZE; // Usamos el contador de iteraciones para elegir un color de la paleta.
        return color_ramp[index]; // Devolvemos el color correspondiente de la paleta.
        //return 0xFF0000FF; // Rojo si escapa.
    }
    return 0xFF000000; // Negro si no escapa.
}

// Función principal con local_hist
void burning_mpi(double x_min, double y_min, double x_max, double y_max, 
    uint32_t width, uint32_t height, 
    uint32_t row_start, uint32_t row_end,
    uint32_t* pixel_buffer,
    int* local_hist, //arreglo de 16 posiciones
    int max_iteraciones

) {
    
    double dx = (x_max - x_min) / width;   
    double dy = (y_max - y_min) / height;  

    for(int j = row_start; j < row_end; j++) {
        for(int i = 0; i < width; i++) {
            
            double x = x_min + i * dx;
            double y = y_min + j * dy;

            int iteraciones_escape = 0;
            //acotado ahora rellena iteraciones_escape
            auto color = acotado(x,y, iteraciones_escape, max_iteraciones);

            pixel_buffer[(j - row_start) * width + i] = color;

            //actualizar el histograma
            //solo si el pixel escapo, lo contamos en el bin correspondiente?
            if(iteraciones_escape < max_iteraciones){
                int bin = (iteraciones_escape * 16) / max_iteraciones;
                //para no salirnos
                if(bin >= 16) bin = 15;
                local_hist[bin]++;
            }
        }
    }

}