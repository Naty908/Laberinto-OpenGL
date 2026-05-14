#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <random>
#include "shaders/shader_loader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

const unsigned int SRC_WIDTH = 1280;
const unsigned int SRC_HEIGHT = 720;

ma_engine engine;
ma_sound bgm;      
ma_sound pasos;    

const int NUM_ESTRELLAS = 150; 
unsigned int estrellasVAO, estrellasVBO;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// cargar texturas
unsigned int cargarTextura(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA; 

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);


        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Error al cargar textura en: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

// camaras
glm::vec3 cameraPos = glm::vec3(0.0f, 1.6f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 playerPos = glm::vec3(2.0f, 0.0f, 2.0f);
bool firstPerson = true;

// --- LABERINTO NIVEL 1 (Matriz Global) ---
const int FILAS = 10;
const int COLUMNAS = 10;
int laberintoNivel1[FILAS][COLUMNAS] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1} // El 0 de aquí es la puerta de salida
};

// Tus variables del jugador deben estar justo debajo:
// glm::vec3 playerPos = glm::vec3(2.0f, 0.0f, 2.0f);
// bool firstPerson = true;
// void processInput(GLFWwindow *window) { ... }

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Control de cámara C (Orbital vs Primera Persona)
    static bool cKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cKeyPressed) {
            firstPerson = !firstPerson;
            cKeyPressed = true;
        }
    } else {
        cKeyPressed = false;
    }

    // --- CÁLCULO DE COLISIÓN ---
    float velocity = 4.0f * deltaTime; // Ajusta la velocidad si lo sientes muy rápido
    glm::vec3 nextPos = playerPos; 

    // Calculamos el movimiento deseado
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        nextPos += cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        nextPos -= cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        nextPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        nextPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;

    // Mantenemos la altura (Y) fija para que no "vuele" ni se hunda
    nextPos.y = playerPos.y; 

    // Convertimos la posición 3D a coordenadas de la Matriz
    // Dividimos entre 2.0f porque escalamos los muros a ese tamaño en el render
    int matrizX = round(nextPos.x / 2.0f);
    int matrizZ = round(nextPos.z / 2.0f);

    // Verificamos límites y si la celda es un pasillo (0)
    if (matrizX >= 0 && matrizX < COLUMNAS && matrizZ >= 0 && matrizZ < FILAS) {
        if (laberintoNivel1[matrizZ][matrizX] == 0) {
            playerPos = nextPos; // Solo permitimos el movimiento si es un pasillo
        }
    } else {
        // Si se sale de los límites del laberinto (la salida), permitimos movimiento libre
        playerPos = nextPos; 
    }
}

// suelo
float vericesSuelo[] = {
    -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f};

    //sprite
float verticesSprite[] = {
    -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, 
    -0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 
     0.5f,  0.0f, 0.0f,  1.0f, 1.0f, 

    -0.5f,  1.0f, 0.0f,  0.0f, 0.0f,
     0.5f,  0.0f, 0.0f,  1.0f, 1.0f,
     0.5f,  1.0f, 0.0f,  1.0f, 0.0f
};


// raton
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SRC_WIDTH;
float lastY = SRC_HEIGHT;

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// cubo
float verticesCubo[] = {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f};

// dibujo
void dibujarBancaReal(unsigned int modelLoc, glm::mat4 base, unsigned int vao)
{
    glBindVertexArray(vao);
    glm::vec3 p[] = {{-.7, .2, .2}, {.7, .2, .2}, {-.7, .2, -.2}, {.7, .2, -.2}};
    for (int i = 0; i < 4; i++)
    {
        glm::mat4 m = glm::translate(base, p[i]);
        m = glm::scale(m, glm::vec3(0.1f, 0.4f, 0.1f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    for (int i = 0; i < 3; i++)
    {
        glm::mat4 m = glm::translate(base, glm::vec3(0.0f, 0.45f, -0.2f + i * 0.2f));
        m = glm::scale(m, glm::vec3(1.6f, 0.05f, 0.15f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glm::mat4 r = glm::translate(base, glm::vec3(0.0f, 0.7f, -0.3f));
    r = glm::rotate(r, glm::radians(-15.0f), glm::vec3(1, 0, 0));
    r = glm::scale(r, glm::vec3(1.6f, 0.4f, 0.05f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(r));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// arbol draw
void dibujarArbol2D(unsigned int shaderProgram, unsigned int vao, unsigned int texturaID, glm::vec3 pos, glm::vec3 camPos) {
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texturaID);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);


    float angle = atan2(camPos.x - pos.x, camPos.z - pos.z);
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(4.0f, 8.0f, 1.0f)); 

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, 6);
}


// farola draw
void dibujarFarola(unsigned int modelLoc, unsigned int uColorLoc, glm::vec3 pos, unsigned int vao)
{
    glBindVertexArray(vao);


    glm::mat4 base = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, 0.1f, 0.0f));
    base = glm::scale(base, glm::vec3(0.4f, 0.2f, 0.4f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(base));
    glUniform3f(uColorLoc, 0.05f, 0.05f, 0.05f); 
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glm::mat4 poste = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, 1.5f, 0.0f));
    poste = glm::scale(poste, glm::vec3(0.12f, 3.0f, 0.12f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(poste));
    glUniform3f(uColorLoc, 0.1f, 0.1f, 0.1f);
    glDrawArrays(GL_TRIANGLES, 0, 36);


    glm::mat4 lampara = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, 3.2f, 0.0f));
    lampara = glm::scale(lampara, glm::vec3(0.4f, 0.5f, 0.4f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lampara));
    

    glUniform3f(uColorLoc, 2.0f, 1.5f, 0.5f); 
    glDrawArrays(GL_TRIANGLES, 0, 36);


    glm::mat4 tapa = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, 3.45f, 0.0f));
    tapa = glm::scale(tapa, glm::vec3(0.5f, 0.1f, 0.5f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(tapa));
    glUniform3f(uColorLoc, 0.05f, 0.05f, 0.05f);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// arbusto draw
void dibujarArbusto(unsigned int modelLoc, glm::vec3 pos, unsigned int vao)
{
    glBindVertexArray(vao);
    glm::mat4 arbusto = glm::translate(glm::mat4(1.0f), pos);
    arbusto = glm::scale(arbusto, glm::vec3(0.8f, 0.6f, 0.8f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(arbusto));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}



// cinematica
float tiempoCinematica = 0.0f;
const float DuracionTotal = 30.0f;
glm::vec3 posicionInicialCinematica;
bool primeravezCinematica = true;

// posiciones aleatorias
struct Arbol {
    glm::vec3 pos;
};
Arbol arboles[50];

struct Farola {
    glm::vec3 pos;
};
Farola farolas[5];

struct  Edificio
{
    glm::vec3 pos;
    glm::vec3 tamaño;
    glm::vec3 color;
};
// dibujar edificios
void dibujarEdificio(unsigned int modelLoc, unsigned int uColorLoc, Edificio edif, unsigned int vao)
{
    glBindVertexArray(vao);
    
    glm::mat4 edificio = glm::translate(glm::mat4(1.0f), edif.pos);
    edificio = glm::scale(edificio, edif.tamaño);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(edificio));
    glUniform3f(uColorLoc, 0.08f, 0.08f, 0.1f);  
    glDrawArrays(GL_TRIANGLES, 0, 36);
}



// Personaje Draw
struct  Mesh {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
};

Mesh cargarModelo(const char* path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals  | aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR ASSIMP: " << importer.GetErrorString() << std::endl;
        return Mesh{};
    }

    Mesh mesh;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* aiMesh = scene->mMeshes[m];
        unsigned int offsetVértices = mesh.vertices.size() / 8; 

        for (unsigned int i = 0; i < aiMesh->mNumVertices; i++) {

            mesh.vertices.push_back(aiMesh->mVertices[i].x);
            mesh.vertices.push_back(aiMesh->mVertices[i].y);
            mesh.vertices.push_back(aiMesh->mVertices[i].z);

            if (aiMesh->HasNormals()) {
                mesh.vertices.push_back(aiMesh->mNormals[i].x);
                mesh.vertices.push_back(aiMesh->mNormals[i].y);
                mesh.vertices.push_back(aiMesh->mNormals[i].z);
            } else {
                mesh.vertices.push_back(0.0f); mesh.vertices.push_back(1.0f); mesh.vertices.push_back(0.0f);
            }


            if (aiMesh->mTextureCoords[0]) {
                mesh.vertices.push_back(aiMesh->mTextureCoords[0][i].x);
                mesh.vertices.push_back(aiMesh->mTextureCoords[0][i].y);
            } else {
                mesh.vertices.push_back(0.0f); mesh.vertices.push_back(0.0f);
            }
        }

        for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
            aiFace face = aiMesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                mesh.indices.push_back(face.mIndices[j] + offsetVértices);
            }
        }
    }

    std::cout << "Modelo cargado: " << path << " con " << scene->mNumMeshes << " partes." << std::endl;


    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
    
    int stride = 8 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);


    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    return mesh;
}
void dibujarchica(unsigned int modelLoc, unsigned int uColorLoc, 
                   Mesh& mesh, glm::vec3 pos, float escala) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0, 1, 0));
    
    model = glm::scale(model, glm::vec3(escala));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(uColorLoc, 1.0f, 1.0f, 1.0f); 
    
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
}

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "Komorebi Park", nullptr, nullptr);
    if (window == NULL)
    {
        std::cout << "Error al crear ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_BLEND);     
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Mesh chicaMOdel = cargarModelo("/Users/natyv/Documents/Nataly/Laberinto-OpenGL/assets/models/GirlModel.glb");
    unsigned int texturaArbol = cargarTextura("/Users/natyv/Documents/Nataly/Laberinto-OpenGL/assets/textures/arbol.png");


    // arboles init
    std::mt19937 gen(70);
    std::uniform_real_distribution<> disX(-30.0f, 30.0f);
    std::uniform_real_distribution<> disZ(-50.0f, 0.0f);

    for (int i = 0; i < 50; i++)
    {
        arboles[i].pos = glm::vec3(disX(gen), 0.0f, disZ(gen));
    }

    // farolas init :)
    farolas[0].pos = glm::vec3(-20.0f, 0.0f, -10.0f);
    farolas[1].pos = glm::vec3(20.0f, 0.0f, -10.0f);
    farolas[2].pos = glm::vec3(-15.0f, 0.0f, -30.0f);
    farolas[3].pos = glm::vec3(15.0f, 0.0f, -30.0f);
    farolas[4].pos = glm::vec3(0.0f, 0.0f, -50.0f);

    //edificios init
    Edificio edificios[12];

    float radioCirculo = 60.0f;  
    int numEdificios = 10;       
    for (int i = 0; i < numEdificios; i++)
    {
        float angulo = (3.14159f * i) / (numEdificios -1); 
        
        float x = cos(angulo) * radioCirculo;
        float z = -sin(angulo) * radioCirculo - 40.0f;  
        
        float alturaAleatoria = 15.0f + (i % 3) * 3.0f;  
        float anchoAleatorio = 10.0f + (i % 3) * 2.0f;   
        
        edificios[i].pos = glm::vec3(x, alturaAleatoria / 2.0f, z);  
        edificios[i].tamaño = glm::vec3(anchoAleatorio, alturaAleatoria, 10.0f);  
        edificios[i].color = glm::vec3(0.08f, 0.08f, 0.1f);  
    }

       // estrellas
    std::vector<float> verticesEstrellas;
    std::mt19937 genEstrellas(12345); 
    std::uniform_real_distribution<> disXstars(-50.0f, 50.0f); 
    std::uniform_real_distribution<> disYStars(15.0f, 25.0f);  
    std::uniform_real_distribution<> disZStars(-100.0f, 0.0f);

    for (int i = 0; i < NUM_ESTRELLAS; ++i) {
        verticesEstrellas.push_back(disXstars(genEstrellas)); 
        verticesEstrellas.push_back(disYStars(genEstrellas)); 
        verticesEstrellas.push_back(disZStars(genEstrellas)); 
    }

    unsigned int VAO[3], VBO[3];
    glGenVertexArrays(3, VAO);
    glGenBuffers(3, VBO);

    // suelo
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vericesSuelo), vericesSuelo, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // cubo
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCubo), verticesCubo, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // arboles 2d
    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesSprite), verticesSprite, GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // estrellas
    glGenVertexArrays(1, &estrellasVAO);
    glGenBuffers(1, &estrellasVBO);

    glBindVertexArray(estrellasVAO);
    glBindBuffer(GL_ARRAY_BUFFER, estrellasVBO);
    glBufferData(GL_ARRAY_BUFFER, verticesEstrellas.size() * sizeof(float), verticesEstrellas.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // shaders
    unsigned int animeShader = loadShader("/Users/natyv/Documents/Nataly/Laberinto-OpenGL/shaders/vertex_shader.glsl", "/Users/natyv/Documents/Nataly/Laberinto-OpenGL/shaders/fragment_shader.glsl");
    unsigned int skyboxShader = loadShader("/Users/natyv/Documents/Nataly/Laberinto-OpenGL/shaders/sky_v.glsl", "/Users/natyv/Documents/Nataly/Laberinto-OpenGL/shaders/sky_f.glsl");

    int modelLoc = glGetUniformLocation(animeShader, "model");
    int uColorLoc = glGetUniformLocation(animeShader, "uColor");
    int uFadeLoc = glGetUniformLocation(animeShader, "uFade");
    int viewPosLoc = glGetUniformLocation(animeShader, "viewPos");
    int fogColorLoc = glGetUniformLocation(animeShader, "fogColor");
    int lightColorLoc = glGetUniformLocation(animeShader, "lightColor");
    int lightPosLoc = glGetUniformLocation(animeShader, "lightPos");
    int projLoc = glGetUniformLocation(animeShader, "projection");
    int viewLoc = glGetUniformLocation(animeShader, "view");

    int skyViewLoc = glGetUniformLocation(skyboxShader, "view");
    int skyProjLoc = glGetUniformLocation(skyboxShader, "projection");
    int skyFadeLoc = glGetUniformLocation(skyboxShader, "uFade");

    //sonido
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
    std::cout << "Error al iniciar el motor de audio" << std::endl;
    }   


    ma_sound_init_from_file(&engine, "assets/audio/Nostalgia.mp3", 0, NULL, NULL, &bgm);
    ma_sound_start(&bgm);


    ma_sound_init_from_file(&engine, "assets/audio/Minecraft - Pasos (sound effect).mp3", 0, NULL, NULL, &pasos);
    ma_sound_set_looping(&pasos, MA_TRUE);

    glEnable(GL_BLEND);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);


    bool cinematicaActiva = false;
    bool mostrarChica = false;

while (!glfwWindowShouldClose(window)) {
        // 1. Tiempos y Teclado
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- AUDIO DE PASOS ---
        bool estaCaminando = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || 
                              glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
                              glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                              glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
        if (estaCaminando) {
            if (!ma_sound_is_playing(&pasos)) ma_sound_start(&pasos); 
        } else {
            if (ma_sound_is_playing(&pasos)) ma_sound_stop(&pasos); 
        }

        // 2. Colores del atardecer
        glm::vec3 sunsetColor = glm::vec3(0.95f, 0.6f, 0.3f);
        glm::vec3 sunsetLight = glm::vec3(1.5f, 1.2f, 0.8f);

        glClearColor(sunsetColor.x, sunsetColor.y, sunsetColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 3. Activar shader principal y configurar cámara
        glUseProgram(animeShader);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view;
        if (firstPerson) {
            cameraPos = playerPos + glm::vec3(0.0f, 1.6f, 0.0f);
            view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        } else {
            cameraPos = playerPos + glm::vec3(0.0f, 15.0f, 10.0f);
            view = glm::lookAt(cameraPos, playerPos, cameraUp); 
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // 4. Luces y Niebla
        glUniform3f(lightColorLoc, sunsetLight.x, sunsetLight.y, sunsetLight.z); 
        glUniform3f(lightPosLoc, 10.0f, 50.0f, 10.0f); 
        glUniform3f(fogColorLoc, sunsetColor.x, sunsetColor.y, sunsetColor.z); 
        glUniform1f(uFadeLoc, 1.0f);                  
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        
        // Desactivamos texturas para que los muros y el piso usen colores sólidos
        glUniform1i(glGetUniformLocation(animeShader, "usesTexture"), 0);

        // 5. Dibujar el Suelo Verde (Un solo piso limpio)
        glBindVertexArray(VAO[0]);
        glm::mat4 modelSuelo = glm::mat4(1.0f);
        modelSuelo = glm::scale(modelSuelo, glm::vec3(100.0f, 1.0f, 100.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSuelo));
        glUniform3f(uColorLoc, 0.3f, 0.5f, 0.2f); 
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 6. Dibujar el Laberinto
        // 6. Dibujar el Laberinto
glBindVertexArray(VAO[1]); 
glUniform3f(uColorLoc, 0.8f, 0.8f, 0.8f); // Color gris claro para los muros
for (int z = 0; z < FILAS; z++) {
    for (int x = 0; x < COLUMNAS; x++) {
        if (laberintoNivel1[z][x] == 1) {
            glm::mat4 modelMuro = glm::mat4(1.0f);
            // IMPORTANTE: Multiplicar por 2.0f para que coincida con la colisión
            modelMuro = glm::translate(modelMuro, glm::vec3(x * 2.0f, 1.0f, z * 2.0f)); 
            modelMuro = glm::scale(modelMuro, glm::vec3(2.0f, 2.0f, 2.0f)); 
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMuro));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
}

        // 7. Dibujar los Árboles (requieren textura)
        glUniform1i(glGetUniformLocation(animeShader, "usesTexture"), 1);   
        glBindVertexArray(VAO[2]);
        glUniform3f(uColorLoc, 0.4f, 0.25f, 0.1f);
        for (int i = 0; i < 50; i++) {
            dibujarArbol2D(animeShader, VAO[2], texturaArbol, arboles[i].pos, cameraPos);
        }

        // 8. Dibujar a la Chica (solo en vista orbital)
        if (!firstPerson) {
            glUniform1i(glGetUniformLocation(animeShader, "usesTexture"), 0);
            dibujarchica(modelLoc, uColorLoc, chicaMOdel, playerPos, 1.5f); 
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(3, VAO);
    glDeleteBuffers(3, VBO);
    ma_engine_uninit(&engine);
    glfwTerminate();
    return 0;
}