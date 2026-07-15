#include <iostream>
#include <mpi.h>
#include <complex>
#include <vector>
#include <fmt/core.h>
#include <cstring> // Para memset
#include <SFML/Graphics.hpp>
#include "fractal_burning_mpi.h"
#include "draw_text.h"

#ifdef _WIN32
    #include <windows.h>
#endif

namespace arial_ttf 
{
    extern size_t data_len;
    extern unsigned char data[];
}

// Parametros por defecto
double x_min = -2; 
double x_max = 1;  
double y_min = -1.5; 
double y_max = 1.5; 

int max_iteraciones = 10;

#define WIDTH 1600
#define HEIGHT 900
uint32_t* pixel_buffer = nullptr;
uint32_t* texture_buffer = nullptr;

int running = 1;
int row_start;
int row_end;
int padding;
int delta;
int nprocs; // Variable global única
int rank;   // Variable global única

// Arreglo local para el histograma
int local_hist[16] = {0};

void dibujar_texto(int rank, int mis_filas){
    auto texto = fmt::format("RANK_{} - ITERACIONES: {}", rank, max_iteraciones);

    draw_text_to_texture(
        (unsigned char*)pixel_buffer,
        WIDTH, mis_filas,
        texto.c_str(),
        10, 25, 20);
}

void setup_ui(){
    texture_buffer = new uint32_t[WIDTH * HEIGHT];
    std::memset(texture_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));

    // Inicializar la UI
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Fractal MPI");

    // Si es Windows, forzamos que la ventana se abra maximizada usando el handle nativo.
#ifdef _WIN32
    HWND hwnd = window.getNativeHandle(); 
    ShowWindow(hwnd, SW_MAXIMIZE);        
#endif
    sf::Texture texture({WIDTH, HEIGHT});
    texture.update((const uint8_t *)texture_buffer); 
    sf::Sprite sprite(texture); 

    // Textos
    const sf::Font font(arial_ttf::data, arial_ttf::data_len);

    // Texto para el overlay (esquina superior)
    sf::Text text(font, "Cargando...", 20); 
    text.setFillColor(sf::Color::Red); 
    text.setPosition({10, 30}); 
    text.setStyle(sf::Text::Bold); 

    // Texto para la ayuda (esquina inferior)
    std::string options = "Up/Down: Change iterations";
    sf::Text textOptions(font, options, 18);
    textOptions.setFillColor(sf::Color::Red);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getSize().y - 40});

    // FPS
    int frames = 0;
    int fps = 0;
    sf::Clock clock; 

    // Buffer para recolectar los histogramas de todos los procesos (Usa el nprocs global correctamente)
    std::vector<int> gather_hist(nprocs * 16, 0);
    int global_hist[16] = {0};

    while (window.isOpen())
    {
        // A. PROCESAR EVENTOS: Entrada del usuario.
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()){
                running = 0;
                window.close();
            }
            else if(event->is<sf::Event::KeyReleased>()) {
                auto evt = event->getIf<sf::Event::KeyReleased>();
                // Controlamos las iteraciones con las flechas del teclado.
                switch(evt->scancode) {
                    case sf::Keyboard::Scan::Up:
                        max_iteraciones += 10; // Más detalle.
                        break;
                    case sf::Keyboard::Scan::Down:
                        max_iteraciones -= 10; // Menos detalle (más rápido).
                        if(max_iteraciones < 10) max_iteraciones = 10;
                        break;
                    case sf::Keyboard::Scan::Escape:
                        running = 0;
                        window.close();
                        break;
                    default:
                        break;
                }

                std::memset(texture_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));
            }
        }
        
        // Comunicacion colectiva
        int meta_int[2] = {max_iteraciones, running};
        double meta_double[4] = {x_min, x_max, y_min, y_max};
        
        MPI_Bcast(meta_int, 2, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(meta_double, 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if(running == 0) {
            break;
        }

        // Limpiar histograma local antes de calcular
        for(int k=0; k<16; k++) local_hist[k] = 0;

        // Calcular porcion del rank 0
        int mis_filas = row_end - row_start;
        burning_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer, local_hist, max_iteraciones);
        dibujar_texto(0, mis_filas);
        
        // Copiar porcion del rank 0 a la textura
        std::memcpy(texture_buffer, pixel_buffer, WIDTH * mis_filas * sizeof(uint32_t));
        
        // Recibir las imagenes parciales de los otros ranks
        for(int i = 1; i < nprocs; i++) {
            int slave_start = i * delta;
            int slave_end = slave_start + delta;
            if (slave_end > HEIGHT) slave_end = HEIGHT;
            int slave_rows = slave_end - slave_start;

            MPI_Recv(pixel_buffer, WIDTH * slave_rows, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::memcpy(texture_buffer + (slave_start * WIDTH), pixel_buffer, WIDTH * slave_rows * sizeof(uint32_t));
        }

        // Comunicacion colectiva (recolectar histogramas)
        MPI_Gather(local_hist, 16, MPI_INT, gather_hist.data(), 16, MPI_INT, 0, MPI_COMM_WORLD);

        // Sumar todos los histogramas recibidos en el global_hist
        for(int k=0; k<16; k++) global_hist[k] = 0; // Limpiar primero
        for(int p=0; p < nprocs; p++) {
            for(int k=0; k<16; k++) {
                global_hist[k] += gather_hist[p * 16 + k];
            }
        }

        // Actualizar la textura
        texture.update((const uint8_t *)texture_buffer);
        frames++;

        // D. CÁLCULO DE FPS: Cada vez que pase 1 segundo, actualizamos el contador.
        if (clock.getElapsedTime().asSeconds() >= 1.0f){
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // OVERLAY SFML (Formatear toda la información)
        std::string hist_str = "";
        for(int k=0; k<16; k++) hist_str += fmt::format("{} ", global_hist[k]);

        auto msg = fmt::format(
            "RANKS: {}\nITERACIONES: {}\nDOMINIO: [{:.1f}, {:.1f}] x [{:.1f}, {:.1f}]\nFPS: {}\nHISTOGRAMA: [{}]", 
            nprocs, max_iteraciones, x_min, x_max, y_min, y_max, fps, hist_str
        );
        text.setString(msg);

        // F. RENDERIZADO:
        window.clear();      // Limpiar la pantalla (borrar el frame anterior).
        window.draw(sprite); // Dibujar el fractal (la textura).
        window.draw(text);   // Dibujar el contador de FPS encima.
        window.draw(textOptions); // Dibujar las opciones de control.
        window.display();    // Intercambiar buffers para mostrar el dibujo en el monitor.
    }

    delete[] texture_buffer; // Liberamos solo lo creado en setup_ui()
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    // SOLUCIONADO: Se eliminó la redefinición local. Ahora se asignan las variables globales.
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    init_freetype();

    delta = std::ceil(HEIGHT * 1.0 / nprocs); 
    row_start = rank * delta; 
    row_end = row_start + delta; 
    padding = delta * nprocs - HEIGHT; 

    if(row_end > HEIGHT) {
        row_end = HEIGHT;
    }

    pixel_buffer = new uint32_t[WIDTH * delta];
    std::memset(pixel_buffer, 0, WIDTH * delta * sizeof(uint32_t)); 

    if(rank == 0){
        setup_ui();
    }
    else {
        // Bucle de los esclavos
        while(true){
            // Recibir los parametros de los esclavos
            int meta_int[2];
            double meta_double[4];
            
            MPI_Bcast(meta_int, 2, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(meta_double, 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            max_iteraciones = meta_int[0];
            running = meta_int[1];
            x_min = meta_double[0];
            x_max = meta_double[1];
            y_min = meta_double[2];
            y_max = meta_double[3];

            if(running == 0) {
                fmt::println("RANK_{}: received shutdown signal. Exiting.", rank);
                break;
            }
            // Limpiar histograma local antes de iterar
            for(int k=0; k<16; k++) local_hist[k] = 0;
            
            // 2. CALCULAR FRACTAL Y HISTOGRAMA
            int mis_filas = row_end - row_start;
            burning_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer, local_hist, max_iteraciones);
            dibujar_texto(rank, mis_filas);
            
            // 3. ENVIAR LA PORCIÓN EXACTA (Solución al Deadlock)
            MPI_Send(pixel_buffer, WIDTH * mis_filas, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);

            // 4. ENVIAR HISTOGRAMA LOCAL AL MAESTRO (MPI_Gather)
            MPI_Gather(local_hist, 16, MPI_INT, nullptr, 0, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }

    // SOLUCIONADO: Liberación centralizada y segura para todos los ranks.
    delete[] pixel_buffer;
    MPI_Finalize();
    return 0;
}