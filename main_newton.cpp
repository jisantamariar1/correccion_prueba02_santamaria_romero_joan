#include <iostream>
#include <mpi.h>
#include <complex>
#include <vector>
#include <fmt/core.h>
#include <cstring>
#include <SFML/Graphics.hpp>
#include "fractal_newton_mpi.h"
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
double x_min = -1.5; 
double x_max = 1.5;  
double y_min = -1.0; 
double y_max = 1.0; 

int max_iteraciones = 50;

#define WIDTH 1600
#define HEIGHT 900
uint32_t* pixel_buffer = nullptr;
uint32_t* texture_buffer = nullptr;

int running = 1;
int row_start;
int row_end;
int padding;
int delta;
int nprocs; 
int rank;   

void dibujar_texto(int rank, int mis_filas){
    auto texto = fmt::format("RANK_{} - ITER: {}", rank, max_iteraciones);
    draw_text_to_texture(
        (unsigned char*)pixel_buffer,
        WIDTH, mis_filas,
        texto.c_str(),
        10, 25, 20);
}

void setup_ui(){
    texture_buffer = new uint32_t[WIDTH * HEIGHT];
    std::memset(texture_buffer, 0, WIDTH * HEIGHT * sizeof(uint32_t));

    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Fractal de Newton MPI");

#ifdef _WIN32
    HWND hwnd = window.getNativeHandle(); 
    ShowWindow(hwnd, SW_MAXIMIZE);        
#endif
    sf::Texture texture({WIDTH, HEIGHT});
    texture.update((const uint8_t *)texture_buffer); 
    sf::Sprite sprite(texture); 

    const sf::Font font(arial_ttf::data, arial_ttf::data_len);

    sf::Text text(font, "Cargando...", 20); 
    text.setFillColor(sf::Color::White); // Blanco resalta mejor en fondos oscuros
    text.setPosition({10, 30}); 
    text.setStyle(sf::Text::Bold); 

    std::string options = "Up/Down: Change iterations | Esc: Exit";
    sf::Text textOptions(font, options, 18);
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getSize().y - 40});

    int frames = 0;
    int fps = 0;
    sf::Clock clock; 

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()){
                running = 0;
                window.close();
            }
            else if(event->is<sf::Event::KeyReleased>()) {
                auto evt = event->getIf<sf::Event::KeyReleased>();
                switch(evt->scancode) {
                    case sf::Keyboard::Scan::Up:
                        max_iteraciones += 10;
                        break;
                    case sf::Keyboard::Scan::Down:
                        max_iteraciones -= 10;
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
        
        // Comunicacion colectiva Bcast
        int meta_int[2] = {max_iteraciones, running};
        double meta_double[4] = {x_min, x_max, y_min, y_max};
        
        MPI_Bcast(meta_int, 2, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(meta_double, 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if(running == 0) {
            break;
        }

        // Variables para las métricas locales
        long long local_iters = 0;
        double local_time = 0.0;
        long long global_iters = 0;
        double max_time = 0.0;

        // Calcular porcion del rank 0
        int mis_filas = row_end - row_start;
        newton_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer, max_iteraciones, local_iters, local_time);
        dibujar_texto(0, mis_filas);
        
        std::memcpy(texture_buffer, pixel_buffer, WIDTH * mis_filas * sizeof(uint32_t));
        
        // Recibir imagenes
        for(int i = 1; i < nprocs; i++) {
            int slave_start = i * delta;
            int slave_end = slave_start + delta;
            if (slave_end > HEIGHT) slave_end = HEIGHT;
            int slave_rows = slave_end - slave_start;

            MPI_Recv(pixel_buffer, WIDTH * slave_rows, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::memcpy(texture_buffer + (slave_start * WIDTH), pixel_buffer, WIDTH * slave_rows * sizeof(uint32_t));
        }

        // Comunicacion colectiva Reduce (Tiempo e Iteraciones)
        MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&local_iters, &global_iters, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        texture.update((const uint8_t *)texture_buffer);
        frames++;

        if (clock.getElapsedTime().asSeconds() >= 1.0f){
            fps = frames;
            frames = 0;
            clock.restart();
        }

        // Overlay actualizado 
        auto msg = fmt::format(
            "RANK: 0\nMAX_ITER: {}\nMAX_COMPUTE_MS: {:.2f} ms\nTOTAL_ITERS: {}\nFPS: {}", 
            max_iteraciones, max_time, global_iters, fps
        );
        text.setString(msg);

        window.clear();      
        window.draw(sprite); 
        window.draw(text);   
        window.draw(textOptions); 
        window.display();    
    }

    delete[] texture_buffer;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

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
        while(true){
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
                break;
            }
            
            long long local_iters = 0;
            double local_time = 0.0;
            int mis_filas = row_end - row_start;
            
            newton_mpi(x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer, max_iteraciones, local_iters, local_time);
            dibujar_texto(rank, mis_filas);
            
            // enviar pixeles 
            MPI_Send(pixel_buffer, WIDTH * mis_filas, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);

            
            //reduce (esclavos envian su tiempo e iteraciones a rank 0)
            MPI_Reduce(&local_time, nullptr, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            MPI_Reduce(&local_iters, nullptr, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        }
    }

    delete[] pixel_buffer;
    MPI_Finalize();
    return 0;
}