cmake_minimum_required(VERSION 3.10)
project(OpenGLApp)

set(CMAKE_CXX_STANDARD 17)

# Rutas para tu Mac M2
include_directories(include /opt/homebrew/include)
link_directories(/opt/homebrew/lib)

add_executable(app main.cpp src/glad.c)

if(APPLE)
    # Incluimos assimp aquí para que el ejecutable sepa leer modelos 3D
    target_link_libraries(app glfw glew assimp "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
else()
    target_link_libraries(app glfw GL dl assimp)
endif()