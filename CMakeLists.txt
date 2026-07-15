cmake_minimum_required(VERSION 3.16)
project(ejemplo-mpi)

# 1. Definir variables PRIMERO (para que estén disponibles)
set(MPT_ROOT "C:/oneAPI/mpi/latest") #ruta base
set(MPI_INCLUDE "${MPT_ROOT}/include") #archivos .h (includes)
set(MPI_LIB "${MPT_ROOT}/lib") # librerias (linker)

find_package(SFML COMPONENTS System Graphics Window CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

# 2. Configurar directorios (ahora sí conoce MPI_INCLUDE)
include_directories(${MPI_INCLUDE})

# 3. Definir los ejecutables
add_executable(fractal_mpi 
main.cpp 
fractal_mpi.cpp 
palette.cpp 
draw_text.cpp)



# 4. Buscar librerías externas
find_package(fmt CONFIG REQUIRED)

# 5. Enlazar las librerías a cada ejecutable
#impi.lib intel mpi (arhivo con todo el codigo binario de la implementacion MPI de intel)
#target_link_libraries(ejemplo-mpi PRIVATE fmt::fmt ${MPI_LIB}/impi.lib) 
target_link_libraries(fractal_mpi PRIVATE fmt::fmt ${MPI_LIB}/impi.lib SFML::System SFML::Graphics SFML::Window Freetype::Freetype) 

